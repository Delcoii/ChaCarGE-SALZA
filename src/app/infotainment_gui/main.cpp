#include "main.h"
#include "UserData.h"
#include "ImageData.h"
#include "BaseData.h"
#include "RenderingData.h"
#include "InfotainmentWidget.h"
#include "AppController.h"
#include "Common.h"

#include <QApplication>
#include <QDir>
#include <QTimer>
#include <QObject>
#include <thread>
#include <chrono>
#include <atomic>

namespace {
QString makeAbsolutePath(const QString& relative) {
    QDir dir(QCoreApplication::applicationDirPath());
    return dir.absoluteFilePath(relative);
}
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    UserData& userData = UserData::getInstance();
    const std::string userDataPath = makeAbsolutePath("../../../resources/userdata/user_data.json").toStdString();
    if (!userData.loadFromJsonFile(userDataPath)) {
        std::cerr << "Failed to load user data from JSON file." << std::endl;
        return -1;
    }

    ImageData& imageData = ImageData::getInstance();
    const QString assetBasePath = makeAbsolutePath("../../../resources/images/");
    AppController::loadAssets(imageData, assetBasePath);

    BaseData& baseData = BaseData::getInstance();
    RenderingData& renderingData = RenderingData::getInstance();

    InfotainmentWidget window(imageData, baseData, renderingData);
    window.setWindowTitle("Infotainment Layout Demo");
    window.resize(WindowSize::WIDTH, WindowSize::HEIGHT);

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
