#include "pch.h"
#include "SidebarUtilityButton.h"

#include <QPainter>
#include <QPaintEvent>

namespace vth::widgets
{
    namespace metrics = vth::widgets::sidebar;

    SidebarUtilityButton::SidebarUtilityButton(const QString& routeId,
        const QString& label, const QString& iconName, bool checkable, QWidget* parent)
        : SidebarButton(routeId, iconName, parent)
    {
        setCheckable(checkable);
        setAccessibleName(label);
        setToolTip(label);
    }

    QSize SidebarUtilityButton::sizeHint() const
    {
        return QSize(metrics::RailWidth - 2 * metrics::SideMargin, metrics::UtilityItemHeight);
    }

    QSize SidebarUtilityButton::minimumSizeHint() const
    {
        return sizeHint();
    }

    bool SidebarUtilityButton::hitButton(const QPoint& pos) const
    {
        // The whole button, because the badge is only the painted highlight.
        return rect().contains(pos);
    }

    QRectF SidebarUtilityButton::badgeRect() const
    {
        return QRectF((width() - metrics::UtilityBadgeWidth) / 2.0,
                      (height() - metrics::UtilityBadgeHeight) / 2.0,
                      metrics::UtilityBadgeWidth,
                      metrics::UtilityBadgeHeight);
    }

    void SidebarUtilityButton::paintEvent(QPaintEvent*)
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const QRectF badge = badgeRect();
        const QRect iconRect((width() - metrics::IconSize) / 2,
                             (height() - metrics::IconSize) / 2,
                             metrics::IconSize,
                             metrics::IconSize);

        if (wantsBadge())
            paintBadge(painter, badge);

        stateIcon().paint(&painter, iconRect, Qt::AlignCenter,
                          isEnabled() ? QIcon::Normal : QIcon::Disabled);

        if (m_notification)
            paintNotificationDot(painter, iconRect);
    }
}
