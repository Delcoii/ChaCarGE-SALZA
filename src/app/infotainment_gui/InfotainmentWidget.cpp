#include "InfotainmentWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QStackedLayout>
#include <QPushButton>
#include <QProgressBar>
#include <QPainter>
#include <QTransform>
#include <QColor>
#include <QFrame>
#include <QSpacerItem>
#include <algorithm>
#include <QTimer>
#include <QDateTime>
#include <QTimeZone>
#include "UserData.h"
#include "Common.h"

InfotainmentWidget::InfotainmentWidget(ImageData& images, BaseData& base, RenderingData& rendering, QWidget* parent)
    : QWidget(parent)
    , imageData(images)
    , baseData(base)
    , renderingData(rendering)
{
    setMinimumSize(static_cast<int>(WindowSize::WIDTH), static_cast<int>(WindowSize::HEIGHT));
    setStyleSheet("background: #ffffff; color: #111111; font-family: 'Helvetica Neue', Arial;");

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 24); // lift bottom button
    root->setSpacing(16);

    auto* header = new QHBoxLayout();
    header->setSpacing(12);

    // Username above signal icon
    auto* nameSignalBox = new QVBoxLayout();
    nameSignalBox->setSpacing(6);
    nameSignalBox->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    headerTitle = new QLabel("Mr.Tele", this);
    headerTitle->setAlignment(Qt::AlignCenter);
    headerTitle->setStyleSheet("font-size: 22px; font-weight: 800; color: #0a4f91;");
    signalLabel = createImageLabel(1, 1);
    nameSignalBox->addWidget(headerTitle, 0, Qt::AlignCenter);
    nameSignalBox->addWidget(signalLabel, 0, Qt::AlignCenter);
    header->addLayout(nameSignalBox, 0);

    header->addStretch();
    root->addLayout(header);

    stack = new QStackedLayout();

    mainContainer = new QWidget(this);
    auto* body = new QVBoxLayout(mainContainer);
    body->setContentsMargins(0, 0, 0, 0);
    body->setSpacing(8);

    // Gauge only (no numeric score)
    auto* diamondRow = new QHBoxLayout();
    diamondRow->setSpacing(4);
    diamondRow->setAlignment(Qt::AlignLeft);
    diamondLabels.reserve(20);
    for (int i = 0; i < 20; ++i) {
        auto* d = new QLabel(this);
        d->setFixedSize(18, 16);
        diamondLabels.push_back(d);
        diamondRow->addWidget(d);
    }
    body->addLayout(diamondRow);
    topSpacer = new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding);
    body->addSpacerItem(topSpacer);

    // Content row: GIF left, warning centered in remaining space
    contentRow = new QHBoxLayout();
    contentRow->setContentsMargins(0, 0, 0, 0);
    contentRow->setSpacing(0);
    contentRow->setAlignment(Qt::AlignVCenter);

    gifLabel = createImageLabel(360, 360);
    gifLabel->setStyleSheet("background: transparent; border: none;");
    auto* gifLayout = new QVBoxLayout(gifLabel);
    gifLayout->setContentsMargins(8, 8, 8, 8);
    gifLayout->setSpacing(4);
    dateLabel = new QLabel(gifLabel);
    dateLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    dateLabel->setStyleSheet("color: #111111; font-size: 32px; font-weight: 700;");
    timeLabel = new QLabel(gifLabel);
    timeLabel->setAlignment(Qt::AlignCenter);
    // Dial back the clock size so it fits more comfortably inside the GIF frame
    timeLabel->setStyleSheet("color: #000000; font-size: 90px; font-weight: 800; font-stretch: 75;");
    gifLayout->addWidget(dateLabel, 0, Qt::AlignHCenter | Qt::AlignTop);
    gifLayout->addWidget(timeLabel, 1, Qt::AlignCenter);
    dateLabel->hide();
    timeLabel->hide();
    contentRow->addWidget(gifLabel, 0, Qt::AlignLeft | Qt::AlignVCenter);

    centerDivider = new QFrame(this);
    centerDivider->setFrameShape(QFrame::VLine);
    centerDivider->setFrameShadow(QFrame::Plain);
    centerDivider->setStyleSheet("color: #2a2f3a; background: #2a2f3a;");
    contentRow->addWidget(centerDivider, 0, Qt::AlignVCenter);

    warningLabel = createImageLabel(200, 200);
    warningLabel->setStyleSheet("background: transparent;");
    auto* warningWrap = new QHBoxLayout();
    warningWrap->setContentsMargins(0, 0, 0, 0);
    warningWrap->setSpacing(0);
    warningWrap->addStretch();
    warningWrap->addWidget(warningLabel, 0, Qt::AlignCenter);
    warningWrap->addStretch();
    auto* warningContainer = new QWidget(this);
    warningContainer->setLayout(warningWrap);
    contentRow->addWidget(warningContainer, 0, Qt::AlignLeft | Qt::AlignVCenter);

    body->addLayout(contentRow);
    body->setStretchFactor(contentRow, 1);

    // Gauges + toggle aligned at the bottom of the main view
    gaugeContainer = new QWidget(this);
    gaugeContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    gaugeRow = new QHBoxLayout(gaugeContainer);
    gaugeRow->setContentsMargins(0, 0, 0, 0);
    gaugeRow->setSpacing(12);

    struct GaugeWidgets {
        QProgressBar* bar;
        QLabel* label;
        QVBoxLayout* layout;
    };
    auto makeGauge = [this](const QString& name, const QString& chunkStyle) -> GaugeWidgets {
        auto* column = new QVBoxLayout();
        column->setSpacing(4);
        column->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);

        auto* bar = new QProgressBar(this);
        bar->setOrientation(Qt::Vertical);
        bar->setRange(0, 100);
        bar->setValue(0);
        bar->setFixedSize(32, 180);
        bar->setTextVisible(false);
        bar->setStyleSheet(
            "QProgressBar { background: #1e222b; border-radius: 8px; border: 1px solid #2a2f3a; }"
            "QProgressBar::chunk {" + chunkStyle + " border-radius: 8px; }");

        auto* lbl = new QLabel(name, this);
        lbl->setStyleSheet("font-size: 16px; font-weight: 700; color: #111111;");
        lbl->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

        column->addWidget(bar, 0, Qt::AlignHCenter | Qt::AlignBottom);
        column->addWidget(lbl, 0, Qt::AlignHCenter | Qt::AlignTop);
        return {bar, lbl, column};
    };

    auto accelGauge = makeGauge("A", " background: qlineargradient(x1:0, y1:1, x2:0, y2:0, stop:0 #1f8a4d, stop:1 #2ecc71);");
    throttleBar = accelGauge.bar;
    throttleLabel = accelGauge.label;

    auto brakeGauge = makeGauge("B", " background: qlineargradient(x1:0, y1:1, x2:0, y2:0, stop:0 #8a1f1f, stop:1 #ff4d4d);");
    brakeBar = brakeGauge.bar;
    brakeLabel = brakeGauge.label;

    // Steering wheel
    auto* steeringCol = new QVBoxLayout();
    steeringCol->setSpacing(4);
    steeringCol->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    steeringLabel = createImageLabel(140, 140);
    steeringLabel->setStyleSheet("background: transparent;");
    steeringTextLabel = new QLabel("Steer", this);
    steeringTextLabel->setStyleSheet("font-size: 16px; font-weight: 700; color: #111111;");
    steeringTextLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    steeringCol->addWidget(steeringLabel, 0, Qt::AlignHCenter | Qt::AlignBottom);
    steeringCol->addWidget(steeringTextLabel, 0, Qt::AlignHCenter | Qt::AlignTop);

    gaugeRow->addLayout(steeringCol);
    gaugeRow->addSpacing(12);
    gaugeRow->addLayout(accelGauge.layout);
    gaugeRow->addSpacing(12);
    gaugeRow->addLayout(brakeGauge.layout);
    gaugeRow->addStretch();

    auto* bottomBar = new QHBoxLayout();
    bottomBar->setContentsMargins(0, 0, 0, 0);
    bottomBar->setSpacing(12);
    bottomBar->addWidget(gaugeContainer, 1, Qt::AlignBottom | Qt::AlignLeft);

    // Toggle button (bottom-right)
    toggleBtn = new QPushButton("Toggle View", this);
    toggleBtn->setStyleSheet("background: #5dd1ff; color: #0f1115; font-weight: 700; padding: 10px 14px; border-radius: 10px;");
    connect(toggleBtn, &QPushButton::clicked, this, &InfotainmentWidget::toggleDisplayType);
    bottomBar->addWidget(toggleBtn, 0, Qt::AlignBottom | Qt::AlignRight);

    stack->addWidget(mainContainer);

    // Score container
    scoreContainer = new QWidget(this);
    auto* scoreLayout = new QHBoxLayout(scoreContainer);
    scoreLayout->setSpacing(20);

    auto* scoreLeft = new QVBoxLayout();
    scoreLeft->setSpacing(12);
    scoreValueLabel = createImageLabel(200, 200);
    scoreValueLabel->setStyleSheet("font-size: 80px; font-weight: 900; color: #0a4f91;");
    scoreLeft->addWidget(scoreValueLabel, 1, Qt::AlignCenter);
    scoreLayout->addLayout(scoreLeft, 1);

    tierLabel = createImageLabel(200, 200);
    tierLabel->setStyleSheet("background: #f2f4f7; border: 1px solid #cfd3d8; border-radius: 12px;");
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
        name->setStyleSheet("font-size: 42px; color: #cfd2d8;");
        auto* bar = new QProgressBar(this);
        bar->setRange(0, 100);
        bar->setTextVisible(false);
        bar->setFixedHeight(16);
        bar->setStyleSheet("QProgressBar { background: #1e222b; border-radius: 8px; }"
                           "QProgressBar::chunk { background: #5dd1ff; border-radius: 8px; }");
        auto* val = new QLabel(this);
        val->setStyleSheet("font-size: 14px; color: #111111;");
        row->addWidget(name, 1);
        row->addWidget(bar, 3);
        row->addWidget(val, 0, Qt::AlignRight);
        detailLayout->addLayout(row);
        detailRows.push_back({name, bar, val});
    }
    detailLayout->addStretch();
    stack->addWidget(detailContainer);

    root->addLayout(stack, 1);
    root->addLayout(bottomBar, 0);

    RenderingData::RenderPayload initPayload{};
    initPayload.userTotalScore = BaseData::getInstance().getFrameDataCopy().userData.getUserTotalScore();
    applyImages(initPayload);

}

InfotainmentWidget::~InfotainmentWidget() = default;

void InfotainmentWidget::showFrame(const RenderingData::RenderPayload& payload) {
    // Update username from UserData (via payload.userTotalScore owner)
    headerTitle->setText("Mr.Tele");

    const int incomingDisplay = static_cast<int>(payload.displayType);
    const bool displayChanged = (incomingDisplay != lastDisplayType);
    if (displayChanged) {
        // Reset warning/emoji state whenever the view changes so the next payload applies freshly
        warningTimer.invalidate();
        directionTimer.invalidate();
        warningActive = false;
        activeWarningPixmap = nullptr;
        lastWarningSignal = -1;
        activeDirection = ScoreDirection::SCORE_NORMAL;
        lastDisplayType = incomingDisplay;
        forceApplyOnNextPayload = true;
    }

    // Only show gauges on the main dashboard-style views (hide on score/detail pages)
    const bool showGauges = payload.displayType != RenderingData::DisplayType::ScoreBoard &&
                            payload.displayType != RenderingData::DisplayType::Emotion;
    if (gaugeContainer) {
        const double fx = width() / 800.0;
        const double fy = height() / 600.0;
        const int gaugeHeight = static_cast<int>(130 * fy);
        gaugeContainer->setVisible(showGauges);
        gaugeContainer->setMinimumHeight(showGauges ? gaugeHeight + 32 : 0);
        gaugeContainer->setMaximumHeight(showGauges ? gaugeHeight + 48 : 0);
    }

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
        label->clear();
        if (fallback && *fallback) {
            label->setText(fallback);
        }
    }
}

namespace {
ImageData::SignType warningSignFromScore(uint8_t raw) {
    switch (static_cast<ScoreType>(raw)) {
    case ScoreType::SCORE_BUMP: return ImageData::SignType::BUMP;
    case ScoreType::SCORE_OVER_SPEED: return ImageData::SignType::OVERSPEED;
    case ScoreType::SCORE_SUDDEN_ACCEL: return ImageData::SignType::OVERSPEED;
    case ScoreType::SCORE_SUDDEN_CURVE: return ImageData::SignType::OVERTURN;
    default: return ImageData::SignType::NONE;
    }
}

QPixmap makeSegmentPixmap(const QColor& fill, const QColor& border) {
    const int w = 18;
    const int h = 16;
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
    const int sigSide = std::max(120, static_cast<int>(height() * 0.15));
    const bool hasTraffic = (payload.rawSignSignal != 0) && payload.signImage;
    setPixmapToLabel(signalLabel, hasTraffic ? payload.signImage : nullptr, sigSide, sigSide, "");

    const double fx = width() / 800.0;
    const double fy = height() / 600.0;
    const int w = width();
    const int h = height();
    const int leftMargin = static_cast<int>(50 * fx);
    const int rightMargin = static_cast<int>(50 * fx);
    const int gaugeHeight = static_cast<int>(130 * fy); // 460~590 -> 130px
    const int steerWidth = static_cast<int>(100 * fx);
    const int gaugeWidth = static_cast<int>(50 * fx);
    const int centerW = static_cast<int>(300 * fx);
    const int centerH = static_cast<int>(300 * fy);
    const int topMargin = std::max(0, h - centerH - (gaugeHeight + 32) - 120);
    if (contentRow) {
        contentRow->setContentsMargins(leftMargin, 0, rightMargin, 0);
        contentRow->setSpacing(static_cast<int>(100 * fx)); // gap between gif and warning
    }
    if (topSpacer) {
        topSpacer->changeSize(0, topMargin, QSizePolicy::Fixed, QSizePolicy::Fixed);
    }

    const int gifSize = std::min(centerW, centerH);
    gifSizePx = gifSize;                                      // keep stable target
    gifLabel->setMinimumSize(centerW, centerH);
    gifLabel->setMaximumSize(centerW, centerH);
    if (centerDivider) {
        centerDivider->setFixedHeight(centerH);
    }
    // cache movies
    if (!happyGif) happyGif = imageData.getEmotionGif(ImageData::EmotionGifType::HAPPY);
    if (!badGif)   badGif   = imageData.getEmotionGif(ImageData::EmotionGifType::BAD_FACE);

    // Track warning changes (score_type) and show paired emoji for a short time, otherwise default clock
    const bool forceApply = forceApplyOnNextPayload;
    forceApplyOnNextPayload = false;
    const bool warningExpired = warningActive && warningTimer.isValid() && warningTimer.elapsed() >= warningShowMs;
    if (warningExpired) {
        warningActive = false;
        activeWarningPixmap = nullptr;
        activeDirection = ScoreDirection::SCORE_NORMAL;
    }

    const bool isSupportedWarning = payload.rawWarningSignal <= static_cast<uint8_t>(ScoreType::SCORE_SUDDEN_CURVE);
    const bool shouldActivate =
        forceApply ||
        (isSupportedWarning && (!warningActive || static_cast<int>(payload.rawWarningSignal) != lastWarningSignal));
    if (shouldActivate) {
        lastWarningSignal = static_cast<int>(payload.rawWarningSignal);
        activeWarningPixmap = imageData.getSignImage(warningSignFromScore(payload.rawWarningSignal));
        warningActive = (activeWarningPixmap != nullptr);
        warningTimer.restart();

        const ScoreDirection incoming = static_cast<ScoreDirection>(payload.scoreDirection);
        if (incoming == ScoreDirection::SCORE_PLUS || incoming == ScoreDirection::SCORE_MINUS) {
            activeDirection = incoming;
            directionTimer.restart();
        } else {
            activeDirection = ScoreDirection::SCORE_NORMAL;
        }
    }

    const bool emojiWindow =
        (activeDirection == ScoreDirection::SCORE_PLUS || activeDirection == ScoreDirection::SCORE_MINUS) &&
        directionTimer.isValid() && directionTimer.elapsed() < directionShowMs &&
        warningActive;

    if (emojiWindow) {
        dateLabel->hide();
        timeLabel->hide();
        if (activeDirection == ScoreDirection::SCORE_PLUS && happyGif) {
            setGifMovie(happyGif, gifSizePx);
        } else if (activeDirection == ScoreDirection::SCORE_MINUS && badGif) {
            setGifMovie(badGif, gifSizePx);
        }
    } else {
        activeDirection = ScoreDirection::SCORE_NORMAL;
        if (currentGif) {
            currentGif->stop();
            gifLabel->setMovie(nullptr);
            currentGif = nullptr;
        }
        const auto now = QDateTime::currentDateTimeUtc().toTimeZone(QTimeZone("Asia/Seoul"));
        dateLabel->setText(now.toString("yyyy.MM.dd"));
        timeLabel->setText(now.toString("HH:mm"));
        dateLabel->show();
        timeLabel->show();
        // clear any pixmap content behind the labels
        gifLabel->setPixmap(QPixmap());
    }

    // Warning image mirrors GIF area; size set to 2/3 of GIF
    sideSizePx = std::max(1, (2 * gifSize) / 3);
    warningLabel->setFixedSize(sideSizePx, sideSizePx);

    updateDiamondGauge(payload.userTotalScore);

    // show warning sign only during the active window
    if (warningActive && activeWarningPixmap) {
        setPixmapToLabel(warningLabel, activeWarningPixmap, sideSizePx, sideSizePx, "");
    } else {
        if (warningEmpty.isNull()) {
            warningEmpty = QPixmap(sideSizePx, sideSizePx);
            warningEmpty.fill(Qt::transparent);
        }
        warningLabel->setPixmap(warningEmpty);
    }

    if (gaugeContainer) {
        // ensure layout reserves room for the gauges + labels
        gaugeContainer->setMinimumHeight(gaugeHeight + 32);
        gaugeContainer->setMaximumHeight(gaugeHeight + 48); // keep tight to bottom
        const int gaugeSpacing = static_cast<int>(20 * fx);
        const int margin = static_cast<int>(12 * fx);
        const int minWidth = steerWidth + 2 * gaugeWidth + (gaugeSpacing * 2) + (margin * 2);
        gaugeContainer->setMinimumWidth(minWidth);
    }
    if (gaugeRow) {
        const int margin = static_cast<int>(12 * fx);
        const int top = static_cast<int>(10 * fy);
        gaugeRow->setContentsMargins(margin, top, margin, 0);
        gaugeRow->setSpacing(static_cast<int>(12 * fx));
    }
    if (steeringLabel) {
        steeringLabel->setFixedSize(steerWidth, gaugeHeight);
    }
    if (throttleBar) throttleBar->setFixedSize(gaugeWidth, gaugeHeight);
    if (brakeBar) brakeBar->setFixedSize(gaugeWidth, gaugeHeight);
    if (steeringTextLabel) steeringTextLabel->setText("Steer");
    if (throttleLabel) throttleLabel->setText("A");
    if (brakeLabel) brakeLabel->setText("B");

    // throttle gauge update (0~100)
    if (throttleBar) {
        throttleBar->setValue(static_cast<int>(payload.throttle));
    }
    if (brakeBar) {
        brakeBar->setValue(static_cast<int>(payload.brake));
    }

    // Steering wheel rotation (-50 ~ 50 deg, left negative, right positive)
    if (steeringLabel) {
        if (payload.steeringWheelImage) {
            steeringWheelPix = payload.steeringWheelImage; // cache pointer
        }
        const QPixmap* src = steeringWheelPix;
        if (src) {
            const double clamped = std::clamp(payload.steerAngleDeg, -22.0, 22.0);
            const double angle = (clamped / 22.0) * 90.0; // map raw steer -> visual range
            const int targetSize = 140;
            QTransform t;
            // Qt: positive angle is CCW (left); need CW for positive degrees -> rotate(-angle)
            t.rotate(-angle);
            QPixmap rotated = src->transformed(t, Qt::SmoothTransformation);
            QPixmap scaled = rotated.scaled(targetSize, targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            steeringLabel->setPixmap(scaled);
        } else {
            steeringLabel->setText("No wheel");
        }
    }

    // update score gauge only (no numeric text)
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
        detailRows[i].value->setStyleSheet("font-size: 42px; color: #111111;");
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
    baseData.setDisplayType(nextDisplay);
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
    if (diamondRed.isNull()) {
        diamondRed = makeSegmentPixmap(QColor("#ff4d4d"), QColor("#b30000"));
    }
    const int filled = std::max(0, std::min(20, static_cast<int>(score / 5)));
    // Base fill for current state
    for (int i = 0; i < static_cast<int>(diamondLabels.size()); ++i) {
        if (i < filled) diamondLabels[i]->setPixmap(diamondFilled);
        else diamondLabels[i]->setPixmap(diamondEmpty);
    }
    // Flash any segments that turned off (3 on/off cycles → 6 steps)
    if (lastFilled > filled) {
        const int steps = 6;
        const int intervalMs = 150;
        for (int i = filled; i < lastFilled && i < static_cast<int>(diamondLabels.size()); ++i) {
            QLabel* lbl = diamondLabels[i];
            for (int s = 0; s < steps; ++s) {
                QTimer::singleShot(s * intervalMs, this, [this, lbl, s]() {
                    if (s == steps - 1) {
                        lbl->setPixmap(diamondEmpty);
                    } else if (s % 2 == 0) {
                        lbl->setPixmap(diamondRed);
                    } else {
                        lbl->setPixmap(diamondEmpty);
                    }
                });
            }
        }
    }
    lastFilled = filled;
}

void InfotainmentWidget::setGifMovie(QMovie* movie, int size) {
    if (!movie) return;
    movie->setScaledSize(QSize(size, size));
    if (currentGif != movie) {
        gifLabel->setMovie(movie);
        movie->start();
        currentGif = movie;
    }
}
