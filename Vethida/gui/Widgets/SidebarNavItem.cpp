#include "pch.h"
#include "SidebarNavItem.h"

#include "../Theme/DesignTokens.h"
#include "../Utils/TablerIcon.h"

#include "../../vth_core/Runtime/Tier.h"

#include <QFontMetrics>
#include <QCursor>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>

namespace vth::widgets
{
    namespace palette = vth::theme::palette;
    namespace metrics = vth::widgets::sidebar;

    SidebarButton::SidebarButton(QString routeId, QString iconName, QWidget* parent)
        : QAbstractButton(parent)
        , m_routeId(std::move(routeId))
        , m_iconName(std::move(iconName))
    {
        setCheckable(true);
        setFocusPolicy(Qt::StrongFocus);
        setMouseTracking(true);
        unsetCursor();
        setAttribute(Qt::WA_Hover, true);

        connect(this, &QAbstractButton::toggled, this, [this](bool) { update(); });
        connect(this, &QAbstractButton::pressed, this, [this] { update(); });
        connect(this, &QAbstractButton::released, this, [this] { update(); });
    }

    void SidebarButton::setNotificationVisible(bool visible)
    {
        if (m_notification == visible)
            return;
        m_notification = visible;
        update();
    }

    void SidebarButton::refresh()
    {
        m_cachedColor = QColor();
        update();
    }

    void SidebarButton::setMinTier(vth::models::ProductTier tier)
    {
        if (m_minTier == tier)
            return;
        m_minTier = tier;
        refresh();
    }

    bool SidebarButton::isTierLocked() const
    {
        return !vth::meetsTier(m_minTier);
    }

    QColor SidebarButton::iconColorForState() const
    {
        // A destination this license can't open is drawn in the disabled ink.
        if (!isEnabled() || isTierLocked())
            return vth::icons::disabledIconColor();
        if (isChecked())
            return palette::accentOnSurface();
        if (isDown() || underMouse() || hasFocus())
            return palette::textPrimary();
        return palette::sidebarIconIdle();
    }

    const QIcon& SidebarButton::stateIcon() const
    {
        const QColor color = iconColorForState();
        if (color != m_cachedColor) {
            m_cachedColor = color;
            m_cachedIcon  = vth::icons::icon(m_iconName, color, metrics::IconSize);
        }
        return m_cachedIcon;
    }

    bool SidebarButton::wantsBadge() const
    {
        if (isTierLocked())
            return false;
        const bool hot = isEnabled() && underMouse();
        return isChecked() || isDown() || hot || hasFocus();
    }

    void SidebarButton::paintBadge(QPainter& painter, const QRectF& badge) const
    {
        painter.setPen(Qt::NoPen);
        if (isChecked()) {
            painter.setBrush(palette::sidebarItemSelected());
            painter.drawRoundedRect(badge, metrics::BadgeRadius, metrics::BadgeRadius);
        } else if (isDown()) {
            // A step deeper than hover, the press every button in the app takes.
            painter.setBrush(palette::sidebarItemActive());
            painter.drawRoundedRect(badge, metrics::BadgeRadius, metrics::BadgeRadius);
        } else if (underMouse()) {
            painter.setBrush(palette::sidebarItemHover());
            painter.drawRoundedRect(badge, metrics::BadgeRadius, metrics::BadgeRadius);
        }

        // The keyboard's position on the rail.
        if (m_keyboardFocus && hasFocus()) {
            constexpr qreal w = vth::theme::tokens::FocusBorderWidth;
            painter.setPen(QPen(palette::accentOnSurface(), w));
            painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(badge.adjusted(w / 2, w / 2, -w / 2, -w / 2),
                                    metrics::BadgeRadius - w / 2, metrics::BadgeRadius - w / 2);
            painter.setPen(Qt::NoPen);
        }
    }

    void SidebarButton::paintNotificationDot(QPainter& painter, const QRect& iconRect) const
    {
        const int d = metrics::NotificationDotSize;
        const QRect dot(iconRect.right() - d + 2, iconRect.top() - 1, d, d);

        painter.setPen(Qt::NoPen);

        painter.setBrush(palette::appChrome());
        painter.drawEllipse(dot.adjusted(-2, -2, 2, 2));
        painter.setBrush(palette::accentOnSurface());
        painter.drawEllipse(dot);
    }

    void SidebarButton::enterEvent(QEnterEvent* event)
    {
        QAbstractButton::enterEvent(event);
        refreshCursor();
        update();
    }

    void SidebarButton::leaveEvent(QEvent* event)
    {
        QAbstractButton::leaveEvent(event);
        unsetCursor();
        update();
    }

    void SidebarButton::mouseMoveEvent(QMouseEvent* event)
    {
        QAbstractButton::mouseMoveEvent(event);
        refreshCursor();
    }

    void SidebarButton::focusInEvent(QFocusEvent* event)
    {
        QAbstractButton::focusInEvent(event);
        const Qt::FocusReason reason = event->reason();
        m_keyboardFocus = reason == Qt::TabFocusReason || reason == Qt::BacktabFocusReason
                       || reason == Qt::ShortcutFocusReason;
        update();
    }

    void SidebarButton::focusOutEvent(QFocusEvent* event)
    {
        QAbstractButton::focusOutEvent(event);
        m_keyboardFocus = false;
        update();
    }

    void SidebarButton::keyPressEvent(QKeyEvent* event)
    {
        if (event->key() == Qt::Key_Down) {
            focusNextPrevChild(true);
            return;
        }
        if (event->key() == Qt::Key_Up) {
            focusNextPrevChild(false);
            return;
        }
        QAbstractButton::keyPressEvent(event);
    }

    void SidebarButton::changeEvent(QEvent* event)
    {
        QAbstractButton::changeEvent(event);
        if (event->type() == QEvent::EnabledChange) {
            refreshCursor();
            refresh();
            return;
        }
        if (event->type() == QEvent::ApplicationPaletteChange)
            refresh();
    }

    void SidebarButton::refreshCursor()
    {
        // A pointing hand promises the thing under it opens. A locked
        // destination keeps the arrow, and the tooltip says why.
        if (isEnabled() && !isTierLocked() && underMouse()
            && hitButton(mapFromGlobal(QCursor::pos())))
            setCursor(Qt::PointingHandCursor);
        else
            unsetCursor();
    }

    SidebarNavItem::SidebarNavItem(const QString& routeId,
                                   const QString& label,
                                   const QString& iconName,
                                   QWidget* parent)
        : SidebarButton(routeId, iconName, parent)
    {
        setText(label);
        setAccessibleName(label);
        // No tooltip, because this item paints its label.
    }

    QSize SidebarNavItem::sizeHint() const
    {
        return QSize(metrics::RailWidth - 2 * metrics::SideMargin, metrics::PrimaryItemHeight);
    }

    QSize SidebarNavItem::minimumSizeHint() const
    {
        return sizeHint();
    }

    bool SidebarNavItem::hitButton(const QPoint& pos) const
    {
        return rect().contains(pos);
    }

    // Selection is carried by color, because a weight change would reflow the label.
    QFont SidebarNavItem::labelFont() const
    {
        QFont f = font();
        f.setPixelSize(vth::theme::type::Caption);
        f.setWeight(QFont::Weight(vth::theme::type::WeightMedium));
        return f;
    }

    QRectF SidebarNavItem::badgeRect() const
    {
        const int labelHeight = QFontMetrics(labelFont()).height();
        const int contentHeight = metrics::PrimaryBadgeHeight + labelHeight;
        const int top = (height() - contentHeight) / 2;
        return QRectF((width() - metrics::PrimaryBadgeWidth) / 2.0, top,
                      metrics::PrimaryBadgeWidth, metrics::PrimaryBadgeHeight);
    }

    void SidebarNavItem::paintEvent(QPaintEvent*)
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const QFont labelFont = this->labelFont();
        const QFontMetrics fm(labelFont);
        const int labelHeight = fm.height();

        const QRectF badge = badgeRect();
        const int top = static_cast<int>(badge.top());
        const QRect iconRect((width() - metrics::IconSize) / 2,
                            top + (metrics::PrimaryBadgeHeight - metrics::IconSize) / 2,
                            metrics::IconSize, metrics::IconSize);

        if (wantsBadge())
            paintBadge(painter, badge);

        stateIcon().paint(&painter, iconRect, Qt::AlignCenter, isEnabled() ? QIcon::Normal : QIcon::Disabled);

        if (m_notification)
            paintNotificationDot(painter, iconRect);

        painter.setFont(labelFont);
        QColor labelColor;
        if (!isEnabled() || isTierLocked())
            labelColor = palette::textMuted();
        else if (isChecked() || isDown() || underMouse() || hasFocus())
            labelColor = palette::textPrimary();
        else
            labelColor = palette::textSecondary();
        painter.setPen(labelColor);

        const QRect labelRect(0, top + metrics::PrimaryBadgeHeight, width(), labelHeight);
        const QString elided = fm.elidedText(text(), Qt::ElideRight, width());
        painter.drawText(labelRect, Qt::AlignHCenter | Qt::AlignTop, elided);
    }
}
