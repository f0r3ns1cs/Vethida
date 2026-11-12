#pragma once

#include "../Theme/DesignTokens.h"

#include "../../vth_core/Models/ProductTier.h"

#include <QAbstractButton>
#include <QColor>
#include <QFont>
#include <QIcon>
#include <QRectF>
#include <QString>

class QMouseEvent;

namespace vth::widgets::sidebar
{
    namespace tk = vth::theme::tokens;

    constexpr int RailWidth      = 88;
    constexpr int TopPadding     = tk::Space8;
    constexpr int BottomPadding  = tk::Space8;
    constexpr int SideMargin     = tk::Space8;

    constexpr int PrimaryItemHeight  = 60;
    constexpr int PrimaryItemSpacing = tk::Space8;
    constexpr int UtilityItemHeight  = 42;
    // The gap between utility buttons, and the gap above their block.
    constexpr int UtilitySpacing = tk::Space4;
    constexpr int UtilityTopGap  = tk::Space8;

    constexpr int IconSize = 20;

    // The active-tab background is a rounded square, not a wide rectangle.
    constexpr int PrimaryBadgeWidth  = 40;
    constexpr int PrimaryBadgeHeight = 40;
    constexpr int UtilityBadgeWidth  = 38;
    constexpr int UtilityBadgeHeight = 34;
    constexpr int BadgeRadius        = tk::RadiusCard;

    constexpr int NotificationDotSize = tk::Space8;
}

namespace vth::widgets
{
    // Base for the nav items and utility buttons: handles the icon/badge/dot
    // painting and the hover/focus state, so subclasses only do geometry + label.
    class SidebarButton : public QAbstractButton
    {
    public:
        SidebarButton(QString routeId, QString iconName, QWidget* parent = nullptr);

        const QString& routeId() const { return m_routeId; }
        void setNotificationVisible(bool visible);
        bool isNotificationVisible() const { return m_notification; }

        // The tier this destination needs. Free means no gate, which is the
        // default and almost every item.
        void setMinTier(vth::models::ProductTier tier);
        vth::models::ProductTier minTier() const { return m_minTier; }
        // True when this machine's entitlement does not reach minTier().
        bool isTierLocked() const;

        // Drops the cached ink and repaints.
        void refresh();

        // Where the selection badge is drawn, in this button's coordinates. The
        // rail's sliding indicator lines up with it.
        virtual QRectF badgeRect() const { return QRectF(rect()); }

    protected:
        QColor iconColorForState() const;
        const QIcon& stateIcon() const;
        void paintBadge(QPainter& painter, const QRectF& badge) const;
        void paintNotificationDot(QPainter& painter, const QRect& iconRect) const;
        bool wantsBadge() const;

        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void changeEvent(QEvent* event) override;

        void refreshCursor();

        QString m_routeId;
        QString m_iconName;
        bool m_notification = false;
        // Focus that arrived from the keyboard, which is the only focus that
        // draws a ring: a click focuses the item too, and must not leave one.
        bool m_keyboardFocus = false;
        vth::models::ProductTier m_minTier = vth::models::ProductTier::Free;
        mutable QColor m_cachedColor;
        mutable QIcon m_cachedIcon;
    };

    class SidebarNavItem final : public SidebarButton
    {
    public:
        SidebarNavItem(const QString& routeId,
                       const QString& label,
                       const QString& iconName,
                       QWidget* parent = nullptr);

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;
        QRectF badgeRect() const override;

    protected:
        bool hitButton(const QPoint& pos) const override;
        void paintEvent(QPaintEvent* event) override;

    private:
        QFont labelFont() const;
    };
}
