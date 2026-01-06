#ifndef BASE_DATA_H
#define BASE_DATA_H
#include <cstdint>
#include <mutex>
#include <vector>
#include "UserData.h"
#include "ImageData.h"

/**
 * @brief BaseData singleton class definition
 * Responsibilities:
 *  - Read Data (shared memory + UserData + ImageData)
 *  - Save Data (within this class)
 *  - Provide the single stored Frame to ComposeDisplay (Send)
 */

class BaseData {
public:
    struct ViolationEvent {
        uint8_t scoreType;
        int64_t timestampMs;
    };

    struct RawData {
        double throttle;
        double brake;
        double steerTireDegree;
        uint8_t signSignal;   // Traffic sign signal // 0:NONE, 1:RED, 2:YELLOW, 3:GREEN
    };

    struct FrameData {
        // Shared Memory for raw data
        RawData rawData;   // Traffic sign signal + vehicle command

        // Shared Memory for generated data
        uint8_t warningSignal;
        
        uint8_t emotion;
        uint8_t scoreDirection;

        // UserData
        UserData& userData;  // user data reference

        // ImageData
        ImageData& imageData;

        // current frame data
        uint8_t curDisplayType;
        bool useDrivingScoreCheck;
        std::vector<ViolationEvent> violations;
        int64_t sessionStartMs;
        int64_t sessionEndMs;
    };
private:
    BaseData();
    ~BaseData() = default;
    BaseData(const BaseData&) = delete;
    BaseData& operator=(const BaseData&) = delete;  
public:
    static BaseData& getInstance();

    // Note: Read-only accessor for the current frame snapshot.
    // Later, producer thread can update curFrameData via dedicated setters.
    FrameData getFrameDataCopy() const;

    // Temporary setter for prototyping/demo; replace with thread-safe updater later.
    void setFrameSignals(const RawData& rawData,
                         uint8_t warningSignal,
                         uint8_t emotion,
                         uint8_t scoreDirection,
                         uint8_t displayType,
                         bool useDrivingScoreCheck,
                         std::vector<ViolationEvent> violations,
                         int64_t sessionStartMs,
                         int64_t sessionEndMs);
    void setDisplayType(uint8_t displayType);
    uint8_t getCurDisplayType() const;

private:
    FrameData curFrameData;
    mutable std::mutex mtx;
};

#endif // BASE_DATA_H
