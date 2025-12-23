#include "BaseData.h"

BaseData::BaseData()
    : curFrameData{
        0, // signSignal
        0, // warningSignal
        0, // emotion
        UserData::getInstance(), // userData reference
        ImageData::getInstance(), // imageData reference
        0  // curDisplayType
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

void BaseData::setFrameSignals(uint8_t signSignal, uint8_t warningSignal, uint8_t emotion, uint8_t displayType) {
    std::lock_guard<std::mutex> lock(mtx);
    curFrameData.signSignal = signSignal;
    curFrameData.warningSignal = warningSignal;
    curFrameData.emotion = emotion;
    curFrameData.curDisplayType = displayType;
}

uint8_t BaseData::getCurDisplayType() const {
    std::lock_guard<std::mutex> lock(mtx);
    return curFrameData.curDisplayType;
}
