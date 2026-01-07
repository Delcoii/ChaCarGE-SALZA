#include "AppController.h"
#include "Common.h"

#include <QMetaObject>
#include <QDir>
#include <atomic>
#include <chrono>
#include <thread>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>

#include "struct_traffic_sign.h"

AppController::AppController(BaseData& base, RenderingData& rendering, InfotainmentWidget& uiWidget)
    : baseData(base), renderingData(rendering), ui(uiWidget) {}

AppController::~AppController() {
    stop();
    join();
    if (shmPtr) {
        detach_shared_memory(shmPtr);
        shmPtr = nullptr;
    }
}

void AppController::start() {
    running = true;
    shmPtr = init_shared_memory();
    producerThread = std::thread(&AppController::producerLoop, this);
    rendererThread = std::thread(&AppController::rendererLoop, this);
}

void AppController::stop() {
    running = false;
    auto frame = baseData.getFrameDataCopy();
    baseData.setFrameSignals(frame.rawData,
                             frame.warningSignal,
                             frame.emotion,
                             4,
                             false,
                             {});
}

void AppController::join() {
    if (producerThread.joinable()) producerThread.join();
    if (rendererThread.joinable()) rendererThread.join();
}

void AppController::requestToggleDrivingCheck() {
    pendingToggle.fetch_add(1, std::memory_order_relaxed);
}

void AppController::loadAssets(ImageData& imageData, const QString& assetBasePath) {
    QDir base(assetBasePath);
    const auto path = [&](const QString& file) { return base.filePath(file); };

    imageData.loadDefaultImage(ImageData::DefaultImageType::MENU_ICON_1, path("button.png"));
    imageData.loadDefaultImage(ImageData::DefaultImageType::SIGNAL_CORNER, path("Signal.png"));
    imageData.loadSignImage(ImageData::SignType::NONE, path("signal_none.png"));
    imageData.loadSignImage(ImageData::SignType::TRAFFIC_RED, path("signal_red.png"));
    imageData.loadSignImage(ImageData::SignType::TRAFFIC_YELLOW, path("signal_yellow.png"));
    imageData.loadSignImage(ImageData::SignType::TRAFFIC_GREEN, path("signal_green.png"));
    imageData.loadSignImage(ImageData::SignType::BUMP, path("bump.png"));
    imageData.loadSignImage(ImageData::SignType::OVERSPEED, path("overspeed.png"));
    imageData.loadSignImage(ImageData::SignType::OVERTURN, path("turn.png"));
    imageData.loadSignImage(ImageData::SignType::SIGNAL_VIOLATION, path("regalSignal.png"));
    imageData.loadEmotionGif(ImageData::EmotionGifType::HAPPY, path("thumbs-up-2584.gif"), 150);
    imageData.loadEmotionGif(ImageData::EmotionGifType::BAD_FACE, path("bad_face.gif"), 150);
    imageData.loadTierImage(ImageData::TierType::BRONZE, path("bronze.png"));
    imageData.loadTierImage(ImageData::TierType::SILVER, path("silver.png"));
    imageData.loadTierImage(ImageData::TierType::GOLD, path("gold.png"));
    imageData.loadTierImage(ImageData::TierType::DIAMOND, path("diamond.png"));
    imageData.loadTierImage(ImageData::TierType::MASTER, path("master.png"));
    imageData.loadSteeringWheel(path("steering_wheel.png"));
}

void AppController::producerLoop() {
    // Read from shared memory and update BaseData
    auto mapToUserScoreType = [](ScoreType st, UserData::ScoreType& out) -> bool {
        switch (st) {
        case ScoreType::SCORE_BUMP: out = UserData::ScoreType::SPEED_BUMPS; return true;
        case ScoreType::SCORE_SUDDEN_ACCEL: out = UserData::ScoreType::RAPID_ACCELERATIONS; return true;
        case ScoreType::SCORE_SUDDEN_CURVE: out = UserData::ScoreType::SHARP_TURNS; return true;
        case ScoreType::SCORE_IGNORE_SIGN: out = UserData::ScoreType::SIGNAL_VIOLATIONS; return true;
        default: return false;
        }
    };

    while (running) {
        if (!shmPtr) {
            shmPtr = init_shared_memory();
            if (!shmPtr) {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                continue;
            }
        }
        // Apply any UI toggle requests before reading values
        if (pendingToggle.exchange(0, std::memory_order_acq_rel) > 0 && shmPtr) {
            uint8_t newVal = shmPtr->given_info.bUseDrivingScoreChecking ? 0 : 1;
            shmPtr->given_info.bUseDrivingScoreChecking = newVal;
            std::cout << "[AppController] UI toggle -> bUseDrivingScoreChecking "
                      << (newVal ? "ON" : "OFF") << std::endl;
        }

        // Traffic sign + vehicle command raw inputs
        const ShmGivenInfo& given = shmPtr->given_info;
        BaseData::RawData rawData{};
        rawData.signSignal = static_cast<uint8_t>(given.traffic_state.sign_state);
        rawData.throttle = given.vehicle_command.throttle;
        rawData.brake = given.vehicle_command.brake;
        rawData.steerTireDegree = given.vehicle_command.steer_tire_degree;

        // Driving score
        const ShmGeneratedInfo& generated = shmPtr->generated_info;
        const bool useDrivingCheck = (given.bUseDrivingScoreChecking != 0);
        const uint16_t rawScoreType = generated.driving_score_type.score_type;
        const uint16_t rawScoreCount = generated.driving_score_type.count;
        // Sync user total score in real time from shared memory
        {
            double incoming = generated.total_score;
            if (!std::isfinite(incoming)) incoming = 0.0;
            if (incoming < 0.0) incoming = 0.0;
            if (incoming > std::numeric_limits<uint16_t>::max()) incoming = std::numeric_limits<uint16_t>::max();
            UserData::getInstance().setUserTotalScore(static_cast<uint16_t>(std::llround(incoming)));
        }
        const bool scoreTypeInRange = rawScoreType <= static_cast<uint16_t>(ScoreType::SCORE_TYPE_NONE);
        const ScoreType scoreType = scoreTypeInRange
            ? static_cast<ScoreType>(rawScoreType)
            : ScoreType::SCORE_TYPE_NONE;
        const bool isNoneType = scoreType == ScoreType::SCORE_TYPE_NONE;

        uint8_t warningSignal = 0xFF;
        uint8_t emotionEncoded = static_cast<uint8_t>(ImageData::EmotionGifType::HAPPY);
        uint16_t countDelta = 0;
        const bool typeChanged = scoreTypeInRange && rawScoreType != lastRawScoreType;

        const bool useCheckStarted = useDrivingCheck && !lastUseDrivingCheck;
        const bool useCheckStopped = !useDrivingCheck && lastUseDrivingCheck;
        if (useCheckStarted || useCheckStopped) {
            std::cout << "[AppController] bUseDrivingScoreChecking "
                      << (useDrivingCheck ? "ON" : "OFF")
                      << " total_score=" << generated.total_score
                      << " type=" << rawScoreType
                      << " count=" << rawScoreCount
                      << std::endl;
        }
        if (useCheckStarted) {
            activeViolations.clear();
            std::fill(lastScoreCounts.begin(), lastScoreCounts.end(), 0);
            std::fill(typeChangeCounts.begin(), typeChangeCounts.end(), 0);
            UserData::getInstance().resetCurScores();
            // Baseline to NONE so the first non-NONE type triggers immediately
            lastRawScoreType = static_cast<uint16_t>(ScoreType::SCORE_TYPE_NONE);
            lastEmittedType = 0xFFFF;
            lastEmittedCount = 0;
            activeWarningType = 0xFFFF;
            activeWarningCount = 0;
            warningExpiry = std::chrono::steady_clock::time_point{};
            const auto now = std::chrono::system_clock::now().time_since_epoch();
            sessionStartMs = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
            sessionEndMs = -1;
        }

        // Track score changes/counts
        if (useDrivingCheck && scoreTypeInRange && isNoneType) {
            // Clear any active warning when analyzer reports NONE
            activeWarningType = 0xFFFF;
            activeWarningCount = 0;
            warningExpiry = std::chrono::steady_clock::time_point{};
        }

        if (useDrivingCheck && scoreTypeInRange && !isNoneType) {
            const size_t scoreIdx = std::min(static_cast<size_t>(rawScoreType),
                                             static_cast<size_t>(lastScoreCounts.size() - 1));
            const uint16_t prevCount = lastScoreCounts[scoreIdx];
            bool countChanged = (rawScoreCount != prevCount);
            if (rawScoreCount > prevCount) {
                countDelta = static_cast<uint16_t>(rawScoreCount - prevCount);
            }
            // In case the producer or analyzer resets counts, adopt the reported value as baseline
            lastScoreCounts[scoreIdx] = rawScoreCount;
            typeChangeCounts[scoreIdx] = rawScoreCount;

            UserData::ScoreType userScoreType;
            if (mapToUserScoreType(scoreType, userScoreType)) {
                // Sync UI counters directly with shared memory counts
                UserData::getInstance().setCurScore(userScoreType, rawScoreCount);
            }

            const bool trackable =
                scoreType == ScoreType::SCORE_BUMP ||
                scoreType == ScoreType::SCORE_SUDDEN_ACCEL ||
                scoreType == ScoreType::SCORE_SUDDEN_CURVE ||
                scoreType == ScoreType::SCORE_IGNORE_SIGN;
            const bool shouldFireWarning = typeChanged || countChanged;

            const auto nowSteady = std::chrono::steady_clock::now();
            if (shouldFireWarning) {
                warningSignal = static_cast<uint8_t>(scoreType);
                emotionEncoded = static_cast<uint8_t>(ImageData::EmotionGifType::BAD_FACE);
                lastEmittedType = rawScoreType;
                lastEmittedCount = rawScoreCount;
                activeWarningType = rawScoreType;
                activeWarningCount = rawScoreCount;
                warningExpiry = nowSteady + std::chrono::milliseconds(3000);
            } else if (activeWarningType != 0xFFFF && nowSteady < warningExpiry) {
                warningSignal = static_cast<uint8_t>(activeWarningType);
                emotionEncoded = static_cast<uint8_t>(ImageData::EmotionGifType::BAD_FACE);
            } else {
                warningSignal = 0xFF;
                emotionEncoded = static_cast<uint8_t>(ImageData::EmotionGifType::HAPPY);
            }

            const auto now = std::chrono::system_clock::now().time_since_epoch();
            const auto tsMs = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

            if (trackable && countDelta > 0) {
                for (uint16_t i = 0; i < countDelta; ++i) {
                    BaseData::ViolationEvent ev{
                        static_cast<uint8_t>(scoreType),
                        static_cast<int64_t>(tsMs + i)
                    };
                    activeViolations.push_back(ev);
                }
            }

            // First page visuals must mirror score_type changes 1:1 (even if count didn't rise)
            // (warningSignal already set above; nothing else needed here)
        }

        if (useDrivingCheck && scoreTypeInRange && (typeChanged || countDelta != 0)) {
            std::cout << "[AppController] score_type update -> type=" << rawScoreType
                      << " count=" << rawScoreCount
                      << " total_score=" << generated.total_score
                      << std::endl;
        }

        // Map score_type -> UserData::ScoreType
        if (useCheckStopped) {
            lastRawScoreType = 0xFFFF;
            const auto now = std::chrono::system_clock::now().time_since_epoch();
            sessionEndMs = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
        }
        if (useDrivingCheck && scoreTypeInRange) {
            lastRawScoreType = rawScoreType;
        } else if (!useDrivingCheck) {
            lastRawScoreType = static_cast<uint16_t>(ScoreType::SCORE_TYPE_NONE);
        } else {
            lastRawScoreType = 0xFFFF;
        }

        const auto curFrame = baseData.getFrameDataCopy();
        uint8_t nextDisplay = curFrame.curDisplayType;
        if (useCheckStopped && !activeViolations.empty()) {
            nextDisplay = static_cast<uint8_t>(RenderingData::DisplayType::History);
        }
        if (rawData.signSignal == static_cast<uint8_t>(TRAFFIC_STATE_GREEN)) {
            nextDisplay = static_cast<uint8_t>(RenderingData::DisplayType::Dashboard);
        }
        std::vector<BaseData::ViolationEvent> violationsForFrame;
        if (useCheckStopped) {
            violationsForFrame = activeViolations;
        } else if (!useDrivingCheck &&
                   nextDisplay == static_cast<uint8_t>(RenderingData::DisplayType::History)) {
            // Keep showing the last recorded violations until a new session starts
            violationsForFrame = curFrame.violations;
        } else {
            violationsForFrame = activeViolations;
        }
        baseData.setFrameSignals(rawData,
                                 warningSignal,
                                 emotionEncoded,
                                 nextDisplay,
                                 useDrivingCheck,
                                 std::move(violationsForFrame));
        if (useCheckStopped) {
            std::cout << "[AppController] switching to History, violations count="
                      << activeViolations.size() << std::endl;
            activeViolations.clear();
        }
        lastUseDrivingCheck = useDrivingCheck;

        // Prepare latest frame for renderer/UI
        const int bufIdx = acquireRenderBuffer();
        if (bufIdx >= 0) {
            renderingData.composeFrame(renderBuffers[bufIdx]);
            renderBuffers[bufIdx].sessionStartMs = sessionStartMs;
            renderBuffers[bufIdx].sessionEndMs = sessionEndMs;
            // Keep only the freshest frame; drop any previously unconsumed buffer
            const int prev = latestReady.exchange(bufIdx, std::memory_order_acq_rel);
            if (prev >= 0 && prev != bufIdx) {
                releaseRenderBuffer(prev);
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void AppController::rendererLoop() {
    while (running) {
        const int bufIdx = latestReady.exchange(-1, std::memory_order_acq_rel);
        if (bufIdx < 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        QMetaObject::invokeMethod(
            &ui,
            [this, bufIdx]() {
                ui.showFrame(renderBuffers[bufIdx]);
                releaseRenderBuffer(bufIdx);
            },
            Qt::QueuedConnection);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

int AppController::acquireRenderBuffer() {
    const int startIdx = nextWriteIndex;
    for (int i = 0; i < kRenderBufferCount; ++i) {
        const int idx = (startIdx + i) % kRenderBufferCount;
        bool expected = false;
        if (bufferInUse[idx].compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
            nextWriteIndex = (idx + 1) % kRenderBufferCount;
            return idx;
        }
    }
    return -1;
}

void AppController::releaseRenderBuffer(int idx) {
    if (idx < 0 || idx >= kRenderBufferCount) return;
    bufferInUse[idx].store(false, std::memory_order_release);
}
