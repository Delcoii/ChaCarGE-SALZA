#include "AppController.h"

#include <QMetaObject>
#include <QDir>
#include <chrono>
#include <thread>
#include <cmath>
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
    baseData.setFrameSignals(frame.rawData, frame.warningSignal, frame.emotion, 4);
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
    while (running) {
        if (!shmPtr) {
            shmPtr = init_shared_memory();
            if (!shmPtr) {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                continue;
            }
        }

        // Traffic sign + vehicle command raw inputs
        BaseData::RawData rawData{};
        rawData.signSignal = static_cast<uint8_t>(shmPtr->given_info.traffic_state.sign_state);
        rawData.throttle = shmPtr->given_info.vehicle_command.throttle;
        rawData.brake = shmPtr->given_info.vehicle_command.brake;
        rawData.steerTireDegree = shmPtr->given_info.vehicle_command.steer_tire_degree;

        // Driving score
        const DrivingScore& ds = shmPtr->generated_info.driving_score;
        uint8_t warningSignal = ds.score_type;
        int delta = static_cast<int>(std::llround(ds.total_score));

        // Map score_type -> UserData::ScoreType
        auto& ud = UserData::getInstance();
        if (warningSignal < static_cast<uint8_t>(UserData::ScoreType::MAX_SCORE_TYPES)) {
            auto st = static_cast<UserData::ScoreType>(warningSignal);
            ud.adjustCurScore(st, delta);
        }
        ud.adjustUserTotalScore(delta);

        uint8_t emotionEncoded =
            (delta < 0) ? static_cast<uint8_t>(ImageData::EmotionGifType::BAD_FACE)
                        : static_cast<uint8_t>(ImageData::EmotionGifType::HAPPY);

        baseData.setFrameSignals(rawData, warningSignal, emotionEncoded, baseData.getCurDisplayType());

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        if (baseData.getCurDisplayType() == 4) break;
    }
}

void AppController::rendererLoop() {
    while (running) {
        auto payload = renderingData.composeFrame();
        QMetaObject::invokeMethod(
            &ui,
            [payload, this]() { ui.showFrame(payload); },
            Qt::QueuedConnection);

        if (baseData.getCurDisplayType() == 4) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}
