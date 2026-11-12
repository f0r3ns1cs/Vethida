#include "pch.h"
#include "SystemPage.h"

#include "../Views/AppInstallerView.h"
#include "../Views/AppsView.h"
#include "../Views/FixesView.h"
#include "../Views/PowerPlansView.h"
#include "../Views/StartupAppsView.h"
#include "../Widgets/PageHeader.h"
#include "../Widgets/TabStrip.h"

#include <QShowEvent>
#include <QStringList>
#include <QVBoxLayout>

namespace vth::ui
{
    SystemPage::SystemPage(TweakController* controller, QWidget* parent)
        : QWidget(parent)
        , controller_(controller)
    {
        setObjectName(QStringLiteral("SystemPage"));

        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(0, 0, 0, 0);
        root->setSpacing(0);
    }

    void SystemPage::showEvent(QShowEvent* e)
    {
        QWidget::showEvent(e);
        if (!built_) {
            built_ = true;
            buildContent();
        }
    }

    void SystemPage::prewarm()
    {
        if (!built_) {
            built_ = true;
            buildContent();
        }
        if (tabs_)
            tabs_->prewarm();
        ensurePolished();
    }

    void SystemPage::buildContent()
    {
        auto* root = static_cast<QVBoxLayout*>(layout());

        header_ = new vth::widgets::PageHeader(
            tr("System"),
            tr("Fix common Windows problems, install and remove apps, choose "
               "what starts with your PC, and pick a power plan."),
            this);

        root->addWidget(header_);

        tabs_ = new vth::widgets::TabStrip(this);
        root->addWidget(tabs_, 1);

        tabs_->addTab(QStringLiteral("bug-off"), tr("Fixes"),
                      [c = controller_](QWidget* p) -> QWidget* { return new FixesView(c, p); },
                      vth::widgets::TabStrip::PaneScroll::Managed);

        tabs_->addTab(QStringLiteral("package"), tr("Get apps"),
                      [](QWidget* p) -> QWidget* { return new AppInstallerView(p); },
                      vth::widgets::TabStrip::PaneScroll::Managed);

        tabs_->addTab(QStringLiteral("layout-grid"), tr("Installed apps"),
                      [](QWidget* p) -> QWidget* { return new AppsView(p); });

        tabs_->addTab(QStringLiteral("player-play"), tr("Startup apps"),
                      [](QWidget* p) -> QWidget* { return new StartupAppsView(p); });

        // Managed like Fixes: it is a short laid-out card, not a table that
        // scrolls itself.
        tabs_->addTab(QStringLiteral("zap"), tr("Power plans"),
                      [](QWidget* p) -> QWidget* { return new PowerPlansView(p); },
                      vth::widgets::TabStrip::PaneScroll::Managed);
    }
}

namespace vth::ui
{
    void SystemPage::onTabChanged(int index)
    {
        lastTab_ = index;
    }
}
