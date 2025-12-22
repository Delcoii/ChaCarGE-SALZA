#include "main.h"
#include "UserData.h"
#include "ImageData.h"
#include "BaseData.h"

int main() {
    // 1. Get User Data
    UserData& userData = UserData::getInstance();
    if (!userData.loadFromJsonFile("user_data.json")) {
        std::cerr << "Failed to load user data from JSON file." << std::endl;
        return -1;
    }

    // 2. Shared Memory
    // 3. ImageData Instance
    ImageData& imageData = ImageData::getInstance();

    // 4. BaseData Instance
    BaseData& baseData = BaseData::getInstance();


    // Create BaseData Thread
    // Create ComposeDisplay Thread

    // Destroy BaseData Thread
    // Destroy ImageData Thread

    // Save User Data ( Using destructor )
    return 0;
}