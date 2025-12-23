#ifndef RENDERING_DATA_H
#define RENDERING_DATA_H

#include <array>
#include <cstdint>

#include "BaseData.h"

/**
 * @brief RenderingData singleton
 *  - Consumes BaseData::FrameData and ImageData
 *  - Produces a Qt-friendly payload describing what to draw for the current display type.
 *
 * 사용처:
 *  - UI 스레드에서 composeFrame()을 호출 → RenderPayload를 받아서 QWidget/QML 등에 표시.
 */
class RenderingData {
public:
    enum class DisplayType : uint8_t {
        Default = 0,     // 기본 배경만
        SignAlert = 1,   // 표지판 이미지
        Warning = 2,     // 경고 아이콘
        Emotion = 3,     // 운전자 감정 GIF
        Dashboard = 4,   // 점수 + 필요한 아이콘
        ScoreBoard = 5,  // 총점/티어 화면
        COUNT
    };

    struct RenderPayload {
        DisplayType displayType;

        // Qt assets (nullptr이면 해당 항목 없음)
        const QPixmap* background = nullptr;
        const QPixmap* signImage = nullptr;
        const QPixmap* warningIcon = nullptr;
        QMovie* emotionGif = nullptr;
        const QPixmap* tierImage = nullptr;

        // Derived typed info (유효하지 않으면 *_Type::NONE)
        ImageData::SignType signType = ImageData::SignType::NONE;
        ImageData::WarningIconType warningType = ImageData::WarningIconType::NONE;
        ImageData::EmotionGifType emotionType = ImageData::EmotionGifType::HAPPY;
        ImageData::TierType tierType = ImageData::TierType::BRONZE;

        // User scores
        uint16_t userTotalScore = 0;
        std::array<uint8_t, static_cast<size_t>(UserData::ScoreType::MAX_SCORE_TYPES)> curScores{};

        // Raw signals (그대로 UI에 보여주거나 디버깅용)
        uint8_t rawSignSignal = 0;
        uint8_t rawWarningSignal = 0;
        uint8_t rawEmotionSignal = 0;
    };

    static RenderingData& getInstance();

    // BaseData의 현재 FrameData를 읽어 RenderPayload로 변환
    RenderPayload composeFrame();

private:
    RenderingData(BaseData& baseData, ImageData& imageData);
    ~RenderingData() = default;
    RenderingData(const RenderingData&) = delete;
    RenderingData& operator=(const RenderingData&) = delete;

    static DisplayType toDisplayType(uint8_t raw);
    static ImageData::SignType toSignType(uint8_t raw);
    static ImageData::WarningIconType toWarningType(uint8_t raw);
    static ImageData::EmotionGifType toEmotionType(uint8_t raw);
    static ImageData::TierType toTierType(uint16_t score);

private:
    BaseData& baseData;
    ImageData& imageData;
};

#endif // RENDERING_DATA_H
