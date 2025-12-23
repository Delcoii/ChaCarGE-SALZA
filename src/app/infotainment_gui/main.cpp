#include "main.h"
#include "UserData.h"
#include "ImageData.h"
#include "BaseData.h"
#include "RenderingData.h"
#include "InfotainmentWidget.h"
#include "AppController.h"

#include <QApplication>
#include <QTimer>
#include <QObject>
#include <thread>
#include <chrono>
#include <atomic>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    UserData& userData = UserData::getInstance();
    const std::string userDataPath = "../../../resources/userdata/user_data.json";
    if (!userData.loadFromJsonFile(userDataPath)) {
        std::cerr << "Failed to load user data from JSON file." << std::endl;
        return -1;
    }

    ImageData& imageData = ImageData::getInstance();
    AppController::loadAssets(imageData);

    BaseData& baseData = BaseData::getInstance();
    RenderingData& renderingData = RenderingData::getInstance();

    InfotainmentWidget window(imageData, baseData, renderingData);
    window.setWindowTitle("Infotainment Layout Demo");

    // Periodic saver for user data
    std::atomic<bool> keepSaving{true};
    std::thread saver([&]() {
        while (keepSaving.load()) {
            userData.saveToJsonFile(userDataPath);
            for (int i = 0; i < 50 && keepSaving.load(); ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    });

    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&]() {
        keepSaving.store(false);
        userData.saveToJsonFile(userDataPath);
    });

    AppController controller(baseData, renderingData, window);
    controller.start();

    window.show();
    const int ret = app.exec();

    keepSaving.store(false);
    if (saver.joinable()) saver.join();
    userData.saveToJsonFile(userDataPath);
    controller.join();
    return ret;
}
