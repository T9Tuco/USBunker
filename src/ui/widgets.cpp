#include "widgets.h"
#include <QPainter>
#include <QPainterPath>
#include <QEnterEvent>
#include <QFont>

namespace bunker::ui {

// ---------------------------------------------------------------------------
// ProgressRing
// ---------------------------------------------------------------------------

ProgressRing::ProgressRing(QWidget* parent) : QWidget(parent) {
    setFixedSize(192, 192);
}

void ProgressRing::setValue(int percent) {
    m_value = qBound(0, percent, 100);
    qreal target = m_value * 3.6;

    auto* anim = new QPropertyAnimation(this, "sweepAngle");
    anim->setDuration(350);
    anim->setStartValue(m_sweep);
    anim->setEndValue(target);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void ProgressRing::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const qreal pen = 5.0;
    const qreal margin = pen + 12;
    QRectF ring(margin, margin, width() - 2 * margin, height() - 2 * margin);

    // background track
    p.setPen(QPen(QColor("#21262d"), pen, Qt::SolidLine, Qt::RoundCap));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(ring);

    // active arc
    if (m_sweep > 0.1) {
        QColor accent("#00bcd4");
        p.setPen(QPen(accent, pen, Qt::SolidLine, Qt::RoundCap));
        int start = 90 * 16;
        int span  = -static_cast<int>(m_sweep * 16);
        p.drawArc(ring, start, span);
    }

    // percentage label
    p.setPen(QColor("#e6edf3"));
    QFont f = font();
    f.setPixelSize(36);
    f.setWeight(QFont::Bold);
    p.setFont(f);
    p.drawText(rect(), Qt::AlignCenter, QString("%1%").arg(m_value));
}

// ---------------------------------------------------------------------------
// DriveCard
// ---------------------------------------------------------------------------

DriveCard::DriveCard(QWidget* parent) : QWidget(parent) {
    setFixedHeight(92);
    setMinimumWidth(420);
    setCursor(Qt::PointingHandCursor);

    m_glowAnim = new QPropertyAnimation(this, "glowStrength", this);
    m_glowAnim->setDuration(180);
    m_glowAnim->setEasingCurve(QEasingCurve::OutQuad);
}

void DriveCard::setDriveInfo(const DriveInfo& info) {
    m_info = info;
    update();
}

void DriveCard::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF card = rect().adjusted(1, 1, -1, -1);

    // card fill
    p.setBrush(QColor("#161b22"));
    QColor border("#30363d");
    if (m_glow > 0.01) {
        auto lerp = [](int a, int b, qreal t) { return a + int((b - a) * t); };
        border = QColor(
            lerp(border.red(),   0,   m_glow),
            lerp(border.green(), 188, m_glow),
            lerp(border.blue(),  212, m_glow));
    }
    p.setPen(QPen(border, 1.5));
    p.drawRoundedRect(card, 12, 12);

    // usb icon (drawn manually for a clean look)
    qreal iy = (height() - 38) / 2.0;
    QRectF body(22, iy + 6, 28, 32);
    p.setPen(QPen(QColor("#00bcd4"), 2));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(body, 4, 4);
    // connector pins
    p.drawLine(QPointF(33, iy + 6), QPointF(33, iy));
    p.drawLine(QPointF(41, iy + 6), QPointF(41, iy));
    // inner chip
    p.setBrush(QColor("#00bcd4"));
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(QRectF(30, iy + 18, 12, 8), 2, 2);

    int tx = 68;

    // name
    p.setPen(QColor("#e6edf3"));
    QFont nf = font();
    nf.setPixelSize(15);
    nf.setWeight(QFont::DemiBold);
    p.setFont(nf);
    p.drawText(tx, 35, m_info.name);

    // size info
    p.setPen(QColor("#8b949e"));
    QFont df = font();
    df.setPixelSize(12);
    p.setFont(df);

    double totalGB = m_info.totalBytes / (1024.0 * 1024.0 * 1024.0);
    double freeGB  = m_info.freeBytes  / (1024.0 * 1024.0 * 1024.0);
    QString detail = QString("%1  |  %2 GB  |  %3 GB free")
        .arg(m_info.path)
        .arg(totalGB, 0, 'f', 1)
        .arg(freeGB, 0, 'f', 1);
    p.drawText(tx, 54, detail);

    // status badge
    QString label = m_info.hasVault ? "Encrypted" : "Unencrypted";
    QColor  lc    = m_info.hasVault ? QColor("#00bcd4") : QColor("#8b949e");

    QFont sf = font();
    sf.setPixelSize(11);
    sf.setWeight(QFont::Medium);
    p.setFont(sf);

    int bw = p.fontMetrics().horizontalAdvance(label) + 18;
    QRectF badge(width() - bw - 16, (height() - 24) / 2.0, bw, 24);

    p.setBrush(lc.darker(500));
    p.setPen(QPen(lc.darker(250), 1));
    p.drawRoundedRect(badge, 6, 6);

    p.setPen(lc);
    p.drawText(badge, Qt::AlignCenter, label);
}

void DriveCard::enterEvent(QEnterEvent*) {
    m_glowAnim->stop();
    m_glowAnim->setStartValue(m_glow);
    m_glowAnim->setEndValue(1.0);
    m_glowAnim->start();
}

void DriveCard::leaveEvent(QEvent*) {
    m_glowAnim->stop();
    m_glowAnim->setStartValue(m_glow);
    m_glowAnim->setEndValue(0.0);
    m_glowAnim->start();
}

// ---------------------------------------------------------------------------
// StrengthMeter
// ---------------------------------------------------------------------------

StrengthMeter::StrengthMeter(QWidget* parent) : QWidget(parent) {
    setFixedHeight(4);
}

void StrengthMeter::setScore(int score) {
    m_score = qBound(0, score, 4);

    static const QColor palette[] = {
        {"#21262d"}, {"#f85149"}, {"#f0883e"}, {"#d29922"}, {"#3fb950"}
    };
    m_color = palette[m_score];

    qreal target = m_score / 4.0;
    auto* anim = new QPropertyAnimation(this, "barWidth");
    anim->setDuration(280);
    anim->setStartValue(m_barWidth);
    anim->setEndValue(target);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void StrengthMeter::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#21262d"));
    p.drawRoundedRect(rect(), 2, 2);

    if (m_barWidth > 0.001) {
        QRectF fill(0, 0, width() * m_barWidth, height());
        p.setBrush(m_color);
        p.drawRoundedRect(fill, 2, 2);
    }
}

}
