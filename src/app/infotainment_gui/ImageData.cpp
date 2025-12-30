#include "ImageData.h"
#include <QtCore/QString>

ImageData& ImageData::getInstance() {
    static ImageData instance;
    return instance;
}

ImageData::ImageData() {
}

ImageData::~ImageData() {
}

bool ImageData::loadEmotionGif(EmotionGifType type, const QString& path, int speedPercent) {
    const auto idx = static_cast<size_t>(type);
    auto movie = std::make_unique<QMovie>(path);
    if (!movie->isValid()) {
        return false;
    }
    movie->setSpeed(speedPercent);
    emotionGifs[idx] = std::move(movie);
    return true;
}

bool ImageData::loadDefaultImage(DefaultImageType type, const QString& path) {
    const auto idx = static_cast<size_t>(type);
    QPixmap pix(path);
    if (pix.isNull()) return false;
    defaultImages[idx] = std::move(pix);
    return true;
}

bool ImageData::loadSignImage(SignType type, const QString& path) {
    const auto idx = static_cast<size_t>(type);
    QPixmap pix(path);
    if (pix.isNull()) return false;
    signImages[idx] = std::move(pix);
    return true;
}

bool ImageData::loadWarningIcon(WarningIconType type, const QString& path) {
    const auto idx = static_cast<size_t>(type);
    QPixmap pix(path);
    if (pix.isNull()) return false;
    warningIcons[idx] = std::move(pix);
    return true;
}

bool ImageData::loadTierImage(TierType type, const QString& path) {
    const auto idx = static_cast<size_t>(type);
    QPixmap pix(path);
    if (pix.isNull()) return false;
    tierImages[idx] = std::move(pix);
    return true;
}

bool ImageData::loadSteeringWheel(const QString& path) {
    QPixmap pix(path);
    if (pix.isNull()) return false;
    steeringWheel = std::move(pix);
    return true;
}

QMovie* ImageData::getEmotionGif(EmotionGifType type) const {
    return emotionGifs[static_cast<size_t>(type)].get();
}

const QPixmap* ImageData::getDefaultImage(DefaultImageType type) const {
    const auto& pix = defaultImages[static_cast<size_t>(type)];
    return pix.isNull() ? nullptr : &pix;
}

const QPixmap* ImageData::getSignImage(SignType type) const {
    const auto& pix = signImages[static_cast<size_t>(type)];
    return pix.isNull() ? nullptr : &pix;
}

const QPixmap* ImageData::getWarningIcon(WarningIconType type) const {
    const auto& pix = warningIcons[static_cast<size_t>(type)];
    return pix.isNull() ? nullptr : &pix;
}

const QPixmap* ImageData::getTierImage(TierType type) const {
    const auto& pix = tierImages[static_cast<size_t>(type)];
    return pix.isNull() ? nullptr : &pix;
}

const QPixmap* ImageData::getSteeringWheel() const {
    return steeringWheel.isNull() ? nullptr : &steeringWheel;
}
