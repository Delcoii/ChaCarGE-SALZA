#ifndef BASE_DATA_H
#define BASE_DATA_H
#include <cstdint>
#include <mutex>
#include "UserData.h"
#include "ImageData.h"

/**
 * @brief BaseData singleton class definition
 * 책임
 *  - Read Data ( shared memory + UserData + ImageData )
 *  - Save Data ( at this class )
 *  - 저장된 1개의 Frame 데이터를 ComposeDisplay에 제공한다. ( Send )
 */

class BaseData {
public:
    struct FrameData {
        // Shared Memory Data
        uint8_t signSignal;   // 교통 표지판 신호
        uint8_t warningSignal;
        uint8_t emotion;

        // UserData
        UserData& userData;  // user data reference

        // ImageData
        ImageData& imageData;

        // current frame data
        uint8_t curDisplayType;
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
    void setFrameSignals(uint8_t signSignal, uint8_t warningSignal, uint8_t emotion, uint8_t displayType);
    uint8_t getCurDisplayType() const;

private:
    FrameData curFrameData;
    mutable std::mutex mtx;
};

#endif // BASE_DATA_H
