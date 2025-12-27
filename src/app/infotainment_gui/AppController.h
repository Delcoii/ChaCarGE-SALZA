#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include <thread>
#include <atomic>

#include "BaseData.h"
#include "RenderingData.h"
#include "InfotainmentWidget.h"
#include "ImageData.h"
#include "ShmCompat.h"

class AppController {
public:
    AppController(BaseData& base, RenderingData& rendering, InfotainmentWidget& ui);
    ~AppController();

    void start();
    void stop();
    void join();

    static void loadAssets(ImageData& imageData);

private:
    void producerLoop();
    void rendererLoop();

    BaseData& baseData;
    RenderingData& renderingData;
    InfotainmentWidget& ui;

    std::thread producerThread;
    std::thread rendererThread;
    std::atomic<bool> running{false};
    ShmIntegrated* shmPtr = nullptr;
};

#endif // APP_CONTROLLER_H
