#include "AppController.h"
#include "Common.h"

#include <QMetaObject>
#include <QDir>
#include <atomic>
#include <chrono>
#include <thread>
#include <cmath>
#include <iostream>

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
    DrivingScore prevDrivingScore{};
    bool hasPrevDrivingScore = false;

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
        const bool useDrivingCheck = (generated.bUseDrivingScoreChecking != 0);
        const DrivingScore& ds = generated.driving_score;
        const bool scoreTypeValid = ds.score_type <= static_cast<uint8_t>(SCORE_V2V_DISTANCE);
        const bool directionValid = ds.score_direction <= static_cast<uint8_t>(SCORE_MINUS);
        const bool scoreFinite = std::isfinite(ds.total_score);
        const bool hasDelta = scoreFinite && fabs(ds.total_score) > TOLERANCE_DOUBLE;
        const bool drivingScorePresent = scoreTypeValid && directionValid && hasDelta;
        uint8_t warningSignal = drivingScorePresent ? ds.score_type : 0xFF;
        double delta = drivingScorePresent ? ds.total_score : 0.0;
        uint8_t scoreDirection = drivingScorePresent ? ds.score_direction : static_cast<uint8_t>(SCORE_NORMAL);

        if (useDrivingCheck && drivingScorePresent) {
            const bool drivingScoreChanged =
                !hasPrevDrivingScore ||
                (fabs(ds.total_score - prevDrivingScore.total_score) > TOLERANCE_DOUBLE ||
                 ds.score_type != prevDrivingScore.score_type ||
                 ds.score_direction != prevDrivingScore.score_direction);
            if (drivingScoreChanged) {
                if (hasPrevDrivingScore) {
                    std::cout << "[producerLoop] generated_info changed: "
                              << "total_score " << prevDrivingScore.total_score << " -> " << ds.total_score
                              << ", score_type " << static_cast<int>(prevDrivingScore.score_type)
                              << " -> " << static_cast<int>(ds.score_type)
                              << ", score_direction " << static_cast<int>(prevDrivingScore.score_direction)
                              << " -> " << static_cast<int>(ds.score_direction)
                              << std::endl;
                } else {
                    std::cout << "[producerLoop] generated_info initial: "
                              << "total_score=" << ds.total_score
                              << " score_type=" << static_cast<int>(ds.score_type)
                              << " score_direction=" << static_cast<int>(ds.score_direction)
                              << std::endl;
                }
            }
            prevDrivingScore = ds;
            hasPrevDrivingScore = true;
        } else {
            hasPrevDrivingScore = false;
        }

        const bool useCheckStarted = useDrivingCheck && !lastUseDrivingCheck;
        const bool useCheckStopped = !useDrivingCheck && lastUseDrivingCheck;
        if (useCheckStarted || useCheckStopped) {
            std::cout << "[AppController] bUseDrivingScoreChecking "
                      << (useDrivingCheck ? "ON" : "OFF")
                      << " ds: score=" << ds.total_score
                      << " type=" << static_cast<int>(ds.score_type)
                      << " dir=" << static_cast<int>(ds.score_direction)
                      << std::endl;
        }
        if (useCheckStarted) {
            activeViolations.clear();
            lastMinusType = 0xFF;
        }

        if (!useDrivingCheck || ds.score_direction != static_cast<uint8_t>(SCORE_MINUS)) {
            lastMinusType = 0xFF;
        }

        if (useDrivingCheck && ds.score_direction == static_cast<uint8_t>(SCORE_MINUS)) {
            const bool trackable =
                ds.score_type == static_cast<uint8_t>(SCORE_BUMP) ||
                ds.score_type == static_cast<uint8_t>(SCORE_SUDDEN_ACCEL) ||
                ds.score_type == static_cast<uint8_t>(SCORE_SUDDEN_CURVE) ||
                ds.score_type == static_cast<uint8_t>(SCORE_IGNORE_SIGN);
            if (trackable) {
                if (ds.score_type != lastMinusType) {
                    const auto now = std::chrono::system_clock::now().time_since_epoch();
                    const auto tsMs = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
                    BaseData::ViolationEvent ev{
                        static_cast<uint8_t>(ds.score_type),
                        static_cast<int64_t>(tsMs)
                    };
                    activeViolations.push_back(ev);
                    lastMinusType = ds.score_type;
                }
            }
        }
        lastUseDrivingCheck = useDrivingCheck;

        // Map score_type -> UserData::ScoreType
        if (drivingScorePresent &&
            fabs(delta) > TOLERANCE_DOUBLE &&
            warningSignal < static_cast<uint8_t>(UserData::ScoreType::MAX_SCORE_TYPES)) {
            auto& ud = UserData::getInstance();
            auto st = static_cast<UserData::ScoreType>(warningSignal);
            ud.adjustCurScore(st, delta);
            ud.adjustUserTotalScore(delta);
        }
        
        // Jaeyeon : 잘했다, 못했다는 type은 필요해서 세원님이 이 타입은 보내줘야함. 
        uint8_t emotionEncoded =
            (delta < TOLERANCE_DOUBLE) ? static_cast<uint8_t>(ImageData::EmotionGifType::BAD_FACE)
                        : static_cast<uint8_t>(ImageData::EmotionGifType::HAPPY);

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
