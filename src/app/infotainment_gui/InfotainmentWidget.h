#ifndef INFOTAINMENT_WIDGET_H
#define INFOTAINMENT_WIDGET_H

#include <QWidget>
#include <QElapsedTimer>
#include <vector>
#include "RenderingData.h"
#include "ImageData.h"
#include "ShmCompat.h"

class QLabel;
class QProgressBar;
class QStackedLayout;
class QHBoxLayout;
class QPushButton;
class QSpacerItem;

class InfotainmentWidget : public QWidget {
public:
    explicit InfotainmentWidget(ImageData& images, BaseData& base, RenderingData& rendering, QWidget* parent = nullptr);
    ~InfotainmentWidget() override;
    void showFrame(const RenderingData::RenderPayload& payload);

private:
    QLabel* createImageLabel(int minW, int minH);
    void setPixmapToLabel(QLabel* label, const QPixmap* pix, int w, int h, const char* fallback);
    void applyImages(const RenderingData::RenderPayload& payload);
    void applyScoreView(const RenderingData::RenderPayload& payload);
    void applyDetailView(const RenderingData::RenderPayload& payload);
    void applyHistoryView(const RenderingData::RenderPayload& payload);
    void toggleDisplayType();
    void updateDiamondGauge(uint16_t score);
    void setGifMovie(QMovie* movie, int size);
    const QPixmap* violationPixmap(uint8_t scoreType) const;

private:
    const QPixmap* tierPix = nullptr;

    ImageData& imageData;
    BaseData& baseData;
    RenderingData& renderingData;

    QWidget* mainContainer = nullptr;
    QWidget* scoreContainer = nullptr;
    QWidget* detailContainer = nullptr;
    QWidget* historyContainer = nullptr;
    class QStackedLayout* stack = nullptr;
    class QHBoxLayout* contentRow = nullptr;
    class QSpacerItem* topSpacer = nullptr;

    QLabel* signalLabel = nullptr;
    QLabel* headerTitle = nullptr;
    QLabel* onOffLabel = nullptr;
    QLabel* gifLabel = nullptr;
    QLabel* timeLabel = nullptr;
    QLabel* dateLabel = nullptr;
    QLabel* scoreValueLabel = nullptr;
    QLabel* tierLabel = nullptr;
    class QPushButton* toggleBtn = nullptr;
    class QFrame* centerDivider = nullptr;
    QLabel* historyEmptyLabel = nullptr;
    class QGraphicsView* historyView = nullptr;
    class QGraphicsScene* historyScene = nullptr;
    struct DetailRow { QLabel* name; QProgressBar* bar; QLabel* value; };
    std::vector<DetailRow> detailRows;
    std::vector<QLabel*> diamondLabels;
    QPixmap diamondFilled;
    QPixmap diamondEmpty;
    QPixmap diamondRed;
    int sideSizePx = 32;
    QLabel* warningLabel = nullptr;
    QPixmap warningEmpty;
    QMovie* happyGif = nullptr;
    QMovie* badGif = nullptr;
    QMovie* currentGif = nullptr;
    ScoreDirection activeDirection = ScoreDirection::SCORE_NORMAL;
    QElapsedTimer directionTimer;
    int directionShowMs = 3000;
    QElapsedTimer warningTimer;
    int warningShowMs = 3000;
    int lastWarningSignal = -1;
    bool warningActive = false;
    const QPixmap* activeWarningPixmap = nullptr;
    int lastDisplayType = -1;
    bool forceApplyOnNextPayload = false;
    int gifSizePx = 0;
    int lastFilled = 0;
    QProgressBar* throttleBar = nullptr;
    QProgressBar* brakeBar = nullptr;
    QLabel* throttleLabel = nullptr;
    QLabel* brakeLabel = nullptr;
    QLabel* steeringLabel = nullptr;
    QLabel* steeringTextLabel = nullptr;
    QWidget* gaugeContainer = nullptr;
    class QHBoxLayout* gaugeRow = nullptr;
    const QPixmap* steeringWheelPix = nullptr;
};

#endif // INFOTAINMENT_WIDGET_H
