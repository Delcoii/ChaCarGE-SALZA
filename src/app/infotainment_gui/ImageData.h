#ifndef IMAGE_DATA_H
#define IMAGE_DATA_H

#include <cstdint>
#include <QtGui/QPixmap>
#include <QtGui/QMovie>

class ImageData {
public:
    enum class DefaultImageType : uint8_t {
        //BACKGROUND,
        MENU_ICON_1,
        MAX_IMAGE_TYPES,
    };

    enum class SignType : uint8_t {
        NONE,
        BUMP,
        OVERSPEED,
        OVERTURN,
        SIGNAL_VIOLATION,   
        MAX_SIGN_TYPES,
    };

    enum class WarningIconType : uint8_t {
        NONE,
        SEATBELT,
        DOOR_OPEN,
        MAX_ICON_TYPES,
    };

    enum class EmotionGifType : uint8_t {
        HAPPY,
        //SAD,
        //ANGRY,
        //SURPRISED,
        MAX_GIF_TYPES,
    };

private:
    ImageData();
    ~ImageData();                         // ✅ 포인터 해제 필요해서 default 제거
    ImageData(const ImageData&) = delete;
    ImageData& operator=(const ImageData&) = delete;

public:
    static ImageData& getInstance();

    //QMovie* getEmotionGif(EmotionGifType type) const {
        //return emotionGifs[static_cast<uint8_t>(type)];
    //}
    //const QPixmap& getDefaultImage(DefaultImageType type) const {
    //    return defaultImages[static_cast<uint8_t>(type)];
    //}

private:
    // ✅ QMovie는 값으로 배열 보관 X → 포인터로 보관
    //QMovie* emotionGifs[static_cast<uint8_t>(EmotionGifType::MAX_GIF_TYPES)] = {nullptr};

    //QPixmap signImages[static_cast<uint8_t>(SignType::MAX_SIGN_TYPES)];
    //QPixmap warningIcons[static_cast<uint8_t>(WarningIconType::MAX_ICON_TYPES)];
    //QPixmap defaultImages[static_cast<uint8_t>(DefaultImageType::MAX_IMAGE_TYPES)];
};

#endif // IMAGE_DATA_H