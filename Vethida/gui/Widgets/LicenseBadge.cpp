#include "pch.h"
#include "LicenseBadge.h"

#include "../Theme/DesignTokens.h"
#include "../Theme/Theme.h"
#include "../Utils/TablerIcon.h"

#include <algorithm>

#include <QEasingCurve>
#include <QEnterEvent>
#include <QFontMetricsF>
#include <QPainter>
#include <QPainterPath>
#include <QVariantAnimation>
#include <QtMath>

namespace vth::widgets
{
    namespace
    {
        namespace tokens  = vth::theme::tokens;
        namespace palette = vth::theme::palette;
        namespace type    = vth::theme::type;

        constexpr int   kGlyph   = tokens::GlyphMd;
        constexpr int   kPadH    = tokens::Space12;
        constexpr int   kGap     = tokens::Space8;
        constexpr int   kHeight  = tokens::TitleBadgeHeight;
        constexpr qreal kBorder  = tokens::RuleThickness;
        constexpr qreal kFocus   = tokens::FocusBorderWidth;

        constexpr int   kHoverMs = 140;

        struct Look {
            QColor fill, fillHover, fillPressed;
            QColor edge, edgeHover;
            QColor ink, inkHover;
            QColor focus;
        };

        Look lookFor(LicenseBadge::Kind kind)
        {
            using Kind = LicenseBadge::Kind;
            switch (kind) {
            case Kind::Warn:
                return { palette::warnPillFill(), palette::warnPillFillHover(), palette::warnPillFill(),
                         palette::warningEdge(), palette::warnPillTextHover(),
                         palette::warnPillText(), palette::warnPillTextHover(),
                         palette::warnPillTextHover() };
            case Kind::Pro:
                return { palette::licensePillFill(), palette::licensePillFillHover(), palette::licensePillFill(),
                         palette::licensePillBorder(), palette::accentOnSurface(),
                         palette::licensePillText(), palette::accentOnSurfaceHover(),
                         palette::accentOnSurface() };
            case Kind::Ultimate:
                // A filled pill, so the focus ring takes the lighter accent the
                // primary button's ring takes for the same reason.
                return { palette::ultimateFill(), palette::ultimateFillHover(), palette::ultimateFillPressed(),
                         palette::ultimateFill(), palette::ultimateFillHover(),
                         palette::accentText(), palette::accentText(),
                         palette::accentOnSurfaceHover() };
            case Kind::Plain:
            default:
                return { palette::badgeFill(), palette::cardSurface(), palette::badgeFill(),
                         palette::cardBorder(), palette::cardBorderEmphasis(),
                         palette::textSecondary(), palette::textPrimary(),
                         palette::accentOnSurface() };
            }
        }

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

    LicenseBadge::LicenseBadge(QWidget* parent)
        : QAbstractButton(parent)
    {
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::TabFocus);
        setAttribute(Qt::WA_Hover, true);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        m_hoverAnim = new QVariantAnimation(this);
        m_hoverAnim->setEasingCurve(QEasingCurve::OutCubic);
        connect(m_hoverAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant& v) {
            m_hover = v.toReal();
            update();
        });
    }

    QFont LicenseBadge::badgeFont() const
    {
        QFont f = font();
        f.setPixelSize(type::Body);
        f.setWeight(QFont::Weight(type::WeightStrong));
        return f;
    }

    void LicenseBadge::setBadge(const QString& text, const QString& iconName, Kind kind)
    {
        setText(text);
        setAccessibleName(text);
        m_iconName = iconName;
        m_kind = kind;
        updateGeometry();
        update();
    }

    QSize LicenseBadge::sizeHint() const
    {
        const QFontMetricsF fm(badgeFont());
        int w = kPadH * 2 + qCeil(fm.horizontalAdvance(text()));
        if (!m_iconName.isEmpty())
            w += kGlyph + kGap;
        return { w, kHeight };
    }

    void LicenseBadge::hideEvent(QHideEvent* event)
    {
        QAbstractButton::hideEvent(event);
        m_hoverAnim->stop();
        m_hover = 0.0;
    }

    void LicenseBadge::enterEvent(QEnterEvent* event)
    {
        animateHover(true);
        QAbstractButton::enterEvent(event);
    }

    void LicenseBadge::leaveEvent(QEvent* event)
    {
        animateHover(false);
        QAbstractButton::leaveEvent(event);
    }

    void LicenseBadge::animateHover(bool hovered)
    {
        const qreal target = hovered ? 1.0 : 0.0;
        m_hoverAnim->stop();
        // Zero when animations are off, which lands the end value at once.
        m_hoverAnim->setDuration(vth::theme::animationDuration(kHoverMs));
        m_hoverAnim->setStartValue(m_hover);
        m_hoverAnim->setEndValue(target);
        m_hoverAnim->start();
    }

    void LicenseBadge::paintEvent(QPaintEvent*)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::TextAntialiasing, true);

        const Look look = lookFor(m_kind);
        const bool focused = hasFocus();
        const qreal edgeW = focused ? kFocus : kBorder;

        // The stroke is centered on the path, so the path is inset by half of
        // it and the whole ring stays inside the widget at either width.
        const QRectF outer = QRectF(rect()).adjusted(edgeW / 2, edgeW / 2, -edgeW / 2, -edgeW / 2);
        const qreal radius = outer.height() / 2.0;
        QPainterPath pill;
        pill.addRoundedRect(outer, radius, radius);

        const QColor fill = isDown() ? look.fillPressed
                                     : lerpColor(look.fill, look.fillHover, m_hover);
        const QColor edge = focused ? look.focus
                                    : lerpColor(look.edge, look.edgeHover, m_hover);
        const QColor ink = lerpColor(look.ink, look.inkHover, m_hover);

        p.fillPath(pill, fill);
        p.strokePath(pill, QPen(edge, edgeW));

        // Centered by cap height, so the word and a square glyph read as level.
        const QFont f = badgeFont();
        const QFontMetricsF fm(f);
        const qreal textW = fm.horizontalAdvance(text());
        const bool hasGlyph = !m_iconName.isEmpty();
        const qreal contentW = textW + (hasGlyph ? kGlyph + kGap : 0);
        const qreal left = (width() - contentW) / 2.0;
        const qreal midY = height() / 2.0;

        const QRectF glyphRect(left, midY - kGlyph / 2.0, kGlyph, kGlyph);
        const qreal textX = left + (hasGlyph ? kGlyph + kGap : 0);
        const qreal baseline = midY + fm.capHeight() / 2.0;

        if (hasGlyph) {
            // The glyph takes one of the two stated inks, never a blend: the
            // icon cache keys on color, and every frame of a fade is a new one.
            const QColor markInk = m_hover < 0.5 ? look.ink : look.inkHover;
            const QPixmap mark = vth::icons::glyphPixmap(m_iconName, markInk, kGlyph, this);
            p.drawPixmap(glyphRect, mark, QRectF(mark.rect()));
        }
        p.setFont(f);
        p.setPen(ink);
        p.drawText(QPointF(textX, baseline), text());
    }
}
