#include "RenderingData.h"

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

RenderingData::RenderPayload RenderingData::composeFrame() {
    const BaseData::FrameData frame = baseData.getFrameDataCopy();

    RenderPayload payload{};
    payload.displayType = toDisplayType(frame.curDisplayType);

    payload.rawSignSignal = frame.signSignal;
    payload.rawWarningSignal = frame.warningSignal;
    payload.rawEmotionSignal = frame.emotion;

    payload.signType = toSignType(frame.signSignal);
    payload.warningType = toWarningType(frame.warningSignal);
    payload.emotionType = toEmotionType(frame.emotion);

    // User scores snapshot
    payload.userTotalScore = frame.userData.getUserTotalScore();
    for (size_t i = 0; i < payload.curScores.size(); ++i) {
        payload.curScores[i] = frame.userData.getCurScore(static_cast<UserData::ScoreType>(i));
    }
    payload.tierType = toTierType(payload.userTotalScore);

    // Default background: 필요 시 MENU_ICON_1 사용 (없으면 nullptr)
    payload.background = imageData.getDefaultImage(ImageData::DefaultImageType::MENU_ICON_1);

    // Display type별로 필요한 asset 선택
    switch (payload.displayType) {
    case DisplayType::Default:
        // 배경만 사용
        break;
    case DisplayType::SignAlert:
        payload.signImage = imageData.getSignImage(payload.signType);
        break;
    case DisplayType::Warning:
        payload.warningIcon = imageData.getWarningIcon(payload.warningType);
        break;
    case DisplayType::Emotion:
        payload.emotionGif = imageData.getEmotionGif(payload.emotionType);
        break;
    case DisplayType::Dashboard:
        payload.signImage = imageData.getSignImage(payload.signType);
        payload.warningIcon = imageData.getWarningIcon(payload.warningType);
        payload.emotionGif = imageData.getEmotionGif(payload.emotionType);
        break;
    case DisplayType::ScoreBoard:
        payload.tierImage = imageData.getTierImage(payload.tierType);
        break;
    case DisplayType::COUNT:
        // never hit; handled by toDisplayType
        break;
    }

    return payload;
}

RenderingData::DisplayType RenderingData::toDisplayType(uint8_t raw) {
    const auto maxVal = static_cast<uint8_t>(DisplayType::COUNT);
    return (raw < maxVal) ? static_cast<DisplayType>(raw) : DisplayType::Default;
}

ImageData::SignType RenderingData::toSignType(uint8_t raw) {
    return clampEnum(raw, ImageData::SignType::NONE, ImageData::SignType::MAX_SIGN_TYPES);
}

ImageData::WarningIconType RenderingData::toWarningType(uint8_t raw) {
    return clampEnum(raw, ImageData::WarningIconType::NONE, ImageData::WarningIconType::MAX_ICON_TYPES);
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
