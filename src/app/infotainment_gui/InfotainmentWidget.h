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

private:
    const QPixmap* signalPix;
    const QPixmap* bumpPix;
    const QPixmap* regalPix;
    const QPixmap* overPix;
    const QPixmap* turnPix;
    QMovie* gif;
    const QPixmap* centerOverride = nullptr;
    const QPixmap* tierPix = nullptr;

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
    QPixmap diamondFilled;
    QPixmap diamondEmpty;
};

#endif // INFOTAINMENT_WIDGET_H
