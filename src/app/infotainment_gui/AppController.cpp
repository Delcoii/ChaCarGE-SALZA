#include "AppController.h"

#include <QMetaObject>
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
    baseData.setFrameSignals(frame.signSignal, frame.warningSignal, frame.emotion, 4);
}

void AppController::join() {
    if (producerThread.joinable()) producerThread.join();
    if (rendererThread.joinable()) rendererThread.join();
}

void AppController::loadAssets(ImageData& imageData) {
    const QString base = "../../../resources/images/";
    imageData.loadDefaultImage(ImageData::DefaultImageType::MENU_ICON_1, base + "button.png");
    imageData.loadDefaultImage(ImageData::DefaultImageType::SIGNAL_CORNER, base + "Signal.png");
    imageData.loadSignImage(ImageData::SignType::BUMP, base + "bump.png");
    imageData.loadSignImage(ImageData::SignType::OVERSPEED, base + "overspeed.png");
    imageData.loadSignImage(ImageData::SignType::OVERTURN, base + "turn.png");
    imageData.loadSignImage(ImageData::SignType::SIGNAL_VIOLATION, base + "regalSignal.png");
    imageData.loadEmotionGif(ImageData::EmotionGifType::HAPPY, base + "thumbs-up-2584.gif", 150);
    imageData.loadEmotionGif(ImageData::EmotionGifType::BAD_FACE, base + "bad_face.gif", 150);
    imageData.loadTierImage(ImageData::TierType::BRONZE, base + "bronze.png");
    imageData.loadTierImage(ImageData::TierType::SILVER, base + "silver.png");
    imageData.loadTierImage(ImageData::TierType::GOLD, base + "gold.png");
    imageData.loadTierImage(ImageData::TierType::DIAMOND, base + "diamond.png");
    imageData.loadTierImage(ImageData::TierType::MASTER, base + "master.png");
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

        // Traffic sign
        uint8_t signSignal = static_cast<uint8_t>(shmPtr->given_info.traffic_state.data);

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

        baseData.setFrameSignals(signSignal, warningSignal, emotionEncoded, baseData.getCurDisplayType());

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
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
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }
}
