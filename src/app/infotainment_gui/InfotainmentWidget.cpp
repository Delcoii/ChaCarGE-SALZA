#include "InfotainmentWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QStackedLayout>
#include <QPushButton>
#include <QProgressBar>
#include <QPainter>
#include <QColor>

InfotainmentWidget::InfotainmentWidget(ImageData& images, BaseData& base, RenderingData& rendering, QWidget* parent)
    : QWidget(parent)
    , signalPix(images.getDefaultImage(ImageData::DefaultImageType::SIGNAL_CORNER))
    , bumpPix(images.getSignImage(ImageData::SignType::BUMP))
    , regalPix(images.getSignImage(ImageData::SignType::SIGNAL_VIOLATION))
    , overPix(images.getSignImage(ImageData::SignType::OVERSPEED))
    , turnPix(images.getSignImage(ImageData::SignType::OVERTURN))
    , gif(images.getEmotionGif(ImageData::EmotionGifType::HAPPY))
    , baseData(base)
    , renderingData(rendering)
{
    setMinimumSize(900, 540);
    setStyleSheet("background: #0f1115; color: #e7e9ec; font-family: 'Helvetica Neue', Arial;");

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(16);

    auto* header = new QHBoxLayout();
    header->setSpacing(12);

    // Username above signal icon
    auto* nameSignalBox = new QVBoxLayout();
    nameSignalBox->setSpacing(6);
    nameSignalBox->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    headerTitle = new QLabel("Driver", this);
    headerTitle->setAlignment(Qt::AlignCenter);
    headerTitle->setStyleSheet("font-size: 22px; font-weight: 800; color: #5dd1ff;");
    signalLabel = createImageLabel(1, 1);
    signalLabel->setFixedSize(144, 144); // 1.5x previous size
    nameSignalBox->addWidget(headerTitle, 0, Qt::AlignCenter);
    nameSignalBox->addWidget(signalLabel, 0, Qt::AlignCenter);
    header->addLayout(nameSignalBox, 0);

    header->addStretch();
    root->addLayout(header);

    stack = new QStackedLayout();

    mainContainer = new QWidget(this);
    auto* body = new QHBoxLayout(mainContainer);
    body->setSpacing(20);

    auto* leftCol = new QVBoxLayout();
    leftCol->setSpacing(12);
    bumpLabel = createImageLabel(160, 160);
    regalLabel = createImageLabel(160, 160);
    leftCol->addWidget(bumpLabel, 0, Qt::AlignCenter);
    leftCol->addWidget(regalLabel, 0, Qt::AlignCenter);
    leftCol->addStretch();

    auto* centerCol = new QVBoxLayout();
    centerCol->setSpacing(8);
    centerCol->setAlignment(Qt::AlignCenter);

    auto* diamondRow = new QHBoxLayout();
    diamondRow->setSpacing(4);
    diamondRow->setAlignment(Qt::AlignCenter);
    diamondLabels.reserve(20);
    for (int i = 0; i < 20; ++i) {
        auto* d = new QLabel(this);
        d->setFixedSize(18, 8);
        diamondLabels.push_back(d);
        diamondRow->addWidget(d);
    }
    centerCol->addLayout(diamondRow);

    gifLabel = createImageLabel(360, 360);
    gifLabel->setStyleSheet("background: #1a1d24; border: 1px solid #2a2f3a; border-radius: 12px;");
    centerCol->addWidget(gifLabel, 0, Qt::AlignCenter);

    auto* rightCol = new QVBoxLayout();
    rightCol->setSpacing(12);
    overLabel = createImageLabel(160, 160);
    turnLabel = createImageLabel(160, 160);
    rightCol->addWidget(overLabel, 0, Qt::AlignCenter);
    rightCol->addWidget(turnLabel, 0, Qt::AlignCenter);
    rightCol->addStretch();

    body->addLayout(leftCol, 1);
    body->addLayout(centerCol, 2);
    body->addLayout(rightCol, 1);
    stack->addWidget(mainContainer);

    // Score container
    scoreContainer = new QWidget(this);
    auto* scoreLayout = new QHBoxLayout(scoreContainer);
    scoreLayout->setSpacing(20);

    auto* scoreLeft = new QVBoxLayout();
    scoreLeft->setSpacing(12);
    scoreValueLabel = createImageLabel(200, 200);
    scoreValueLabel->setStyleSheet("font-size: 240px; font-weight: 900; color: #5dd1ff;"); // ~5x bigger
    scoreLeft->addWidget(scoreValueLabel, 1, Qt::AlignCenter);
    scoreLayout->addLayout(scoreLeft, 1);

    tierLabel = createImageLabel(200, 200);
    tierLabel->setStyleSheet("background: #1a1d24; border: 1px solid #2a2f3a; border-radius: 12px;");
    scoreLayout->addWidget(tierLabel, 1, Qt::AlignCenter);

    stack->addWidget(scoreContainer);

    // Detail container (curScores)
    detailContainer = new QWidget(this);
    auto* detailLayout = new QVBoxLayout(detailContainer);
    detailLayout->setSpacing(10);
    detailLayout->setContentsMargins(12, 12, 12, 12);
    for (size_t i = 0; i < static_cast<size_t>(UserData::ScoreType::MAX_SCORE_TYPES); ++i) {
        auto* row = new QHBoxLayout();
        row->setSpacing(12);
        auto* name = new QLabel(this);
        name->setStyleSheet("font-size: 14px; color: #cfd2d8;");
        auto* bar = new QProgressBar(this);
        bar->setRange(0, 100);
        bar->setTextVisible(false);
        bar->setFixedHeight(16);
        bar->setStyleSheet("QProgressBar { background: #1e222b; border-radius: 8px; }"
                           "QProgressBar::chunk { background: #5dd1ff; border-radius: 8px; }");
        auto* val = new QLabel(this);
        val->setStyleSheet("font-size: 14px; color: #e7e9ec;");
        row->addWidget(name, 1);
        row->addWidget(bar, 3);
        row->addWidget(val, 0, Qt::AlignRight);
        detailLayout->addLayout(row);
        detailRows.push_back({name, bar, val});
    }
    detailLayout->addStretch();
    stack->addWidget(detailContainer);

    root->addLayout(stack, 1);

    // Toggle button (bottom-right)
    toggleBtn = new QPushButton("Toggle View", this);
    toggleBtn->setStyleSheet("background: #5dd1ff; color: #0f1115; font-weight: 700; padding: 10px 14px; border-radius: 10px;");
    connect(toggleBtn, &QPushButton::clicked, this, &InfotainmentWidget::toggleDisplayType);
    root->addWidget(toggleBtn, 0, Qt::AlignRight);

    RenderingData::RenderPayload initPayload{};
    initPayload.userTotalScore = BaseData::getInstance().getFrameDataCopy().userData.getUserTotalScore();
    applyImages(initPayload);
}

InfotainmentWidget::~InfotainmentWidget() = default;

void InfotainmentWidget::showFrame(const RenderingData::RenderPayload& payload) {
    // Update username from UserData (via payload.userTotalScore owner)
    headerTitle->setText(QString::fromStdString(BaseData::getInstance().getFrameDataCopy().userData.getUsername()));

    if (payload.emotionGif) {
        gif = payload.emotionGif;
    }
    centerOverride = payload.signImage;
    tierPix = payload.tierImage;

    if (payload.displayType == RenderingData::DisplayType::ScoreBoard) {
        stack->setCurrentWidget(scoreContainer);
        applyScoreView(payload);
    } else if (payload.displayType == RenderingData::DisplayType::Emotion) {
        stack->setCurrentWidget(detailContainer);
        applyDetailView(payload);
    } else {
        stack->setCurrentWidget(mainContainer);
        applyImages(payload);
    }
}

QLabel* InfotainmentWidget::createImageLabel(int minW, int minH) {
    auto* lbl = new QLabel(this);
    lbl->setMinimumSize(minW, minH);
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setScaledContents(true);
    lbl->setStyleSheet("background: transparent;");
    return lbl;
}

void InfotainmentWidget::setPixmapToLabel(QLabel* label, const QPixmap* pix, int w, int h, const char* fallback) {
    if (pix) {
        label->setPixmap(pix->scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        label->setText(fallback);
    }
}

namespace {
QPixmap makeSegmentPixmap(const QColor& fill, const QColor& border) {
    const int w = 18;
    const int h = 8;
    QPixmap pm(w, h);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    if (fill.alpha() > 0) {
        p.setBrush(fill);
    }
    QPen pen(border, 1.0);
    p.setPen(pen);
    p.drawRoundedRect(QRect(1, 1, w - 2, h - 2), 2, 2);
    return pm;
}
} // namespace

void InfotainmentWidget::applyImages(const RenderingData::RenderPayload& payload) {
    const QSize sigSize(96, 96);
    setPixmapToLabel(signalLabel, signalPix, sigSize.width(), sigSize.height(), "Signal?");

    const int centerSize = std::max(240, std::min(width(), height()) / 3);
    gifLabel->setMinimumSize(centerSize, centerSize);
    const QPixmap* centerPix = centerOverride ? centerOverride : signalPix;
    if (gif) {
        gif->setScaledSize(QSize(centerSize, centerSize));
        gifLabel->setMovie(gif);
        gif->start();
    } else if (centerPix) {
        setPixmapToLabel(gifLabel, centerPix, centerSize, centerSize, "Center?");
    } else {
        gifLabel->setText("GIF missing");
    }

    const int sideSize = centerSize / 2;
    setPixmapToLabel(bumpLabel, bumpPix, sideSize, sideSize, "bump?");
    setPixmapToLabel(regalLabel, regalPix, sideSize, sideSize, "regal?");
    setPixmapToLabel(overLabel, overPix, sideSize, sideSize, "over?");
    setPixmapToLabel(turnLabel, turnPix, sideSize, sideSize, "turn?");

    updateDiamondGauge(payload.userTotalScore);
}

void InfotainmentWidget::applyScoreView(const RenderingData::RenderPayload& payload) {
    scoreValueLabel->setText(QString::number(payload.userTotalScore));

    const int tierSize = std::max(200, std::min(width(), height()) / 2 - 40);
    if (tierPix) {
        setPixmapToLabel(tierLabel, tierPix, tierSize, tierSize, "Tier?");
    } else {
        tierLabel->setText("Tier?");
    }
}

void InfotainmentWidget::applyDetailView(const RenderingData::RenderPayload& payload) {
    static const char* kNames[] = {
        "Speed Bumps", "Rapid Accel", "Sharp Turns", "Signal Violations"
    };
    const auto maxIdx = std::min(detailRows.size(), static_cast<size_t>(payload.curScores.size()));
    for (size_t i = 0; i < maxIdx; ++i) {
        detailRows[i].name->setText(kNames[i]);
        detailRows[i].bar->setValue(payload.curScores[i]);
        detailRows[i].value->setText(QString::number(payload.curScores[i]));
    }
}

void InfotainmentWidget::toggleDisplayType() {
    auto frame = baseData.getFrameDataCopy();
    uint8_t nextDisplay;
    if (frame.curDisplayType == static_cast<uint8_t>(RenderingData::DisplayType::Dashboard)) {
        nextDisplay = static_cast<uint8_t>(RenderingData::DisplayType::ScoreBoard);
    } else if (frame.curDisplayType == static_cast<uint8_t>(RenderingData::DisplayType::ScoreBoard)) {
        nextDisplay = static_cast<uint8_t>(RenderingData::DisplayType::Emotion); // reuse for detail page
    } else {
        nextDisplay = static_cast<uint8_t>(RenderingData::DisplayType::Dashboard);
    }
    baseData.setFrameSignals(frame.signSignal, frame.warningSignal, frame.emotion, nextDisplay);
    // Immediate UI refresh to reflect the new mode without waiting on the renderer loop
    showFrame(renderingData.composeFrame());
}

void InfotainmentWidget::updateDiamondGauge(uint16_t score) {
    if (diamondFilled.isNull()) {
        diamondFilled = makeSegmentPixmap(QColor("#2ecc71"), QColor("#1f8a4d"));
    }
    if (diamondEmpty.isNull()) {
        diamondEmpty = makeSegmentPixmap(QColor("#4a4f5a"), QColor("#2a2f3a"));
    }
    const int filled = std::max(0, std::min(20, static_cast<int>(score / 5)));
    for (int i = 0; i < static_cast<int>(diamondLabels.size()); ++i) {
        if (i < filled) diamondLabels[i]->setPixmap(diamondFilled);
        else diamondLabels[i]->setPixmap(diamondEmpty);
    }
}
