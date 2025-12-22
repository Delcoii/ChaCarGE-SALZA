#ifndef BASE_DATA_H
#define BASE_DATA_H

/**
 * @brief BaseData singleton class definition
 * 책임
 *  - 공유 메모리에서 데이터를 가지고온다. ( Read )
 *  - 가지고온 데이터를 저장한다. ( Save )
 *  - 저장된 1개의 Frame 데이터를 ComposeDisplay에 제공한다. ( Send )
 */

class BaseData {
private:
    BaseData();
    ~BaseData() = default;
    BaseData(const BaseData&) = delete;
    BaseData& operator=(const BaseData&) = delete;  
public:
    static BaseData& getInstance();
public:
private:
    
};

#endif // BASE_DATA_H