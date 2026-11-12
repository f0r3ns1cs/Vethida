#include "pch.h"
#include "NavigationSidebar.h"

#include "SidebarNavItem.h"
#include "SidebarUtilityButton.h"
#include "TierLockOverlay.h"

#include "../Theme/DesignTokens.h"
#include "../Theme/Theme.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QEvent>
#include <QTimer>
#include <QVBoxLayout>

namespace vth::widgets
{
    namespace metrics = vth::widgets::sidebar;

    NavigationSidebar::NavigationSidebar(QWidget* parent)
        : QFrame(parent)
    {
        setObjectName(QStringLiteral("navigationSidebar"));
        setFixedWidth(metrics::RailWidth);
        setFrameShape(QFrame::NoFrame);

        auto* outer = new QVBoxLayout(this);
        outer->setContentsMargins(0, 0, 0, 0);
        outer->setSpacing(0);

        // A small fixed set of destinations, so no scroll area.
        auto* primaryHost = new QWidget(this);
        primaryHost->setObjectName(QStringLiteral("navigationPrimaryHost"));
        m_primaryLayout = new QVBoxLayout(primaryHost);
        m_primaryLayout->setContentsMargins(metrics::SideMargin, metrics::TopPadding, metrics::SideMargin, 0);
        m_primaryLayout->setSpacing(metrics::PrimaryItemSpacing);
        m_primaryLayout->addStretch(1);
        outer->addWidget(primaryHost, 1);

        auto* utilityHost = new QWidget(this);
        utilityHost->setObjectName(QStringLiteral("navigationUtilityHost"));
        m_utilityLayout = new QVBoxLayout(utilityHost);
        m_utilityLayout->setContentsMargins(metrics::SideMargin, metrics::UtilityTopGap, metrics::SideMargin, metrics::BottomPadding);
        m_utilityLayout->setSpacing(metrics::UtilitySpacing);
        outer->addWidget(utilityHost, 0);

        m_group = new QButtonGroup(this);
        m_group->setExclusive(true);
    }

    void NavigationSidebar::addPrimaryItem(const QString& id, const QString& label,
                                           const QString& iconName,
                                           vth::models::ProductTier minTier)
    {
        if (m_items.contains(id))
            return;

        auto* item = new SidebarNavItem(id, label, iconName);
        item->setMinTier(minTier);
        m_items.insert(id, item);
        m_primaryLayout->insertWidget(m_primaryLayout->count() - 1, item);

        // Out of the group and non-checkable, so the rail can't mark it current.
        if (item->isTierLocked()) {
            item->setCheckable(false);
            item->setToolTip(lockedDestinationTip(minTier));
            return;
        }

        m_group->addButton(item);
        connect(item, &QAbstractButton::clicked, this, [this, id] {
            Q_EMIT navigationRequested(id);
        });
    }

    void NavigationSidebar::addUtilityItem(const QString& id, const QString& label, const QString& iconName, bool checkable)
    {
        if (m_items.contains(id))
            return;

        auto* item = new SidebarUtilityButton(id, label, iconName, checkable);
        if (checkable)
            m_group->addButton(item);
        m_items.insert(id, item);
        m_utilityLayout->addWidget(item);

        connect(item, &QAbstractButton::clicked, this, [this, id] {
            Q_EMIT navigationRequested(id);
        });
    }

    SidebarButton* NavigationSidebar::buttonFor(const QString& id) const
    {
        return m_items.value(id, nullptr);
    }

    void NavigationSidebar::setCurrentItem(const QString& id)
    {
        SidebarButton* button = buttonFor(id);
        if (!button || !button->isCheckable())
            return;
        button->setChecked(true);
    }

    void NavigationSidebar::focusCurrentItem()
    {
        if (SidebarButton* button = buttonFor(currentItem()))
            button->setFocus(Qt::OtherFocusReason);
    }

    QString NavigationSidebar::currentItem() const
    {
        for (auto it = m_items.cbegin(); it != m_items.cend(); ++it) {
            if (it.value()->isCheckable() && it.value()->isChecked())
                return it.key();
        }
        return {};
    }

    void NavigationSidebar::setNotificationVisible(const QString& id, bool visible)
    {
        if (SidebarButton* button = buttonFor(id))
            button->setNotificationVisible(visible);
    }

    void NavigationSidebar::setItemEnabled(const QString& id, bool enabled)
    {
        if (SidebarButton* button = buttonFor(id))
            button->setEnabled(enabled);
    }

    void NavigationSidebar::setItemVisible(const QString& id, bool visible)
    {
        if (SidebarButton* button = buttonFor(id))
            button->setVisible(visible);
    }

    bool NavigationSidebar::isItemTierLocked(const QString& id) const
    {
        const SidebarButton* button = buttonFor(id);
        return button && button->isTierLocked();
    }

    void NavigationSidebar::refreshTierGates()
    {
        for (auto it = m_items.cbegin(); it != m_items.cend(); ++it) {
            SidebarButton* item = it.value();
            if (!item || item->minTier() == vth::models::ProductTier::Free)
                continue;

            const bool locked = item->isTierLocked();
            // Remade, because a locked item that kept its connection would still open
            // the page after a license lapsed.
            item->disconnect(SIGNAL(clicked()));

            const QString id = it.key();
            const vth::models::ProductTier minTier = item->minTier();
            if (locked) {
                m_group->removeButton(item);
                item->setCheckable(false);
                item->setToolTip(lockedDestinationTip(minTier));
            } else {
                item->setCheckable(true);
                m_group->addButton(item);
                // A rail item paints its own label, so an openable one has no tooltip.
                item->setToolTip(QString());
                connect(item, &QAbstractButton::clicked, this, [this, id] {
                    Q_EMIT navigationRequested(id);
                });
            }
            item->refresh();
        }
    }
}
