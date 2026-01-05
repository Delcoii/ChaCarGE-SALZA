#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include <atomic>
#include <array>
#include <thread>
#include <vector>
#include <QString>

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

    static void loadAssets(ImageData& imageData, const QString& assetBasePath);

private:
    void producerLoop();
    void rendererLoop();
    int acquireRenderBuffer();
    void releaseRenderBuffer(int idx);

    // Latest produced buffer index; renderer picks it up and resets to -1
    std::atomic<int> latestReady{-1};

    BaseData& baseData;
    RenderingData& renderingData;
    InfotainmentWidget& ui;

    std::thread producerThread;
    std::thread rendererThread;
    std::atomic<bool> running{false};
    ShmIntegrated* shmPtr = nullptr;
    bool lastUseDrivingCheck = false;
    std::vector<BaseData::ViolationEvent> activeViolations;
    uint8_t lastMinusType = 0xFF;

    static constexpr int kRenderBufferCount = 3;
    std::array<RenderingData::RenderPayload, kRenderBufferCount> renderBuffers{};
    std::array<std::atomic<bool>, kRenderBufferCount> bufferInUse{{false, false, false}};
    int nextWriteIndex = 0;
};

#endif // APP_CONTROLLER_H
