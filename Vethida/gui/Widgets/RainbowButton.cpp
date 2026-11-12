#include "pch.h"
#include "RainbowButton.h"

#include "../Theme/DesignTokens.h"
#include "Spinner.h"

#include <QConicalGradient>
#include <QEnterEvent>
#include <QEvent>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>

#include <cmath>

namespace vth::widgets
{
    namespace
    {
        namespace tokens = vth::theme::tokens;

        // Room around the pill for the glow; part of the widget, outside the border.
        constexpr int   kGlow        = 4;
        constexpr int   kBorder      = 2;
        constexpr int   kFrameMs     = 16;
        constexpr qreal kIdleStep    = 1.6;   // degrees per frame
        constexpr qreal kHoverStep   = 4.5;

        QConicalGradient rainbow(const QPointF& center, qreal phase, int alpha)
        {
            QConicalGradient g(center, phase);
            constexpr int kStops = 7;
            for (int i = 0; i <= kStops; ++i) {
                QColor c = QColor::fromHsvF(float(i % kStops) / kStops, 0.85f, 1.0f);
                c.setAlpha(alpha);
                g.setColorAt(qreal(i) / kStops, c);
            }
            return g;
        }
    }

    RainbowButton::RainbowButton(const QString& text, QWidget* parent)
        : QPushButton(text, parent)
    {
        setFocusPolicy(Qt::TabFocus);
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_Hover, true);
        // No commandVariant: the stylesheet has nothing to say about this button.
        setFlat(true);

        m_timer = new QTimer(this);
        m_timer->setInterval(kFrameMs);
        connect(m_timer, &QTimer::timeout, this, [this]() {
            if (!isVisible()) {
                m_timer->stop();
                return;
            }
            m_phase = std::fmod(m_phase + (m_hover ? kHoverStep : kIdleStep), 360.0);
            if (m_loading)
                m_spinAngle = (m_spinAngle + vth::widgets::Spinner::StepDegrees) % 360;
            update();
        });
        syncTimer();
    }

    QSize RainbowButton::sizeHint() const
    {
        QFont f = font();
        f.setBold(true);
        const QFontMetrics fm(f);
        const int w = fm.horizontalAdvance(text()) + 2 * (tokens::ButtonHorizontalPadding + 8);
        return { qMax(w, tokens::ButtonMinWidth) + 2 * kGlow, tokens::ButtonHeight + 2 * kGlow };
    }

    void RainbowButton::setLoading(bool loading)
    {
        if (m_loading == loading)
            return;
        m_loading = loading;
        if (loading) {
            m_savedEnabled = isEnabled();
            m_spinAngle = 0;
            setEnabled(false);
            // Disabled stops the animation; a running write should keep moving.
            syncTimer();
        } else {
            setEnabled(m_savedEnabled);
        }
        update();
    }

    void RainbowButton::syncTimer()
    {
        const bool run = (isEnabled() || m_loading) && isVisible();
        if (run && !m_timer->isActive())
            m_timer->start();
        else if (!run && m_timer->isActive())
            m_timer->stop();
    }

    void RainbowButton::changeEvent(QEvent* event)
    {
        QPushButton::changeEvent(event);
        if (event->type() == QEvent::EnabledChange) {
            if (isEnabled()) setCursor(Qt::PointingHandCursor); else unsetCursor();
            syncTimer();
            update();
        }
    }

    void RainbowButton::enterEvent(QEnterEvent* event)
    {
        m_hover = true;
        QPushButton::enterEvent(event);
        update();
    }

    void RainbowButton::leaveEvent(QEvent* event)
    {
        m_hover = false;
        QPushButton::leaveEvent(event);
        update();
    }

    void RainbowButton::paintEvent(QPaintEvent*)
    {
        // Started here, because the button is built before it's shown.
        syncTimer();

        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        const bool live = isEnabled() || m_loading;
        const QRectF pill = QRectF(rect()).adjusted(kGlow, kGlow, -kGlow, -kGlow);
        const qreal radius = pill.height() / 2.0;
        const QPointF c = pill.center();

        // Glow: the rainbow, widening outward and fading.
        if (live) {
            const int glowAlpha = m_hover ? 70 : 38;
            for (int i = kGlow; i >= 1; --i) {
                const QRectF r = pill.adjusted(-i, -i, i, i);
                QPainterPath path;
                path.addRoundedRect(r, radius + i, radius + i);
                p.fillPath(path, rainbow(c, m_phase, glowAlpha / i));
            }
        }

        // Border ring, then the fill inset by the border width.
        QPainterPath outer;
        outer.addRoundedRect(pill, radius, radius);
        if (live)
            p.fillPath(outer, rainbow(c, m_phase, 255));
        else
            p.fillPath(outer, QColor(0x5A, 0x5A, 0x66));

        const QRectF inner = pill.adjusted(kBorder, kBorder, -kBorder, -kBorder);
        const qreal innerRadius = inner.height() / 2.0;
        QPainterPath fill;
        fill.addRoundedRect(inner, innerRadius, innerRadius);
        QColor base = isDown() ? QColor(0x1E, 0x14, 0x2A)
                    : m_hover  ? QColor(0x1A, 0x16, 0x26)
                               : QColor(0x10, 0x10, 0x18);
        p.fillPath(fill, base);

        // Keyboard focus: a white ring just inside the border.
        if (hasFocus()) {
            p.setPen(QPen(QColor(255, 255, 255, 200), 1.0));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(inner.adjusted(1.5, 1.5, -1.5, -1.5),
                              innerRadius - 1.5, innerRadius - 1.5);
        }

        if (m_loading) {
            const qreal d = qMin(inner.height() - 10, 18.0);
            const QRectF box(c.x() - d / 2, c.y() - d / 2, d, d);
            vth::widgets::Spinner::paintArc(p, box, m_spinAngle, QColor(255, 255, 255), 2.4);
            return;
        }

        QFont f = font();
        f.setBold(true);
        p.setFont(f);
        p.setPen(live ? QColor(255, 255, 255) : QColor(0x9A, 0x9A, 0xA6));
        p.drawText(inner, Qt::AlignCenter, text());
    }
}
