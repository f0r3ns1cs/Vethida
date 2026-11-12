#include "pch.h"
#include "HubPage.h"

#include "../Theme/DesignTokens.h"
#include "../Widgets/SectionNav.h"
#include "../Widgets/TierLockOverlay.h"

#include "../../vth_core/Runtime/Tier.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QShowEvent>
#include <QVBoxLayout>

#include <utility>

namespace vth::ui
{
    namespace tokens = vth::theme::tokens;

    HubPage::HubPage(const QString& title, const QString& subtitle, QWidget* parent)
        : QWidget(parent)
    {
        setAttribute(Qt::WA_StyledBackground, true);

        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(tokens::PageMarginH, tokens::PageMarginTop,
                                 tokens::PageMarginH, tokens::PageMarginBottom);
        root->setSpacing(tokens::SmallMargin);

        auto* titleLabel = new QLabel(title, this);
        titleLabel->setProperty("uiRole", QStringLiteral("pageTitle"));
        root->addWidget(titleLabel);

        if (!subtitle.isEmpty()) {
            auto* subtitleLabel = new QLabel(subtitle, this);
            subtitleLabel->setProperty("uiRole", QStringLiteral("bodyText"));
            subtitleLabel->setWordWrap(true);
            root->addWidget(subtitleLabel);
        }

        root->addSpacing(tokens::TightGap);

        // Page-level notices sit here, between the header and the rail card, so
        // they are not mistaken for content of whichever section is open.
        bodyLay_ = new QVBoxLayout;
        bodyLay_->setContentsMargins(0, 0, 0, 0);
        bodyLay_->setSpacing(tokens::TightGap);
        root->addLayout(bodyLay_);

        // One card holding the rail and the pane. The card only, because this root is
        // already inset to the page margins.
        nav_ = new vth::widgets::SectionNav(this);
        connect(nav_, &vth::widgets::SectionNav::currentChanged,
                this, [this](int i) { ensureBuilt(i); });

        root->addWidget(vth::widgets::sectionNavCard(nav_, this), 1);
    }

    void HubPage::showEvent(QShowEvent* e)
    {
        QWidget::showEvent(e);
        if (firstShown_)
            return;
        firstShown_ = true;
        // SectionNav doesn't emit currentChanged for its first section.
        if (nav_ && nav_->count() > 0)
            ensureBuilt(nav_->currentIndex());
    }

    void HubPage::addBodyWidget(QWidget* widget)
    {
        if (widget)
            bodyLay_->addWidget(widget);
    }

    int HubPage::currentSection() const
    {
        return nav_ ? nav_->currentIndex() : -1;
    }

    // SectionNav doesn't emit currentChanged for the current index, so this builds the
    // pane too.
    void HubPage::setCurrentSection(int index)
    {
        if (!nav_ || index < 0 || index >= entries_.size())
            return;
        nav_->setCurrentIndex(index);
        ensureBuilt(index);
    }

    int HubPage::addPageCard(const QString& iconName, const QString& title,
                             const QString& description, Factory factory,
                             vth::models::ProductTier minTier, PaneScroll scroll)
    {
        Entry e;
        e.factory = std::move(factory);
        e.minTier = minTier;

        // An empty host, filled on first selection, because a pane is one widget per tweak.
        e.host = new QWidget;
        auto* hostLay = new QVBoxLayout(e.host);
        hostLay->setContentsMargins(0, 0, 0, 0);
        hostLay->setSpacing(0);

        // The tier goes to the rail, and the overlay below covers any other way in.
        const int section =
            nav_->addSection(iconName, title, description, e.host, scroll, minTier);
        entries_.push_back(std::move(e));
        return section;
    }

    void HubPage::ensureBuilt(int index)
    {
        if (index < 0 || index >= entries_.size())
            return;
        Entry& e = entries_[index];
        if (e.built || !e.factory || !e.host)
            return;
        e.built = true;

        QWidget* inner = e.factory(e.host);
        e.host->layout()->addWidget(inner);

        // Tier gate: a section above the active entitlement renders dimmed under
        // a TierLockOverlay with an upgrade prompt.
        if (!vth::meetsTier(e.minTier)) {
            auto* lock = new vth::widgets::TierLockOverlay(inner, e.minTier);
            lock->show();
            lock->raise();
        }
    }
}
