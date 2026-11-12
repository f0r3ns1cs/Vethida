#include "pch.h"
#include "Spinner.h"

#include "../Theme/DesignTokens.h"

#include <QPainter>
#include <QPen>
#include <QTimer>

namespace vth::widgets
{
    Spinner::Spinner(QWidget* parent, int diameter)
        : QWidget(parent)
    {
        setFixedSize(diameter, diameter);
        setAttribute(Qt::WA_TransparentForMouseEvents, true);

        timer_ = new QTimer(this);
        timer_->setInterval(FrameMs);
        connect(timer_, &QTimer::timeout, this, [this]() { update(); });
    }

    void Spinner::paintArc(QPainter& painter, const QRectF& box, int angleDeg,
                           const QColor& color, qreal penWidth)
    {
        QPen pen(color);
        pen.setWidthF(penWidth);
        pen.setCapStyle(Qt::RoundCap);
        painter.setPen(pen);
        painter.drawArc(box, (90 - angleDeg) * 16, -270 * 16);
    }

    void Spinner::showEvent(QShowEvent* event)
    {
        QWidget::showEvent(event);
        clock_.restart();
        timer_->start();
    }

    void Spinner::hideEvent(QHideEvent* event)
    {
        QWidget::hideEvent(event);
        timer_->stop();
    }

    void Spinner::paintEvent(QPaintEvent*)
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const qreal inset = 2.0;
        const QRectF box(inset, inset, width() - 2 * inset, height() - 2 * inset);
        const qint64 ms = clock_.isValid() ? clock_.elapsed() : 0;
        const int angle = static_cast<int>((ms * StepDegrees / FrameMs) % 360);
        paintArc(painter, box, angle, vth::theme::palette::accentOnSurface(), 3.0);
    }
}
