#ifndef INFOTAINMENT_WIDGET_H
#define INFOTAINMENT_WIDGET_H

#include <QWidget>
#include <vector>
#include "RenderingData.h"
#include "ImageData.h"

class QLabel;
class QProgressBar;

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
    void toggleDisplayType();
    void updateDiamondGauge(uint16_t score);
    void setGifMovie(QMovie* movie, int size);

private:
    const QPixmap* signalPix;
    const QPixmap* bumpPix;
    const QPixmap* regalPix;
    const QPixmap* overPix;
    const QPixmap* turnPix;
    QMovie* gif;
    const QPixmap* centerOverride = nullptr;
    const QPixmap* tierPix = nullptr;

    ImageData& imageData;
    BaseData& baseData;
    RenderingData& renderingData;

    QWidget* mainContainer = nullptr;
    QWidget* scoreContainer = nullptr;
    QWidget* detailContainer = nullptr;
    class QStackedLayout* stack = nullptr;

    QLabel* signalLabel = nullptr;
    QLabel* headerTitle = nullptr;
    QLabel* headerSubtitle = nullptr;
    QLabel* gifLabel = nullptr;
    QLabel* bumpLabel = nullptr;
    QLabel* regalLabel = nullptr;
    QLabel* overLabel = nullptr;
    QLabel* turnLabel = nullptr;
    QLabel* scoreValueLabel = nullptr;
    QLabel* tierLabel = nullptr;
    class QPushButton* toggleBtn = nullptr;
    struct DetailRow { QLabel* name; QProgressBar* bar; QLabel* value; };
    std::vector<DetailRow> detailRows;
    std::vector<QLabel*> diamondLabels;
    QLabel* gifScoreLabel = nullptr;
    QPixmap diamondFilled;
    QPixmap diamondEmpty;
    QPixmap diamondRed;
    bool gifAttached = false;
    int sideSizePx = 32;
    QLabel* warningLabel = nullptr;
    QPixmap warningEmpty;
    QMovie* happyGif = nullptr;
    QMovie* badGif = nullptr;
    QMovie* currentGif = nullptr;
    int gifSizePx = 0;
    int lastFilled = 0;
    QProgressBar* throttleBar = nullptr;
    QProgressBar* brakeBar = nullptr;
    QLabel* throttleLabel = nullptr;
    QLabel* brakeLabel = nullptr;
    QLabel* steeringLabel = nullptr;
    QLabel* steeringTextLabel = nullptr;
    const QPixmap* steeringWheelPix = nullptr;
};

#endif // INFOTAINMENT_WIDGET_H
