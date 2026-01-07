#include "InfotainmentWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QStackedLayout>
#include <QPushButton>
#include <QProgressBar>
#include <QPainter>
#include <QTransform>
#include <QPainterPath>
#include <QColor>
#include <QFrame>
#include <QSpacerItem>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGridLayout>
#include <algorithm>
#include <QTimer>
#include <QDateTime>
#include <QTimeZone>
#include <QEvent>
#include <QMouseEvent>
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
    root->setContentsMargins(0, 0, 0, 0); // remove padding so header sits tight to edges
    root->setSpacing(0); // remove vertical gap between header and content

    // Floating traffic light at top-left (overlay so it doesn't push content down)
    signalLabel = createImageLabel(1, 1);
    signalLabel->setScaledContents(false);
    signalLabel->setStyleSheet("background: transparent; border: none; padding: 0; margin: 0;");
    signalLabel->setContentsMargins(0, 0, 0, 0);
    signalLabel->setMargin(0);
    signalLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    signalLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    signalLabel->setParent(this);
    signalLabel->raise();
    signalLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    auto* header = new QHBoxLayout();
    header->setContentsMargins(8, 8, 12, 0); // offset on/off badge from the extreme corner
    header->setSpacing(0);
    header->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    header->addStretch();
    onOffLabel = new QLabel("OFF", this);
    onOffLabel->setFixedHeight(48);
    onOffLabel->setFixedWidth(96);
    onOffLabel->setAlignment(Qt::AlignCenter);
    onOffLabel->setStyleSheet("padding: 4px 8px; font-size: 80px; font-weight: 800; color: #ffffff; "
                              "background: #d9534f; border-radius: 12px;");
    onOffLabel->setCursor(Qt::PointingHandCursor);
    onOffLabel->installEventFilter(this);
    header->addWidget(onOffLabel, 0, Qt::AlignRight | Qt::AlignTop);
    root->addLayout(header);

    stack = new QStackedLayout();
    stack->setContentsMargins(0, 0, 0, 0);
    stack->setSpacing(0);

    mainContainer = new QWidget(this);

    // translucent warning arcs (hidden by default)
    leftWarningArc = new QLabel(this);
    leftWarningArc->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    leftWarningArc->setStyleSheet("background: transparent;");
    leftWarningArc->hide();
    rightWarningArc = new QLabel(this);
    rightWarningArc->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    rightWarningArc->setStyleSheet("background: transparent;");
    rightWarningArc->hide();

    auto* body = new QVBoxLayout(mainContainer);
    body->setContentsMargins(0, 0, 0, 0);
    body->setSpacing(0);

    // Spacer to keep content below the traffic light overlay
    topSpacer = new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
    body->addSpacerItem(topSpacer);

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
    timeLabel->setStyleSheet("color: #000000; font-size: 90px; font-weight: 800;");
    gifLayout->addWidget(dateLabel, 0, Qt::AlignHCenter | Qt::AlignTop);
    gifLayout->addWidget(timeLabel, 1, Qt::AlignCenter);
    dateLabel->hide();
    timeLabel->hide();
    contentRow->addWidget(gifLabel, 0, Qt::AlignLeft | Qt::AlignVCenter);

    centerDivider = new QFrame(this);
    centerDivider->setFrameShape(QFrame::VLine);
    centerDivider->setFrameShadow(QFrame::Plain);
    centerDivider->setFixedHeight(200);
    centerDivider->setStyleSheet("color: #42465228; background: #2e333d28;");
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
    steeringTextLabel = new QLabel("0.0", this);
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
    bottomBar->setContentsMargins(0, 0, 10, 10);
    bottomBar->setSpacing(12);
    bottomBar->addWidget(gaugeContainer, 1, Qt::AlignBottom | Qt::AlignLeft);

    // Toggle button (bottom-right)
    toggleBtn = new QPushButton("Next Page", this);
    toggleBtn->setFixedSize(160, 60);
    toggleBtn->setStyleSheet(
        "QPushButton { background: #5dd1ff; color: #0f1115; font-weight: 700; font-size: 18px; padding: 10px 14px; border-radius: 10px; }"
        "QPushButton:hover { background: #7de0ff; color: #0b0d11; transform: translateY(-1px); }"
        "QPushButton:pressed { background: #3fc3f0; color: #0b0d11; }"
    );
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
    detailLayout->setContentsMargins(12, 48, 12, 12); // add top offset so scores sit below the signal
    for (size_t i = 0; i < static_cast<size_t>(UserData::ScoreType::MAX_SCORE_TYPES); ++i) {
        auto* row = new QHBoxLayout();
        row->setSpacing(12);
        auto* name = new QLabel(this);
        name->setStyleSheet("font-size: 28px; color: #000000;");
        name->setFixedWidth(260);
        auto* bar = new QProgressBar(this);
        bar->setRange(0, 100);
        bar->setTextVisible(false);
        bar->setFixedHeight(16);
        bar->setStyleSheet("QProgressBar { background: transparent; border: 0; }"
                           "QProgressBar::chunk { background: #e74c3c; border-radius: 8px; }");
        auto* val = new QLabel(this);
        val->setStyleSheet("font-size: 14px; color: #111111;");
        val->setMinimumWidth(80);
        val->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row->addWidget(name, 1);
        row->addWidget(bar, 3);
        row->addWidget(val, 0, Qt::AlignRight);
        detailLayout->addLayout(row);
        detailRows.push_back({name, bar, val});
    }
    detailLayout->addStretch();
    stack->addWidget(detailContainer);

    // History container (timeline view)
    historyContainer = new QWidget(this);
    auto* historyLayout = new QVBoxLayout(historyContainer);
    historyLayout->setSpacing(10);
    historyLayout->setContentsMargins(12, 12, 12, 12);
    auto* historyTitle = new QLabel("Violation Timeline", this);
    historyTitle->setStyleSheet("font-size: 22px; font-weight: 800; color: #0a4f91;");
    historyLayout->addWidget(historyTitle, 0, Qt::AlignLeft);
    historyEmptyLabel = new QLabel("No violations recorded for the last session.", this);
    historyEmptyLabel->setStyleSheet("font-size: 14px; color: #444444;");
    historyLayout->addWidget(historyEmptyLabel, 0, Qt::AlignLeft);
    historyScene = new QGraphicsScene(this);
    historyView = new QGraphicsView(historyContainer);
    historyView->setScene(historyScene);
    historyView->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    historyView->setFrameShape(QFrame::NoFrame);
    historyView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    historyView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    historyLayout->addWidget(historyView, 1);
    stack->addWidget(historyContainer);

    root->addLayout(stack, 1);
    root->addLayout(bottomBar, 0);

    RenderingData::RenderPayload initPayload{};
    initPayload.userTotalScore = BaseData::getInstance().getFrameDataCopy().userData.getUserTotalScore();
    applyImages(initPayload);

}

InfotainmentWidget::~InfotainmentWidget() = default;

void InfotainmentWidget::setOnOffToggleHandler(std::function<void()> handler) {
    onOffToggleHandler = std::move(handler);
}

bool InfotainmentWidget::eventFilter(QObject* watched, QEvent* event) {
    if (watched == onOffLabel && event->type() == QEvent::MouseButtonRelease) {
        auto* me = static_cast<QMouseEvent*>(event);
        if (me && me->button() == Qt::LeftButton) {
            if (onOffToggleHandler) onOffToggleHandler();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void InfotainmentWidget::showFrame(const RenderingData::RenderPayload& payload) {
    if (signalLabel) {
        const QPixmap* signPix = imageData.getSignImage(payload.signType);
        if (!signPix) signPix = imageData.getSignImage(ImageData::SignType::NONE);
        const int target = std::max(260, static_cast<int>(height() * 0.16)); // desired size
        signalLabel->setFixedSize(target, target);
        if (signPix) {
            QPixmap scaled = signPix->scaled(target, target, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            signalLabel->setPixmap(scaled);
        } else {
            signalLabel->clear();
        }
        signalLabel->move(0, 8); // slight offset down so it sits just above nearby UI
        signalLabel->raise();
        signalLabel->show();
    }
    if (onOffLabel) {
        const bool on = payload.useDrivingScoreCheckActive;
        onOffLabel->setText(on ? "ON" : "OFF");
        onOffLabel->setStyleSheet(on
            ? "padding: 4px 8px; font-size: 20px; font-weight: 800; color: #ffffff; background: #1f8a4d; border-radius: 12px;"
            : "padding: 4px 8px; font-size: 20px; font-weight: 800; color: #ffffff; background: #d9534f; border-radius: 12px;");
        if (on && !lastUseDrivingScoreActive) {
            // Freshly turned on -> allow next payload to trigger warnings/emoji
            lastWarningSignal = -1;
            warningActive = false;
            warningTimer.invalidate();
            forceApplyOnNextPayload = true;
        }
        lastUseDrivingScoreActive = on;
    }

    const int incomingDisplay = static_cast<int>(payload.displayType);
    const bool displayChanged = (incomingDisplay != lastDisplayType);
    if (displayChanged) {
        // Reset warning state whenever the view changes so the next payload applies freshly
        warningTimer.invalidate();
        warningActive = false;
        activeWarningPixmap = nullptr;
        lastWarningSignal = -1;
        lastDisplayType = incomingDisplay;
        forceApplyOnNextPayload = true;
    }

    // Only show gauges on the main dashboard-style views (hide on score/detail pages)
    const bool showGauges = payload.displayType != RenderingData::DisplayType::ScoreBoard &&
                            payload.displayType != RenderingData::DisplayType::Emotion &&
                            payload.displayType != RenderingData::DisplayType::History;
    if (gaugeContainer) {
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
    } else if (payload.displayType == RenderingData::DisplayType::History) {
        stack->setCurrentWidget(historyContainer);
        applyHistoryView(payload);
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
    case ScoreType::SCORE_IGNORE_SIGN: return ImageData::SignType::SIGNAL_VIOLATION;
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

QPixmap makeWarningArc(int w, int h, bool left) {
    QPixmap pm(w, h);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QColor fill(255, 0, 0, 30);
    p.setPen(Qt::NoPen);
    p.setBrush(fill);

    QPainterPath path;
    if (left) {
        path.moveTo(0, 0);
        path.arcTo(QRectF(-w, 0, 2 * w, h), 90, -180);
        path.lineTo(0, h / 2);
    } else {
        path.moveTo(w, 0);
        path.arcTo(QRectF(0, 0, 2 * w, h), 90, 180);
        path.lineTo(w, h / 2);
    }
    path.closeSubpath();
    p.drawPath(path);
    return pm;
}
} // namespace

void InfotainmentWidget::applyImages(const RenderingData::RenderPayload& payload) {
    // signalLabel is updated in showFrame to maintain consistent size/resolution across pages

    const double fx = width() / 800.0;
    const double fy = height() / 600.0;
    const int h = height();
    const int leftMargin = static_cast<int>(50 * fx);
    const int rightMargin = static_cast<int>(50 * fx);
    const int gaugeHeight = static_cast<int>(130 * fy); // 460~590 -> 130px
    const int steerWidth = static_cast<int>(100 * fx);
    const int gaugeWidth = static_cast<int>(50 * fx);
    const int centerW = static_cast<int>(300 * fx);
    const int centerH = static_cast<int>(300 * fy);
    if (contentRow) {
        contentRow->setContentsMargins(leftMargin, 0, rightMargin, 0);
        contentRow->setSpacing(static_cast<int>(100 * fx)); // gap between gif and warning
    }
    if (topSpacer) {
        const int signalHeight = std::max(120, static_cast<int>(height() * 0.16));
        const int spacerHeight = std::max(20, signalHeight / 2); // leave only half the icon height as gap
        topSpacer->changeSize(0, spacerHeight, QSizePolicy::Fixed, QSizePolicy::Fixed);
    }

    const int gifSize = std::min(centerW, centerH);
    gifSizePx = gifSize;                                      // keep stable target
    gifLabel->setMinimumSize(centerW, centerH);
    gifLabel->setMaximumSize(centerW, centerH);
    
    // cache movies
    if (!happyGif) happyGif = imageData.getEmotionGif(ImageData::EmotionGifType::HAPPY);
    if (!badGif)   badGif   = imageData.getEmotionGif(ImageData::EmotionGifType::BAD_FACE);

    // Track warning changes (score_type) and show warning GIF for a short time, otherwise default clock
    const bool isSupportedWarning = payload.rawWarningSignal < static_cast<uint8_t>(ScoreType::SCORE_TYPE_NONE);
    const bool shouldActivate = payload.useDrivingScoreCheckActive && isSupportedWarning;

    const bool newWarningValue = static_cast<int>(payload.rawWarningSignal) != lastWarningSignal;
    const bool forceApply = forceApplyOnNextPayload || newWarningValue;
    forceApplyOnNextPayload = false;

    if (shouldActivate && (forceApply || !warningTimer.isValid() || !warningActive)) {
        // New warning (or reapply) should reset the window and override any existing warning
        lastWarningSignal = static_cast<int>(payload.rawWarningSignal);
        const auto warnType = warningSignFromScore(payload.rawWarningSignal);
        activeWarningPixmap = (warnType != ImageData::SignType::NONE) ? imageData.getSignImage(warnType) : nullptr;
        warningActive = (activeWarningPixmap != nullptr);
        if (warningActive) {
            warningTimer.restart(); // start fresh 3s window
        } else {
            warningTimer.invalidate();
        }
    }

    const bool warningExpired = warningActive && warningTimer.isValid() && warningTimer.elapsed() >= warningShowMs;
    if (warningExpired) {
        warningActive = false;
        activeWarningPixmap = nullptr;
        lastWarningSignal = -1; // allow re-trigger on the same type after expiry
    }

    if (!payload.useDrivingScoreCheckActive) {
        warningActive = false;
        activeWarningPixmap = nullptr;
        warningTimer.invalidate();
        lastWarningSignal = -1;
    }

    const bool emojiWindow =
        warningActive &&
        warningTimer.isValid() && warningTimer.elapsed() < warningShowMs;

    if (emojiWindow) {
        dateLabel->hide();
        timeLabel->hide();
        // With only SCORE_PLUS available, use the "bad" emoji for warning flashes
        if (badGif) {
            setGifMovie(badGif, gifSizePx);
        }
    } else {
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

    if (leftWarningArc && rightWarningArc) {
        const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
        const bool blinkOn = ((nowMs / 300) % 2) == 0; // 300ms on/off cycle
        const bool showArc = warningActive && activeWarningPixmap &&
                             payload.displayType == RenderingData::DisplayType::Dashboard &&
                             blinkOn;
        if (showArc) {
            const int arcW = std::max(100, static_cast<int>(h * 0.3)); // span top->bottom (0,0)-(0,h)
            const int arcH = h;
            const int y = 0;
            leftWarningArc->setPixmap(makeWarningArc(arcW, arcH, true));
            rightWarningArc->setPixmap(makeWarningArc(arcW, arcH, false));
            leftWarningArc->setGeometry(0, y, arcW, arcH);
            rightWarningArc->setGeometry(width() - arcW, y, arcW, arcH);
            leftWarningArc->raise();
            rightWarningArc->raise();
            leftWarningArc->show();
            rightWarningArc->show();
        } else {
            leftWarningArc->hide();
            rightWarningArc->hide();
        }
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
    if (steeringTextLabel) {
        steeringTextLabel->setFixedWidth(steerWidth);
    }
    if (throttleBar) throttleBar->setFixedSize(gaugeWidth, gaugeHeight);
    if (brakeBar) brakeBar->setFixedSize(gaugeWidth, gaugeHeight);
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
            const int targetSize = std::min(steeringLabel->width(), steeringLabel->height());
            // Rotate on a fixed-size canvas to avoid apparent shrinking at angles.
            QPixmap base = src->scaled(targetSize, targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            QPixmap canvas(targetSize, targetSize);
            canvas.fill(Qt::transparent);
            QPainter painter(&canvas);
            painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
            painter.translate(targetSize / 2.0, targetSize / 2.0);
            // Qt: positive angle is CCW (left); need CW for positive degrees -> rotate(-angle)
            painter.rotate(-angle);
            painter.translate(-base.width() / 2.0, -base.height() / 2.0);
            painter.drawPixmap(0, 0, base);
            painter.end();
            steeringLabel->setPixmap(canvas);
        } else {
            steeringLabel->setText("No wheel");
        }
    }
    if (steeringTextLabel) {
        const double val = payload.steerAngleDeg;
        const bool isZero = qFuzzyIsNull(val);
        steeringTextLabel->setText(isZero ? QString::number(0.0, 'f', 1)
                                          : QString::asprintf("%+.1f", val));
    }

    // update score gauge only (no numeric text)
}

void InfotainmentWidget::applyScoreView(const RenderingData::RenderPayload& payload) {
    scoreValueLabel->setText(QString::number(payload.userTotalScore) + " 점");

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
        const int value = payload.curScores[i];
        detailRows[i].name->setText(kNames[i]);
        detailRows[i].bar->setVisible(value > 0);
        const int scaledValue = std::min(value * 3, detailRows[i].bar->maximum());
        detailRows[i].bar->setValue(scaledValue);
        detailRows[i].value->setStyleSheet("font-size: 42px; color: #000000;");
        detailRows[i].value->setText(QString::number(value));
    }
}

const QPixmap* InfotainmentWidget::violationPixmap(uint8_t scoreType) const {
    switch (static_cast<ScoreType>(scoreType)) {
    case ScoreType::SCORE_BUMP: return imageData.getSignImage(ImageData::SignType::BUMP);
    case ScoreType::SCORE_SUDDEN_ACCEL: return imageData.getSignImage(ImageData::SignType::OVERSPEED);
    case ScoreType::SCORE_SUDDEN_CURVE: return imageData.getSignImage(ImageData::SignType::OVERTURN);
    case ScoreType::SCORE_IGNORE_SIGN: return imageData.getSignImage(ImageData::SignType::SIGNAL_VIOLATION);
    default: return nullptr;
    }
}

void InfotainmentWidget::applyHistoryView(const RenderingData::RenderPayload& payload) {
    if (!historyScene || !historyView) return;
    historyScene->clear();

    if (payload.violations.empty()) {
        if (historyEmptyLabel) historyEmptyLabel->show();
        historyView->setVisible(false);
        return;
    }
    if (historyEmptyLabel) historyEmptyLabel->hide();
    historyView->setVisible(true);

    std::vector<BaseData::ViolationEvent> events = payload.violations;
    std::sort(events.begin(), events.end(), [](const auto& a, const auto& b) {
        return a.timestampMs < b.timestampMs;
    });
    int64_t startMs = payload.sessionStartMs >= 0 ? payload.sessionStartMs : events.front().timestampMs;
    int64_t endMs = payload.sessionEndMs >= 0 ? payload.sessionEndMs : events.back().timestampMs;
    if (endMs < startMs) {
        endMs = startMs;
    }
    const int64_t duration = std::max<int64_t>(1, endMs - startMs);

    const int marginLeft = 90;
    const int marginRight = 30;
    const int marginTop = 20;
    const int laneHeight = 80;
    const int iconSize = 40;
    const char* rowTitles[] = {"Bump", "Accel", "Curve", "Signal"};
    const ScoreType rowTypes[] = {
        ScoreType::SCORE_BUMP,
        ScoreType::SCORE_SUDDEN_ACCEL,
        ScoreType::SCORE_SUDDEN_CURVE,
        ScoreType::SCORE_IGNORE_SIGN
    };
    const int rowCount = 4;

    const int viewWidth = std::max(400, historyView->viewport()->width());
    const int timelineWidth = std::max(200, viewWidth - marginLeft - marginRight);
    const int totalHeight = marginTop + laneHeight * rowCount + 20;
    historyScene->setSceneRect(0, 0, marginLeft + timelineWidth + marginRight, totalHeight);

    // Draw lanes and labels
    QPen guidePen(QColor("#e0e5ec"));
    guidePen.setWidth(2);
    for (int r = 0; r < rowCount; ++r) {
        const int yCenter = marginTop + laneHeight * r + laneHeight / 2;
        historyScene->addText(rowTitles[r], QFont("Helvetica", 11, QFont::Bold))
            ->setPos(0, yCenter - 12);
        historyScene->addLine(marginLeft, yCenter, marginLeft + timelineWidth, yCenter, guidePen);
    }

    // Time axis ticks/labels (seconds)
    QPen axisPen(QColor("#c0c5cd"));
    axisPen.setWidth(2);
    const int yAxis = marginTop + laneHeight * rowCount + 5;
    historyScene->addLine(marginLeft, yAxis, marginLeft + timelineWidth, yAxis, axisPen);
    auto tickText = [&](int64_t ms, const QString& label) {
        const double tRatio = static_cast<double>(ms - startMs) / duration;
        const double x = marginLeft + timelineWidth * tRatio;
        historyScene->addLine(x, yAxis - 6, x, yAxis + 6, axisPen);
        auto* txt = historyScene->addText(label, QFont("Helvetica", 9, QFont::Bold));
        txt->setDefaultTextColor(QColor("#0a4f91"));
        txt->setPos(x - 10, yAxis + 8);
    };
    tickText(startMs, "0s");
    tickText(endMs, QString("+%1s").arg(static_cast<int>((endMs - startMs) / 1000)));

    // Place icons
    for (const auto& ev : events) {
        const double tRatio = std::clamp(static_cast<double>(ev.timestampMs - startMs) / duration, 0.0, 1.0);
        const double x = marginLeft + timelineWidth * tRatio;
        int laneIdx = -1;
        for (int r = 0; r < rowCount; ++r) {
            if (static_cast<uint8_t>(rowTypes[r]) == ev.scoreType) {
                laneIdx = r;
                break;
            }
        }
        if (laneIdx < 0) continue;
        const int yCenter = marginTop + laneHeight * laneIdx + laneHeight / 2;
        const QPixmap* pix = violationPixmap(ev.scoreType);
        if (!pix) continue;
        QPixmap scaled = pix->scaled(iconSize, iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        auto* item = historyScene->addPixmap(scaled);
        item->setPos(x - scaled.width() / 2, yCenter - scaled.height() / 2);

        auto* txt = historyScene->addText(QString("+%1s").arg(static_cast<int>((ev.timestampMs - startMs) / 1000)),
                                          QFont("Helvetica", 8, QFont::Bold));
        txt->setDefaultTextColor(QColor("#444444"));
        txt->setPos(x - 12, yCenter - iconSize / 2 - 14);
    }
}

void InfotainmentWidget::toggleDisplayType() {
    auto frame = baseData.getFrameDataCopy();
    uint8_t nextDisplay;
    if (frame.curDisplayType == static_cast<uint8_t>(RenderingData::DisplayType::Dashboard)) {
        nextDisplay = static_cast<uint8_t>(RenderingData::DisplayType::ScoreBoard);
    } else if (frame.curDisplayType == static_cast<uint8_t>(RenderingData::DisplayType::ScoreBoard)) {
        nextDisplay = static_cast<uint8_t>(RenderingData::DisplayType::History);
    } else if (frame.curDisplayType == static_cast<uint8_t>(RenderingData::DisplayType::History)) {
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
