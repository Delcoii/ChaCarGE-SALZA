#include "BaseData.h"

BaseData::BaseData()
    : curFrameData{
        {0.0, 0.0, 0.0, 0}, // rawData
        0xFF, // warningSignal (invalid sentinel so no warning shows initially)
        0, // emotion
        0, // scoreDirection
        UserData::getInstance(), // userData reference
        ImageData::getInstance(), // imageData reference
        4  // curDisplayType (default to dashboard view)
    }
{
}

BaseData& BaseData::getInstance(){
    static BaseData instance;
    return instance;
}

BaseData::FrameData BaseData::getFrameDataCopy() const {
    std::lock_guard<std::mutex> lock(mtx);
    return curFrameData;
}

void BaseData::setFrameSignals(const RawData& rawData, uint8_t warningSignal, uint8_t emotion, uint8_t scoreDirection, uint8_t displayType) {
    std::lock_guard<std::mutex> lock(mtx);
    curFrameData.rawData = rawData;
    curFrameData.warningSignal = warningSignal;
    curFrameData.emotion = emotion;
    curFrameData.scoreDirection = scoreDirection;
    curFrameData.curDisplayType = displayType;
}

uint8_t BaseData::getCurDisplayType() const {
    std::lock_guard<std::mutex> lock(mtx);
    return curFrameData.curDisplayType;
}

void BaseData::setDisplayType(uint8_t displayType) {
    std::lock_guard<std::mutex> lock(mtx);
    curFrameData.curDisplayType = displayType;
}
