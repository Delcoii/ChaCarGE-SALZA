#ifndef IMAGE_DATA_H
#define IMAGE_DATA_H

#include <cstdint>
#include <array>
#include <memory>
#include <QtCore/QString>
#include <QtGui/QPixmap>
#include <QtGui/QMovie>

class ImageData {
public:
    enum class DefaultImageType : uint8_t {
        //BACKGROUND,
        MENU_ICON_1,
        SIGNAL_CORNER,
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

    enum class TierType : uint8_t {
        BRONZE,
        SILVER,
        GOLD,
        DIAMOND,
        MASTER,
        MAX_TIER_TYPES,
    };

private:
    ImageData();
    ~ImageData();                         // ✅ 포인터 해제 필요해서 default 제거
    ImageData(const ImageData&) = delete;
    ImageData& operator=(const ImageData&) = delete;

public:
    static ImageData& getInstance();

    // Loaders (경로 입력 → Qt asset 로드)
    bool loadEmotionGif(EmotionGifType type, const QString& path, int speedPercent = 100);
    bool loadDefaultImage(DefaultImageType type, const QString& path);
    bool loadSignImage(SignType type, const QString& path);
    bool loadWarningIcon(WarningIconType type, const QString& path);
    bool loadTierImage(TierType type, const QString& path);

    // Getters (nullptr or isNull()이면 아직 로드되지 않은 상태)
    QMovie* getEmotionGif(EmotionGifType type) const;
    const QPixmap* getDefaultImage(DefaultImageType type) const;
    const QPixmap* getSignImage(SignType type) const;
    const QPixmap* getWarningIcon(WarningIconType type) const;
    const QPixmap* getTierImage(TierType type) const;

private:
    // ✅ QMovie는 값으로 배열 보관 X → 포인터로 보관
    std::array<std::unique_ptr<QMovie>, static_cast<size_t>(EmotionGifType::MAX_GIF_TYPES)> emotionGifs{};

    std::array<QPixmap, static_cast<size_t>(SignType::MAX_SIGN_TYPES)> signImages{};
    std::array<QPixmap, static_cast<size_t>(WarningIconType::MAX_ICON_TYPES)> warningIcons{};
    std::array<QPixmap, static_cast<size_t>(DefaultImageType::MAX_IMAGE_TYPES)> defaultImages{};
    std::array<QPixmap, static_cast<size_t>(TierType::MAX_TIER_TYPES)> tierImages{};
};

#endif // IMAGE_DATA_H
