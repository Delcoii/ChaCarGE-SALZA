#include "RenderingData.h"
#include <algorithm>
#include "ShmCompat.h"

namespace {
template <typename EnumType>
EnumType clampEnum(uint8_t raw, EnumType fallback, EnumType maxEnum) {
    const auto maxVal = static_cast<uint8_t>(maxEnum);
    return (raw < maxVal) ? static_cast<EnumType>(raw) : fallback;
}
}

RenderingData::RenderingData(BaseData& base, ImageData& images)
    : baseData(base), imageData(images) {}

RenderingData& RenderingData::getInstance() {
    static RenderingData instance(BaseData::getInstance(), ImageData::getInstance());
    return instance;
}

void RenderingData::composeFrame(RenderPayload& payload) {
    const BaseData::FrameData frame = baseData.getFrameDataCopy();

    payload = RenderPayload{};
    payload.displayType = toDisplayType(frame.curDisplayType);

    payload.throttle = std::clamp(frame.rawData.throttle, 0.0, 100.0);
    payload.brake = std::clamp(frame.rawData.brake, 0.0, 100.0);
    // Keep raw steer value; UI handles scaling/clamp for visualization
    payload.steerAngleDeg = frame.rawData.steerTireDegree;
    payload.rawSignSignal = frame.rawData.signSignal;
    payload.rawWarningSignal = frame.warningSignal;
    payload.rawEmotionSignal = frame.emotion;
    payload.scoreDirection = frame.scoreDirection;
    payload.useDrivingScoreCheckActive = frame.useDrivingScoreCheck;
    payload.violations = frame.violations;

    payload.signType = toSignType(frame.rawData.signSignal);
    payload.warningType = toWarningType(frame.warningSignal);
    payload.emotionType = toEmotionType(frame.emotion);

    // User scores snapshot
    payload.userTotalScore = frame.userData.getUserTotalScore();
    for (size_t i = 0; i < payload.curScores.size(); ++i) {
        payload.curScores[i] = frame.userData.getCurScore(static_cast<UserData::ScoreType>(i));
    }
    payload.tierType = toTierType(payload.userTotalScore);
    payload.steeringWheelImage = imageData.getSteeringWheel();

    // Default background: use MENU_ICON_1 if available (otherwise nullptr)
    payload.background = imageData.getDefaultImage(ImageData::DefaultImageType::MENU_ICON_1);

    // Choose the assets needed for each display type
    switch (payload.displayType) {
    case DisplayType::Default:
        // Use only the background (if a sign image exists, it will be filled during the correction below)
        break;
    case DisplayType::SignAlert:
        payload.signImage = imageData.getSignImage(payload.signType);
        break;
    case DisplayType::Warning:
        payload.warningIcon = imageData.getSignImage(toWarningSign(frame.warningSignal));
        break;
    case DisplayType::Emotion:
        payload.emotionGif = imageData.getEmotionGif(payload.emotionType);
        break;
    case DisplayType::Dashboard:
        payload.signImage = imageData.getSignImage(payload.signType);
        payload.warningIcon = imageData.getSignImage(toWarningSign(frame.warningSignal));
        payload.emotionGif = imageData.getEmotionGif(payload.emotionType);
        break;
    case DisplayType::ScoreBoard:
        payload.tierImage = imageData.getTierImage(payload.tierType);
        break;
    case DisplayType::History:
        // History view uses payload.violations and images fetched in the widget
        payload.tierImage = nullptr;
        break;
    case DisplayType::COUNT:
        // never hit; handled by toDisplayType
        break;
    }

    // Fill signImage for any sign type (including NONE) so the widget can render a default placeholder
    if (!payload.signImage) {
        payload.signImage = imageData.getSignImage(payload.signType);
    }
}

RenderingData::RenderPayload RenderingData::composeFrame() {
    RenderPayload payload{};
    composeFrame(payload);
    return payload;
}

RenderingData::DisplayType RenderingData::toDisplayType(uint8_t raw) {
    const auto maxVal = static_cast<uint8_t>(DisplayType::COUNT);
    return (raw < maxVal) ? static_cast<DisplayType>(raw) : DisplayType::Default;
}

ImageData::SignType RenderingData::toSignType(uint8_t raw) {
    switch (raw) {
    case 0: return ImageData::SignType::NONE;
    case 1: return ImageData::SignType::TRAFFIC_RED;
    case 2: return ImageData::SignType::TRAFFIC_YELLOW;
    case 3: return ImageData::SignType::TRAFFIC_GREEN;
    default: return ImageData::SignType::NONE;
    }
}

ImageData::WarningIconType RenderingData::toWarningType(uint8_t raw) {
    return clampEnum(raw, ImageData::WarningIconType::NONE, ImageData::WarningIconType::MAX_ICON_TYPES);
}

ImageData::SignType RenderingData::toWarningSign(uint8_t raw) {
    switch (static_cast<ScoreType>(raw)) {
    case ScoreType::SCORE_BUMP: return ImageData::SignType::BUMP;
    case ScoreType::SCORE_OVER_SPEED: return ImageData::SignType::OVERSPEED;
    case ScoreType::SCORE_SUDDEN_CURVE: return ImageData::SignType::OVERTURN;
    case ScoreType::SCORE_IGNORE_SIGN: return ImageData::SignType::SIGNAL_VIOLATION;
    case ScoreType::SCORE_SUDDEN_ACCEL: return ImageData::SignType::OVERSPEED;
    case ScoreType::SCORE_V2V_DISTANCE: return ImageData::SignType::BUMP;
    default: return ImageData::SignType::NONE;
    }
}

ImageData::EmotionGifType RenderingData::toEmotionType(uint8_t raw) {
    return clampEnum(raw, ImageData::EmotionGifType::HAPPY, ImageData::EmotionGifType::MAX_GIF_TYPES);
}

ImageData::TierType RenderingData::toTierType(uint16_t score) {
    // Simple thresholds; adjust as needed
    if (score >= 800) return ImageData::TierType::MASTER;
    if (score >= 600) return ImageData::TierType::DIAMOND;
    if (score >= 400) return ImageData::TierType::GOLD;
    if (score >= 200) return ImageData::TierType::SILVER;
    return ImageData::TierType::BRONZE;
}
