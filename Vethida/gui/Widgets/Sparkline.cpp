#include "pch.h"
#include "Sparkline.h"

#include "../Theme/DesignTokens.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QFontMetrics>

namespace vth::ui
{
    namespace pal    = vth::theme::palette;
    namespace tokens = vth::theme::tokens;

    Sparkline::Sparkline(QWidget* parent)
        : QWidget(parent)
    {
        setMouseTracking(true);
        setAttribute(Qt::WA_Hover);

        formatter_ = [this](double v) { return defaultFormat(v); };
    }

    Sparkline::~Sparkline() = default;

    void Sparkline::appendSample(double value)
    {
        value = qBound(yMin_, value, yMax_);
        samples_.push_back(value);
        while (static_cast<int>(samples_.size()) > capacity_) {
            samples_.pop_front();
        }
        update();
    }

    void Sparkline::setXTicks(const QStringList& labels)
    {
        xTicks_ = labels;
        update();
    }

    void Sparkline::setValueFormatter(std::function<QString(double)> fmt)
    {
        formatter_ = std::move(fmt);
        update();
    }

    void Sparkline::setRange(double yMin, double yMax)
    {
        yMin_ = yMin;
        yMax_ = yMax;
        update();
    }

    void Sparkline::setCapacity(int n)
    {
        capacity_ = qMax(2, n);
        while (static_cast<int>(samples_.size()) > capacity_) {
            samples_.pop_front();
        }
        update();
    }

    QSize Sparkline::minimumSizeHint() const { return {120, 56}; }
    QSize Sparkline::sizeHint()        const { return {240, 80}; }

    QPointF Sparkline::sampleToPoint(int idx, const QRectF& r) const
    {
        const int n = static_cast<int>(samples_.size());
        if (n < 2) return r.center();

        // Fixed per-sample spacing anchored at the right edge, so a partially filled
        // window grows leftward instead of stretching across the full width.
        const double step  = r.width() / (capacity_ - 1);
        const double yFrac = (samples_[idx] - yMin_) / (yMax_ - yMin_);

        return {r.right() - (n - 1 - idx) * step,
                r.bottom() - yFrac * r.height()};
    }

    void Sparkline::paintEvent(QPaintEvent*)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const QRect wr = rect();
        QFont tiny = font();
        tiny.setPixelSize(vth::theme::type::Caption);

        constexpr int kPadTop   = tokens::Space12;
        constexpr int kPadBot   = tokens::Space16;
        constexpr int kPadLeft  = tokens::Space32;
        constexpr int kPadRight = tokens::Space12;

        const QRectF chartRect(wr.left() + kPadLeft,
                               wr.top()  + kPadTop,
                               wr.width()  - kPadLeft - kPadRight,
                               wr.height() - kPadTop  - kPadBot);
        if (chartRect.width() <= 1.0 || chartRect.height() <= 1.0)
            return;

        // From the tokens, because palette().highlight() is the OS accent.
        QColor gridCol = pal::divider();
        gridCol.setAlpha(110);
        QColor tickCol = pal::textMuted();
        tickCol.setAlpha(200);

        // Horizontal grid at every 20% with a left-gutter tick label.
        p.setFont(tiny);
        for (int pct = 0; pct <= 100; pct += 20) {
            const double y = chartRect.bottom() - (pct / 100.0) * chartRect.height();
            p.setPen(QPen(gridCol, 1, Qt::DotLine));
            p.drawLine(QPointF(chartRect.left(), y), QPointF(chartRect.right(), y));
            p.setPen(tickCol);
            p.drawText(QRectF(wr.left(), y - 6, kPadLeft - 5, 12),
                       Qt::AlignRight | Qt::AlignVCenter, QStringLiteral("%1%").arg(pct));
        }

        p.setPen(tickCol);
        if (!xTicks_.isEmpty()) {
            // Oldest at the left, newest at the right, and only over the part of the
            // window that has samples.
            const int have = static_cast<int>(samples_.size());
            const double step = chartRect.width() / (capacity_ - 1);
            const double firstX = have > 0
                ? chartRect.right() - (have - 1) * step
                : chartRect.right();

            const QFontMetrics fm(tiny);
            const int m = xTicks_.size();
            const double axisTop = chartRect.bottom() + 2;
            const double axisH   = kPadBot - 2;
            for (int i = 0; i < m; ++i) {
                const QString& label = xTicks_.at(i);
                const double frac = (m == 1) ? 0.0 : static_cast<double>(i) / (m - 1);
                const double cx   = chartRect.left() + frac * chartRect.width();
                if (cx < firstX - 0.5)
                    continue;
                const double tw   = fm.horizontalAdvance(label) + 6.0;
                QRectF box;
                int flags = Qt::AlignVCenter;
                if (i == 0) {
                    box = QRectF(chartRect.left(), axisTop, tw, axisH);
                    flags |= Qt::AlignLeft;
                } else if (i == m - 1) {
                    box = QRectF(chartRect.right() - tw, axisTop, tw, axisH);
                    flags |= Qt::AlignRight;
                } else {
                    box = QRectF(cx - tw / 2.0, axisTop, tw, axisH);
                    flags |= Qt::AlignHCenter;
                }
                p.drawText(box, flags, label);
            }
        }

        const int n = static_cast<int>(samples_.size());
        if (n < 2)
            return;

        const QColor lineCol = pal::accentOnSurface();
        QColor fillCol = lineCol;
        fillCol.setAlpha(40);

        QPainterPath fillPath;
        fillPath.moveTo(QPointF(sampleToPoint(0, chartRect).x(), chartRect.bottom()));
        for (int i = 0; i < n; ++i)
            fillPath.lineTo(sampleToPoint(i, chartRect));
        fillPath.lineTo(QPointF(sampleToPoint(n - 1, chartRect).x(), chartRect.bottom()));
        fillPath.closeSubpath();
        p.fillPath(fillPath, fillCol);

        QPainterPath linePath;
        linePath.moveTo(sampleToPoint(0, chartRect));
        for (int i = 1; i < n; ++i)
            linePath.lineTo(sampleToPoint(i, chartRect));
        p.setPen(QPen(lineCol, 1.5));
        p.drawPath(linePath);

        const QPointF last = sampleToPoint(n - 1, chartRect);
        p.setPen(Qt::NoPen);
        p.setBrush(lineCol);
        p.drawEllipse(last, 3.0, 3.0);

        // Hover: crosshair, marker, and a floating value tooltip near the point.
        const double step = chartRect.width() / (capacity_ - 1);
        const double firstX = chartRect.right() - (n - 1) * step;
        if (hovered_ && hoverX_ >= firstX && hoverX_ <= chartRect.right()) {
            const int idx = qBound(
                0, n - 1 - static_cast<int>((chartRect.right() - hoverX_) / step + 0.5), n - 1);
            const QPointF pt = sampleToPoint(idx, chartRect);

            p.setPen(QPen(lineCol, 1, Qt::DashLine));
            p.drawLine(QPointF(pt.x(), chartRect.top()), QPointF(pt.x(), chartRect.bottom()));
            p.setPen(Qt::NoPen);
            p.setBrush(lineCol);
            p.drawEllipse(pt, 3.5, 3.5);

            QFont tipFont = font();
            tipFont.setPixelSize(vth::theme::type::Caption);
            tipFont.setWeight(static_cast<QFont::Weight>(vth::theme::type::WeightMedium));
            const QFontMetrics tfm(tipFont);
            const QString val = formatter_(samples_[idx]);
            QRectF box(0, 0, tfm.horizontalAdvance(val) + 12.0, tfm.height() + 6.0);
            box.moveTo(pt.x() + 10.0, pt.y() - box.height() - 10.0);
            if (box.right() > wr.right() - 2)
                box.moveLeft(pt.x() - box.width() - 10.0);
            if (box.left() < wr.left() + 2)
                box.moveLeft(wr.left() + 2.0);
            if (box.top() < wr.top() + 2)
                box.moveTop(pt.y() + 12.0);

            QColor tipBg = pal::cardSurfaceRaised();
            tipBg.setAlpha(235);
            p.setBrush(tipBg);
            p.setPen(QPen(lineCol, 1));
            p.drawRoundedRect(box, 4.0, 4.0);
            p.setFont(tipFont);
            p.setPen(pal::textPrimary());
            p.drawText(box, Qt::AlignCenter, val);
        }
    }

    void Sparkline::mouseMoveEvent(QMouseEvent* event)
    {
        hovered_ = true;
        hoverX_  = event->pos().x();
        update();
    }

    void Sparkline::leaveEvent(QEvent*)
    {
        hovered_ = false;
        hoverX_  = -1;
        update();
    }

    QString Sparkline::defaultFormat(double v) const
    {
        return QString::number(v, 'g', 3);
    }

}
