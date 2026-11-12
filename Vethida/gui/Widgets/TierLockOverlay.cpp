#include "pch.h"
#include "TierLockOverlay.h"

#include "../../vth_core/SiteUrls.h"

#include "CommandButton.h"
#include "GlyphLabel.h"
#include "../Theme/DesignTokens.h"
#include "../Utils/TablerIcon.h"

#include <QColor>
#include <QDesktopServices>
#include <QEvent>
#include <QFrame>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QRectF>
#include <QUrl>
#include <QVBoxLayout>

namespace vth::widgets
{
    QString lockedDestinationTip(vth::models::ProductTier minTier)
    {
        return QObject::tr("Part of %1. Plans are on the Account tab.")
            .arg(vth::models::tierProductName(minTier));
    }

    TierLockOverlay::TierLockOverlay(QWidget* target, vth::models::ProductTier requiredTier,
                                     Presentation presentation)
        : QWidget(target)
        , target_(target)
        , tier_(requiredTier)
        , presentation_(presentation)
    {
        namespace tokens = vth::theme::tokens;
        const bool onCard = presentation_ == Presentation::Card;

        setAttribute(Qt::WA_NoSystemBackground, true);

        auto* outer = new QVBoxLayout(this);
        outer->setContentsMargins(0, 0, 0, 0);

        // The Pane form floats a raised surface on a light scrim, because the pane
        // under it is the argument for the upgrade and has to stay readable.
        panel_ = new QFrame(this);
        panel_->setAttribute(Qt::WA_StyledBackground, true);
        if (onCard) {
            panel_->setObjectName(QStringLiteral("tierLockVeil"));
        } else {
            panel_->setObjectName(QStringLiteral("advisorInfoCard"));
            panel_->setFixedWidth(tokens::ModalWidthSm);
        }

        auto* col = new QVBoxLayout(panel_);
        const int insetH = onCard ? tokens::CardInsetH : tokens::DialogInsetWideH;
        col->setContentsMargins(insetH,
                                onCard ? tokens::CardInsetV : tokens::DialogInsetTop,
                                insetH,
                                onCard ? tokens::CardInsetV : tokens::DialogInsetBottom);
        col->setSpacing(onCard ? tokens::TightGap : tokens::CardMargin);

        const int glyph = onCard ? tokens::GlyphLg : tokens::StateMarkSize;
        iconLabel_ = new vth::widgets::GlyphLabel(
            QStringLiteral("lock-filled"),
            &vth::theme::palette::premiumFill,
            glyph, panel_);
        col->addWidget(iconLabel_, 0, Qt::AlignHCenter);

        title_ = new QLabel(panel_);
        title_->setObjectName(onCard ? QStringLiteral("tierLockVeilTitle")
                                     : QStringLiteral("overlayHeading"));
        title_->setAlignment(Qt::AlignHCenter);
        title_->setWordWrap(true);
        col->addWidget(title_);

        if (!onCard) {
            body_ = new QLabel(panel_);
            body_->setObjectName(QStringLiteral("overlayBody"));
            body_->setAlignment(Qt::AlignHCenter);
            body_->setWordWrap(true);
            col->addWidget(body_);
        }

        col->addSpacing(onCard ? tokens::CompactGap : tokens::TightGap);
        auto* upgrade = new vth::widgets::CommandButton(vth::widgets::CommandButton::Variant::Primary,
                                          QObject::tr("Upgrade"), panel_);
        upgrade->setCursor(Qt::PointingHandCursor);
        connect(upgrade, &QPushButton::clicked, this, []() {
            QDesktopServices::openUrl(QUrl(vth::site::kPricing));
        });
        col->addWidget(upgrade, 0, Qt::AlignHCenter);

        outer->addWidget(panel_, 0, Qt::AlignCenter);

        refreshText();
        if (target_) {
            cover();
            target_->installEventFilter(this);
        }
    }

    void TierLockOverlay::setRequiredTier(vth::models::ProductTier tier)
    {
        if (tier_ == tier)
            return;
        tier_ = tier;
        refreshText();
    }

    void TierLockOverlay::setFeatureName(const QString& name)
    {
        if (feature_ == name)
            return;
        feature_ = name;
        refreshText();
    }

    void TierLockOverlay::refreshText()
    {
        const QString name = vth::models::tierProductName(tier_);   // "Vethida Pro" / "Vethida Ultimate"
        if (title_) {
            // On a card the tier name is the message.
            title_->setText(presentation_ == Presentation::Card
                                ? vth::models::tierLabel(tier_)
                                : QObject::tr("%1 feature").arg(name));
        }
        if (!body_)
            return;
        if (feature_.isEmpty()) {
            body_->setText(QObject::tr("Upgrade to unlock this and the rest of %1.").arg(name));
            return;
        }
        const QString plan = vth::models::tierLabel(tier_);          // "Pro" / "Ultimate"
        body_->setText(QObject::tr("%1 needs %2. Upgrading unlocks it and everything else in %2.")
                           .arg(feature_, plan));
    }

    void TierLockOverlay::paintEvent(QPaintEvent*)
    {
        QPainter painter(this);
        if (presentation_ == Presentation::Pane) {
            painter.fillRect(rect(), vth::theme::palette::scrimOverlay());
            return;
        }
        // Rounded, because this covers a rounded card exactly.
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(Qt::NoPen);
        painter.setBrush(vth::theme::palette::scrimLocked());
        painter.drawRoundedRect(QRectF(rect()),
                                vth::theme::tokens::RadiusCard,
                                vth::theme::tokens::RadiusCard);
    }

    // keep the overlay on top as the view loads its children
    bool TierLockOverlay::eventFilter(QObject* obj, QEvent* event)
    {
        if (obj == target_) {
            switch (event->type()) {
            case QEvent::Resize:
            case QEvent::Show:
            case QEvent::ChildAdded:
            case QEvent::LayoutRequest:
                cover();
                break;
            default:
                break;
            }
        }
        return QWidget::eventFilter(obj, event);
    }

    void TierLockOverlay::cover()
    {
        if (!target_)
            return;
        setGeometry(target_->rect());

        // A panel wider than the card would clip the Upgrade button.
        if (presentation_ == Presentation::Card && panel_) {
            const int room = target_->rect().width() - 2 * vth::theme::tokens::CardMargin;
            if (room > 0)
                panel_->setMaximumWidth(room);
        }
        raise();
    }
}
