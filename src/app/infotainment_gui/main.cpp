#include "main.h"
#include "UserData.h"

int main() {
    // 1. Get User Data
    UserData& userData = UserData::getInstance();
    if (!userData.loadFromJsonFile("user_data.json")) {
        std::cerr << "Failed to load user data from JSON file." << std::endl;
        return -1;
    }

    // 2. Shared Memory
    // 3. BasaData Instance
    
    // 4. ImageData Instance

    // Create BaseData Thread
    // Create ComposeDisplay Thread

    // Destroy BaseData Thread
    // Destroy ImageData Thread

    // Save User Data ( Using destructor )
    return 0;
}