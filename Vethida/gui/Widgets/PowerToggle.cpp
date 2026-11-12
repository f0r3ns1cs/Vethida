#include "pch.h"
#include "PowerToggle.h"

#include "../Theme/DesignTokens.h"
#include "../Theme/Theme.h"
#include "../Utils/TablerIcon.h"

#include <QConicalGradient>
#include <QTimer>

#include <QEnterEvent>
#include <QEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QPixmap>
#include <QRectF>

#include <cmath>

namespace vth::widgets
{
    namespace
    {
        namespace tokens  = vth::theme::tokens;
        namespace palette = vth::theme::palette;
    }

    PowerToggle::PowerToggle(QWidget* parent)
        : QAbstractButton(parent)
    {
        setCheckable(true);
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::StrongFocus);
        setAttribute(Qt::WA_Hover, true);
        setFixedSize(sizeHint());

        m_spin = new QTimer(this);
        // 30 frames a second is plenty for a ring this slow.
        m_spin->setInterval(33);
        connect(m_spin, &QTimer::timeout, this, [this]() {
            m_phase = std::fmod(m_phase + 3.0, 360.0);
            update();
        });
    }

    void PowerToggle::syncSpin()
    {
        const bool want = isChecked() && isVisible() && isEnabled()
                       && vth::theme::animationsEnabled();
        if (want && !m_spin->isActive())
            m_spin->start();
        else if (!want && m_spin->isActive())
            m_spin->stop();
    }

    void PowerToggle::showEvent(QShowEvent* event)
    {
        QAbstractButton::showEvent(event);
        syncSpin();
    }

    void PowerToggle::hideEvent(QHideEvent* event)
    {
        QAbstractButton::hideEvent(event);
        syncSpin();
    }

    QSize PowerToggle::sizeHint() const
    {
        return { tokens::ToggleDiscSize, tokens::ToggleDiscSize };
    }

    QSize PowerToggle::minimumSizeHint() const
    {
        return sizeHint();
    }

    void PowerToggle::checkStateSet()
    {
        QAbstractButton::checkStateSet();
        syncSpin();
        update();
    }

    void PowerToggle::enterEvent(QEnterEvent* event)
    {
        m_hovered = true;
        update();
        QAbstractButton::enterEvent(event);
    }

    void PowerToggle::leaveEvent(QEvent* event)
    {
        m_hovered = false;
        update();
        QAbstractButton::leaveEvent(event);
    }

    void PowerToggle::changeEvent(QEvent* event)
    {
        // The mark is baked in paintEvent, so a repaint is the re-bake.
        if (event->type() == QEvent::ApplicationPaletteChange
            || event->type() == QEvent::PaletteChange
            || event->type() == QEvent::EnabledChange) {
            if (event->type() == QEvent::EnabledChange)
                syncSpin();
            update();
        }
        QAbstractButton::changeEvent(event);
    }

    void PowerToggle::paintEvent(QPaintEvent*)
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const qreal w = tokens::ToggleRingWidth;
        // Inset by half the pen, or the stroke straddles the widget's edge and
        // the outer half of it is clipped away.
        const QRectF disc = QRectF(rect()).adjusted(w / 2.0, w / 2.0, -w / 2.0, -w / 2.0);

        if (isChecked()) {
            // On: a turning ring around a fill drawn a step inside it.
            constexpr qreal kRing = 2.5;
            const QRectF ringRect = QRectF(rect()).adjusted(kRing / 2.0, kRing / 2.0,
                                                            -kRing / 2.0, -kRing / 2.0);
            QConicalGradient sweep(ringRect.center(), -m_phase);
            QColor fade = palette::accentOnSurface();
            fade.setAlphaF(0.15f);
            sweep.setColorAt(0.00, palette::accentOnSurface());
            sweep.setColorAt(0.35, palette::scoreGoodText());
            sweep.setColorAt(0.70, fade);
            sweep.setColorAt(1.00, palette::accentOnSurface());
            QPen ringPen(QBrush(sweep), kRing);
            ringPen.setCapStyle(Qt::RoundCap);
            painter.setPen(ringPen);
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(ringRect);

            painter.setPen(Qt::NoPen);
            painter.setBrush(m_hovered ? palette::accentFillHover()
                                       : palette::accentFill());
            painter.drawEllipse(QRectF(rect()).adjusted(kRing + 2.0, kRing + 2.0,
                                                        -kRing - 2.0, -kRing - 2.0));
        } else {
            if (m_hovered) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(palette::cardSurfaceRaised());
                painter.drawEllipse(disc);
            }

            // Solved against the circle, because a fixed pattern leaves a seam where the
            // stroke closes. Qt states dash lengths in multiples of the pen width.
            const qreal circumference = M_PI * disc.width();
            const qreal period = circumference / tokens::ToggleRingDots;
            const qreal onLen  = period * tokens::ToggleRingDutyCycle / w;
            const qreal offLen = period / w - onLen;

            QPen ring(isEnabled() ? palette::accentOnSurface() : palette::iconDisabled());
            ring.setWidthF(w);
            ring.setCapStyle(Qt::RoundCap);
            ring.setDashPattern({ onLen, offLen });
            painter.setPen(ring);
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(disc);
        }

        const QColor markColor = !isEnabled() ? palette::iconDisabled()
                              : isChecked()   ? palette::accentText()
                                              : palette::accentOnSurface();
        const QPixmap mark = vth::icons::glyphPixmap(QStringLiteral("power"), markColor,
                                                     tokens::ToggleMarkSize, this);
        const QSizeF markSize = QSizeF(mark.size()) / mark.devicePixelRatio();
        painter.drawPixmap(QPointF(rect().center().x() - markSize.width() / 2.0 + 0.5,
                                   rect().center().y() - markSize.height() / 2.0 + 0.5),
                           mark);

        if (hasFocus()) {
            QPen focus(palette::accentOnSurfaceHover());
            focus.setWidthF(tokens::FocusBorderWidth);
            painter.setPen(focus);
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(QRectF(rect()).adjusted(1, 1, -1, -1));
        }
    }
}
