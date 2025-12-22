#include "BaseData.h"

BaseData& BaseData::getInstance(){
    static BaseData instance;
    return instance;
}

