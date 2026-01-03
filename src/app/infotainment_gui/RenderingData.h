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
 * Usage:
 *  - The UI thread calls composeFrame() -> receives RenderPayload to display in QWidget/QML, etc.
 */
class RenderingData {
public:
    enum class DisplayType : uint8_t {
        Default = 0,     // Background only
        SignAlert = 1,   // Traffic sign image
        Warning = 2,     // Warning icon
        Emotion = 3,     // Driver emotion GIF
        Dashboard = 4,   // Scores plus needed icons
        ScoreBoard = 5,  // Total score/tier screen
        COUNT
    };

    struct RenderPayload {
        DisplayType displayType;

        // Qt assets (nullptr means the item is unavailable)
        const QPixmap* background = nullptr;
        const QPixmap* signImage = nullptr;
        const QPixmap* warningIcon = nullptr;
        QMovie* emotionGif = nullptr;
        const QPixmap* tierImage = nullptr;
        const QPixmap* steeringWheelImage = nullptr;

        // Derived typed info (if invalid, *_Type::NONE)
        ImageData::SignType signType = ImageData::SignType::NONE;
        ImageData::WarningIconType warningType = ImageData::WarningIconType::NONE;
        ImageData::EmotionGifType emotionType = ImageData::EmotionGifType::HAPPY;
        ImageData::TierType tierType = ImageData::TierType::BRONZE;

        // User scores
        uint16_t userTotalScore = 0;
        std::array<uint8_t, static_cast<size_t>(UserData::ScoreType::MAX_SCORE_TYPES)> curScores{};

        // Raw signals (for direct UI display or debugging)
        double throttle = 0.0;
        double brake = 0.0;
        double steerAngleDeg = 0.0;
        uint8_t rawSignSignal = 0;
        uint8_t rawWarningSignal = 0;
        uint8_t rawEmotionSignal = 0;
    };

    static RenderingData& getInstance();

    // Convert the current FrameData from BaseData into a RenderPayload
    void composeFrame(RenderPayload& out);
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
