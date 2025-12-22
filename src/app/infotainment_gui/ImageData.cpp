#include "ImageData.h"

ImageData& ImageData::getInstance() {
    static ImageData instance;
    return instance;
}

ImageData::ImageData() {
    // ===== GIF =====
    //
    // {
    //     auto idx = static_cast<uint8_t>(EmotionGifType::HAPPY);
    //     emotionGifs[idx] = new QMovie("test.gif");  
    //     emotionGifs[idx]->setSpeed(200);            // 2배 속도
    // }
    // ===== defaultImages =====
    //{
    //    auto idx = static_cast<uint8_t>(DefaultImageType::MENU_ICON_1);
    //    defaultImages[idx] = QPixmap("button.png"); 
    //}
}

ImageData::~ImageData() {
    // new로 만든 QMovie 정리
    //for (auto& m : emotionGifs) {
    //    delete m;
    //    m = nullptr;
    //}
}