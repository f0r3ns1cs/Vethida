#pragma once

#include "SidebarNavItem.h"

namespace vth::widgets
{
    class SidebarUtilityButton final : public SidebarButton
    {
    public:
        SidebarUtilityButton(const QString& routeId,
                             const QString& label,
                             const QString& iconName,
                             bool checkable,
                             QWidget* parent = nullptr);

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;
        QRectF badgeRect() const override;

    protected:
        bool hitButton(const QPoint& pos) const override;
        void paintEvent(QPaintEvent* event) override;
    };
}
