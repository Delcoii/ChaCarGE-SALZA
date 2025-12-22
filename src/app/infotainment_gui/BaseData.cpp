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
