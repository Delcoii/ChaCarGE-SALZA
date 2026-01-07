#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include <atomic>
#include <array>
#include <chrono>
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
    void requestToggleDrivingCheck();

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
    std::atomic<int> pendingToggle{0};
    bool lastUseDrivingCheck = false;
    uint16_t lastRawScoreType = 0xFFFF; // track changes while driving score check is on
    uint16_t lastEmittedType = 0xFFFF;
    uint16_t lastEmittedCount = 0;
    uint16_t activeWarningType = 0xFFFF;
    uint16_t activeWarningCount = 0;
    std::chrono::steady_clock::time_point warningExpiry{};
    int64_t sessionStartMs = -1;
    int64_t sessionEndMs = -1;
    std::vector<BaseData::ViolationEvent> activeViolations;
    std::array<uint16_t, static_cast<size_t>(ScoreType::SCORE_TYPE_NONE) + 1> lastScoreCounts{};
    std::array<uint16_t, static_cast<size_t>(ScoreType::SCORE_TYPE_NONE) + 1> typeChangeCounts{};

    static constexpr int kRenderBufferCount = 3;
    std::array<RenderingData::RenderPayload, kRenderBufferCount> renderBuffers{};
    std::array<std::atomic<bool>, kRenderBufferCount> bufferInUse{{false, false, false}};
    int nextWriteIndex = 0;
};

#endif // APP_CONTROLLER_H
