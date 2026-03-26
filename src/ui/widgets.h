#pragma once

#include "core/usb.h"
#include <QWidget>
#include <QPropertyAnimation>

namespace bunker::ui {

class ProgressRing : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal sweepAngle READ sweepAngle WRITE setSweepAngle)

public:
    explicit ProgressRing(QWidget* parent = nullptr);
    void setValue(int percent);
    int  value() const { return m_value; }

protected:
    void paintEvent(QPaintEvent*) override;

private:
    int   m_value = 0;
    qreal m_sweep = 0;

    qreal sweepAngle() const { return m_sweep; }
    void  setSweepAngle(qreal a) { m_sweep = a; update(); }
};


class DriveCard : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal glowStrength READ glowStrength WRITE setGlowStrength)

public:
    explicit DriveCard(QWidget* parent = nullptr);
    void setDriveInfo(const DriveInfo& info);

protected:
    void paintEvent(QPaintEvent*) override;
    void enterEvent(QEnterEvent*) override;
    void leaveEvent(QEvent*) override;

private:
    DriveInfo m_info;
    qreal m_glow = 0;
    QPropertyAnimation* m_glowAnim;

    qreal glowStrength() const { return m_glow; }
    void  setGlowStrength(qreal g) { m_glow = g; update(); }
};


class StrengthMeter : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal barWidth READ barWidth WRITE setBarWidth)

public:
    explicit StrengthMeter(QWidget* parent = nullptr);
    void setScore(int score);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    int    m_score = 0;
    qreal  m_barWidth = 0;
    QColor m_color{"#21262d"};

    qreal barWidth() const { return m_barWidth; }
    void  setBarWidth(qreal w) { m_barWidth = w; update(); }
};

}
