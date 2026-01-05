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
                             frame.scoreDirection,
                             4,
                             false,
                             {});
}

void AppController::join() {
    if (producerThread.joinable()) producerThread.join();
    if (rendererThread.joinable()) rendererThread.join();
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
        case SCORE_BUMP: out = UserData::ScoreType::SPEED_BUMPS; return true;
        case SCORE_SUDDEN_ACCEL: out = UserData::ScoreType::RAPID_ACCELERATIONS; return true;
        case SCORE_SUDDEN_CURVE: out = UserData::ScoreType::SHARP_TURNS; return true;
        case SCORE_IGNORE_SIGN: out = UserData::ScoreType::SIGNAL_VIOLATIONS; return true;
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
        const bool scoreTypeValid = rawScoreType < static_cast<uint16_t>(SCORE_TYPE_NONE);
        uint8_t warningSignal = 0xFF;
        uint8_t emotionEncoded = static_cast<uint8_t>(ImageData::EmotionGifType::HAPPY);
        uint8_t scoreDirection = static_cast<uint8_t>(SCORE_NORMAL);

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
            UserData::getInstance().resetCurScores();
        }

        // Track score changes/counts
        if (useDrivingCheck && scoreTypeValid) {
            const ScoreType scoreType = static_cast<ScoreType>(rawScoreType);
            const size_t scoreIdx = std::min(rawScoreType, static_cast<uint16_t>(lastScoreCounts.size() - 1));
            const uint16_t prevCount = lastScoreCounts[scoreIdx];
            lastScoreCounts[scoreIdx] = rawScoreCount;

            UserData::ScoreType userScoreType;
            if (mapToUserScoreType(scoreType, userScoreType)) {
                UserData::getInstance().setCurScore(userScoreType, rawScoreCount);
            }

            const bool trackable =
                scoreType == SCORE_BUMP ||
                scoreType == SCORE_SUDDEN_ACCEL ||
                scoreType == SCORE_SUDDEN_CURVE ||
                scoreType == SCORE_IGNORE_SIGN;

            if (trackable && rawScoreCount > prevCount) {
                const uint16_t deltaCount = rawScoreCount - prevCount;
                const auto now = std::chrono::system_clock::now().time_since_epoch();
                const auto tsMs = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
                for (uint16_t i = 0; i < deltaCount; ++i) {
                    BaseData::ViolationEvent ev{
                        static_cast<uint8_t>(scoreType),
                        static_cast<int64_t>(tsMs)
                    };
                    activeViolations.push_back(ev);
                }
                warningSignal = static_cast<uint8_t>(scoreType);
                emotionEncoded = static_cast<uint8_t>(ImageData::EmotionGifType::BAD_FACE);
                scoreDirection = static_cast<uint8_t>(SCORE_MINUS);
            }
        }

        // Map score_type -> UserData::ScoreType
        // User total score: average existing score with session score when the check stops
        if (useCheckStopped) {
            auto& ud = UserData::getInstance();
            const double prevTotal = static_cast<double>(ud.getUserTotalScore());
            double averaged = (prevTotal + generated.total_score) / 2.0;
            if (!std::isfinite(averaged)) averaged = 0.0;
            if (averaged < 0.0) averaged = 0.0;
            if (averaged > std::numeric_limits<uint16_t>::max()) averaged = std::numeric_limits<uint16_t>::max();
            ud.setUserTotalScore(static_cast<uint16_t>(std::llround(averaged)));
        }

        const auto curFrame = baseData.getFrameDataCopy();
        uint8_t nextDisplay = curFrame.curDisplayType;
        if (useCheckStopped && !activeViolations.empty()) {
            nextDisplay = static_cast<uint8_t>(RenderingData::DisplayType::History);
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
                                 scoreDirection,
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
