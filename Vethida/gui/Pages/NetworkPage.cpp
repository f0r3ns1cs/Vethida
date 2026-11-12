#include "pch.h"
#include "NetworkPage.h"

#include "../Theme/DesignTokens.h"
#include "../Views/DnsView.h"
#include "../Views/QosView.h"
#include "../Views/TweakSectionView.h"
#include "../Widgets/FeatureCard.h"
#include "../Widgets/PageHeader.h"
#include "../Widgets/SectionNav.h"
#include "../Widgets/SpeedTestView.h"

#include <QFrame>
#include <QShowEvent>
#include <QVBoxLayout>

namespace vth::ui
{
    namespace tokens = vth::theme::tokens;

    namespace
    {
        using vth::widgets::makeFeatureCard;
    }

    NetworkPage::NetworkPage(TweakController* tweakController, QWidget* parent)
        : QWidget(parent)
        , tweakController_(tweakController)
    {
        setObjectName(QStringLiteral("NetworkPage"));

        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(0, 0, 0, 0);
        root->setSpacing(0);
    }

    void NetworkPage::showEvent(QShowEvent* e)
    {
        QWidget::showEvent(e);
        if (!built_) {
            built_ = true;
            buildContent();
        }
    }

    void NetworkPage::prewarm()
    {
        if (!built_) {
            built_ = true;
            buildContent();
        }
        ensurePolished();
    }

    void NetworkPage::buildContent()
    {
        auto* root = static_cast<QVBoxLayout*>(layout());

        auto* header = new vth::widgets::PageHeader(
            tr("Network"),
            tr("Measure the connection, give an app priority on it, and choose "
               "which server resolves the names you visit."),
            this);
        root->addWidget(header);

        auto* nav = new vth::widgets::SectionNav(this);

        // Managed: laid-out content, so the rail owns the scrolling and the inset. Only
        // the tweak list is Self, because it virtualizes.
        nav->addSection(QStringLiteral("gauge"), tr("Connection quality"),
                         tr("Throughput, latency, and bufferbloat grade"),
                         buildConnectionPane(),
                         vth::widgets::SectionNav::PaneScroll::Managed);

        nav->addSection(QStringLiteral("route"), tr("Traffic priority"),
                         tr("Give an app priority, or cap what it uses"),
                         new QosView(nullptr),
                         vth::widgets::SectionNav::PaneScroll::Managed,
                         vth::models::ProductTier::Pro);

        nav->addSection(QStringLiteral("world-www"), tr("DNS server"),
                         tr("Which server turns website names into addresses"),
                         new DnsView(nullptr),
                         vth::widgets::SectionNav::PaneScroll::Managed,
                         vth::models::ProductTier::Pro);

        // Chrome::None, because this pane is already inside the rail's card.
        auto* tweaks = new TweakSectionView(tweakController_,
                                            QStringLiteral("networking"),
                                            QString(), nullptr, QString(),
                                            TweakSectionView::Chrome::None);
        tweaks->setBulkCategory(QStringLiteral("networking"));
        tweaks->setPlaceholder(
            tr("Vethida has no network settings for this connection yet."));
        nav->addSection(QStringLiteral("adjustments"),
                        tr("Adapter and protocol settings"),
                        tr("TCP, DNS and adapter behavior"), tweaks);

        root->addWidget(vth::widgets::sectionNavHost(nav, this), 1);
    }

    QWidget* NetworkPage::buildConnectionPane()
    {
        auto* test = new vth::widgets::SpeedTestView(nullptr);
        return makeFeatureCard(nullptr, QStringLiteral("gauge"),
                               tr("Speed and bufferbloat test"), test,
                               vth::widgets::Accent::Neutral);
    }
}
