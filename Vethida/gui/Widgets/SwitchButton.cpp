#include "pch.h"
#include "SwitchButton.h"

#include "../Theme/DesignTokens.h"
#include "../Theme/Theme.h"

#include <algorithm>

#include <QColor>
#include <QCoreApplication>
#include <QEasingCurve>
#include <QEvent>
#include <QFont>
#include <QPainter>
#include <QSizePolicy>
#include <QVariantAnimation>

namespace vth::widgets
{
    namespace
    {
        constexpr int kSlideMs = 160;   // thumb travel, OFF to ON

        QColor lerpColor(const QColor& a, const QColor& b, qreal t)
        {
            t = std::clamp(t, 0.0, 1.0);
            return QColor::fromRgbF(
                a.redF()   + (b.redF()   - a.redF())   * t,
                a.greenF() + (b.greenF() - a.greenF()) * t,
                a.blueF()  + (b.blueF()  - a.blueF())  * t,
                a.alphaF() + (b.alphaF() - a.alphaF()) * t);
        }
    }

    SwitchButton::SwitchButton(QWidget* parent)
        : QAbstractButton(parent)
    {
        setCheckable(true);
        setFocusPolicy(Qt::StrongFocus);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        updateCursor();

        m_anim = new QVariantAnimation(this);
        m_anim->setDuration(vth::theme::animationDuration(kSlideMs));
        m_anim->setEasingCurve(QEasingCurve::InOutCubic);
        connect(m_anim, &QVariantAnimation::valueChanged, this,
                [this](const QVariant& v) { m_thumbPos = v.toReal(); update(); });

        // Clicks and keyboard toggles arrive as toggled; programmatic setChecked()
        // is handled by checkStateSet() so it survives a QSignalBlocker.
        connect(this, &QAbstractButton::toggled, this, [this] { animateToState(); });
    }

    void SwitchButton::checkStateSet()
    {
        animateToState();
    }

    void SwitchButton::animateToState()
    {
        const qreal target = isChecked() ? 1.0 : 0.0;
        // Before the widget is shown, snap so pages don't slide every toggle on
        // load; once visible, glide from wherever the thumb currently sits.
        if (!isVisible()) {
            m_anim->stop();
            m_thumbPos = target;
            update();
            return;
        }
        if (m_thumbPos == target)
            return;
        m_anim->stop();
        m_anim->setStartValue(m_thumbPos);
        m_anim->setEndValue(target);
        m_anim->start();
    }

    void SwitchButton::setLabeled(bool labeled)
    {
        if (m_labeled == labeled)
            return;
        m_labeled = labeled;
        updateGeometry();
        update();
    }

    QSize SwitchButton::sizeHint() const
    {
        if (m_labeled)
            return { vth::theme::tokens::SwitchLabeledWidth, vth::theme::tokens::SwitchLabeledHeight };
        return { vth::theme::tokens::SwitchWidth, vth::theme::tokens::SwitchHitHeight };
    }

    QSize SwitchButton::minimumSizeHint() const
    {
        return sizeHint();
    }

    void SwitchButton::changeEvent(QEvent* event)
    {
        QAbstractButton::changeEvent(event);

        if (event->type() == QEvent::EnabledChange)
            updateCursor();
    }

    void SwitchButton::paintEvent(QPaintEvent*)
    {
        namespace palette = vth::theme::palette;
        namespace tokens = vth::theme::tokens;

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        if (m_labeled) {
            const qreal W     = tokens::SwitchLabeledWidth;
            const qreal H     = tokens::SwitchLabeledHeight;
            const qreal inset = tokens::SwitchLabeledInset;
            const qreal knob = H - 2 * inset;
            const qreal onX = W - inset - knob;
            // Thumb glides between the OFF and ON rest positions.
            const qreal knobX = inset + (onX - inset) * m_thumbPos;

            // Lighter, medium-tone track in both states so the white ON/OFF
            // label always reads clearly against it.
            QColor track = lerpColor(palette::toggleTrackLabeled(),
                                     palette::toggleTrackOn(), m_thumbPos);
            QColor thumb = lerpColor(palette::toggleThumb(), palette::toggleThumbOn(), m_thumbPos);
            if (!isEnabled()) {
                track = palette::divider();
                thumb = palette::textMuted();
            } else if (underMouse()) {
                track = lerpColor(palette::toggleTrackLabeledHover(),
                                  palette::toggleTrackOnHover(), m_thumbPos);
            }

            if (hasFocus() && isEnabled()) {
                painter.setPen(QPen(palette::accentOnSurface(), 2.0));
            } else {
                painter.setPen(Qt::NoPen);
            }
            painter.setBrush(track);
            painter.drawRoundedRect(QRectF(1.0, 1.0, W - 2.0, H - 2.0),
                                    (H - 2.0) / 2.0, (H - 2.0) / 2.0);
            painter.setPen(Qt::NoPen);
            painter.setBrush(thumb);
            painter.drawEllipse(QRectF(knobX, inset, knob, knob));

            QFont f = font();
            f.setPixelSize(vth::theme::type::Caption);
            f.setWeight(QFont::Weight(vth::theme::type::WeightMedium));
            painter.setFont(f);
            painter.setPen(palette::accentText());
            // Label sits in the open half of the track, opposite the thumb, and
            // flips as the thumb passes the midpoint of its travel.
            const bool showOn = m_thumbPos >= 0.5;
            const QRectF textRect = showOn
                ? QRectF(inset + 2, 0.0, knobX - inset - 4, H)
                : QRectF(knobX + knob, 0.0, W - (knobX + knob) - 4, H);
            // Not tr(): SwitchButton has no Q_OBJECT, so tr() here lands in the
            // QAbstractButton context where a translator would never find it.
            painter.drawText(textRect, Qt::AlignCenter,
                showOn ? QCoreApplication::translate("SwitchButton", "On")
                       : QCoreApplication::translate("SwitchButton", "Off"));
            return;
        }

        // The 20 px track floats centered in the taller hit area.
        const qreal trackY = (height() - tokens::SwitchHeight) / 2.0;
        const QRectF trackRect(0.0, trackY, tokens::SwitchWidth, tokens::SwitchHeight);
        constexpr qreal knobSize = tokens::SwitchThumbSize;
        const qreal knobY = trackY + tokens::SwitchInset;
        constexpr qreal offX = tokens::SwitchInset;
        constexpr qreal onX = tokens::SwitchWidth - tokens::SwitchInset - tokens::SwitchThumbSize;

        const qreal knobX = offX + (onX - offX) * m_thumbPos;

        QColor track = lerpColor(palette::toggleTrack(), palette::toggleTrackOn(), m_thumbPos);
        QColor thumb = lerpColor(palette::toggleThumb(), palette::toggleThumbOn(), m_thumbPos);

        if (!isEnabled()) {
            track = palette::divider();
            thumb = palette::textMuted();
        } else if (underMouse()) {
            track = lerpColor(palette::toggleTrackHover(),
                              palette::toggleTrackOnHover(), m_thumbPos);
        }

        if (hasFocus() && isEnabled()) {
            painter.setPen(QPen(palette::accentOnSurface(), 2.0));
        } else {
            painter.setPen(Qt::NoPen);
        }
        painter.setBrush(track);
        painter.drawRoundedRect(trackRect.adjusted(1.0, 1.0, -1.0, -1.0),
                                (tokens::SwitchHeight - 2.0) / 2.0,
                                (tokens::SwitchHeight - 2.0) / 2.0);

        painter.setPen(Qt::NoPen);
        painter.setBrush(thumb);
        painter.drawEllipse(QRectF(knobX, knobY, knobSize, knobSize));
    }

    void SwitchButton::updateCursor()
    {
        if (isEnabled())
            setCursor(Qt::PointingHandCursor);
        else
            unsetCursor();
    }
}
