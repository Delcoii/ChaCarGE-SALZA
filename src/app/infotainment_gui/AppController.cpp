#include "AppController.h"

#include <QMetaObject>
#include <chrono>
#include <thread>

AppController::AppController(BaseData& base, RenderingData& rendering, InfotainmentWidget& uiWidget)
    : baseData(base), renderingData(rendering), ui(uiWidget) {}

AppController::~AppController() {
    stop();
    join();
}

void AppController::start() {
    running = true;
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
    imageData.loadTierImage(ImageData::TierType::BRONZE, base + "bronze.png");
    imageData.loadTierImage(ImageData::TierType::SILVER, base + "silver.png");
    imageData.loadTierImage(ImageData::TierType::GOLD, base + "gold.png");
    imageData.loadTierImage(ImageData::TierType::DIAMOND, base + "diamond.png");
    imageData.loadTierImage(ImageData::TierType::MASTER, base + "master.png");
}

void AppController::producerLoop() {
    uint8_t step = 0;
    while (running) {
        auto curDisplay = baseData.getCurDisplayType();
        if (curDisplay == 4) break;

        // If user toggled to ScoreBoard, keep producer idle to avoid overriding the view.
        if (curDisplay == static_cast<uint8_t>(RenderingData::DisplayType::ScoreBoard)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        uint8_t sign = static_cast<uint8_t>(ImageData::SignType::BUMP);
        uint8_t warning = 0;
        uint8_t emotion = static_cast<uint8_t>(ImageData::EmotionGifType::HAPPY);
        uint8_t display = static_cast<uint8_t>(RenderingData::DisplayType::Dashboard);
        baseData.setFrameSignals(sign, warning, emotion, display);
        if (step++ > 20) {
            baseData.setFrameSignals(sign, warning, emotion, 4);
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
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
