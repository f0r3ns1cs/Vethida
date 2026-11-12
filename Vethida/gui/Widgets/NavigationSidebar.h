#pragma once

#include "../../vth_core/Models/ProductTier.h"

#include <QColor>
#include <QFrame>
#include <QHash>
#include <QString>

class QButtonGroup;
class QVBoxLayout;

namespace vth::widgets
{
    class SidebarButton;

    class NavigationSidebar final : public QFrame
    {
        Q_OBJECT

    public:
        explicit NavigationSidebar(QWidget* parent = nullptr);

        // minTier gates the destination. Below it the item is inert and grayed, with a
        // tooltip naming the tier.
        void addPrimaryItem(const QString& id, const QString& label, const QString& iconName,
                            vth::models::ProductTier minTier = vth::models::ProductTier::Free);
        void addUtilityItem(const QString& id, const QString& label, const QString& iconName, bool checkable = true);

        void setCurrentItem(const QString& id);
        QString currentItem() const;

        // Puts keyboard focus on the selected item.
        void focusCurrentItem();

        void setNotificationVisible(const QString& id, bool visible);
        void setItemEnabled(const QString& id, bool enabled);
        void setItemVisible(const QString& id, bool visible);

        // Whether this destination is above the active entitlement. The window asks
        // before navigating, so every path into it is gated.
        bool isItemTierLocked(const QString& id) const;

        // Re-reads the entitlement for every item, for a license activated mid-session.
        void refreshTierGates();

    Q_SIGNALS:
        void navigationRequested(const QString& id);

    private:
        SidebarButton* buttonFor(const QString& id) const;

        QVBoxLayout* m_primaryLayout = nullptr;
        QVBoxLayout* m_utilityLayout = nullptr;
        QButtonGroup* m_group = nullptr;
        QHash<QString, SidebarButton*> m_items;
    };
}
