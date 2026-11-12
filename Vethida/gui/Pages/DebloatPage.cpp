#include "pch.h"
#include "DebloatPage.h"

#include "TweakController.h"

#include "../Theme/DesignTokens.h"
#include "../Views/BloatwareView.h"
#include "../Views/TweakSectionView.h"
#include "../Widgets/PageHeader.h"
#include "../Widgets/TabStrip.h"
#include "../Widgets/TierLockOverlay.h"

#include "../../vth_core/Models/ProductTier.h"
#include "../../vth_core/Runtime/Tier.h"
#include "../../vth_core/Tweaks/TweakEngine.h"

#include <QLabel>
#include <QShowEvent>
#include <QVBoxLayout>

namespace vth::ui
{
    namespace tokens = vth::theme::tokens;

    namespace
    {
        // A Self pane sits flush under the strip's rule. This supplies the vertical inset
        // for a pane that only carries a horizontal one.
        QWidget* insetPane(QWidget* parent, QWidget* content)
        {
            auto* host = new QWidget(parent);
            auto* lay = new QVBoxLayout(host);
            lay->setContentsMargins(0, tokens::PageMarginTop, 0,
                                    tokens::PageMarginBottom);
            lay->setSpacing(0);
            lay->addWidget(content, 1);
            return host;
        }
    }

    DebloatPage::DebloatPage(TweakController* controller, QWidget* parent)
        : QWidget(parent)
        , controller_(controller)
    {
        setObjectName(QStringLiteral("DebloatPage"));

        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(0, 0, 0, 0);
        root->setSpacing(0);

        connect(controller_, &TweakController::operationFinished, this,
                [this](const QString& id, const vth::tweaks::ApplyOutcome& outcome) {
                    const vth::tweaks::TweakSpec* spec = vth::tweaks::find(id);
                    if (outcome.ok && spec && spec->requiresReboot && rebootNote_)
                        rebootNote_->setVisible(true);
                });
    }

    void DebloatPage::showEvent(QShowEvent* e)
    {
        QWidget::showEvent(e);
        if (!built_) {
            built_ = true;
            buildContent();
        }
        if (!detected_) {
            detected_ = true;
            controller_->refreshDetection();
        }
    }

    void DebloatPage::prewarm()
    {
        if (!built_) {
            built_ = true;
            buildContent();
        }
        ensurePolished();
    }

    void DebloatPage::buildContent()
    {
        auto* root = static_cast<QVBoxLayout*>(layout());

        auto* header = new vth::widgets::PageHeader(
            tr("Debloat"),
            tr("Turn off Windows features you don't use and remove apps you didn't "
               "ask for. Uninstalled apps have to be reinstalled."),
            this);

        if (!controller_->elevated()) {
            auto* note = new QLabel(
                tr("Browse freely. To apply changes, open Vethida as administrator."), header);
            note->setProperty("uiRole", QStringLiteral("bodyText"));
            note->setWordWrap(true);
            header->addNotice(note);
        }

        rebootNote_ = new QLabel(tr("Some changes need a restart to take effect."),
                                 header);
        rebootNote_->setProperty("uiRole", QStringLiteral("bodyText"));
        rebootNote_->setVisible(false);
        header->addNotice(rebootNote_);

        root->addWidget(header);

        auto* tabs = new vth::widgets::TabStrip(this);
        root->addWidget(tabs, 1);

        TweakController* controller = controller_;
        const auto tweakTab = [controller](const QString& category) {
            return [controller, category](QWidget* p) -> QWidget* {
                auto* view = new TweakSectionView(controller, category, QString(), p,
                                                  QString(),
                                                  TweakSectionView::Chrome::Card);
                view->setBulkCategory(category);
                return insetPane(p, view);
            };
        };

        tabs->addTab(QStringLiteral("brand-windows"), tr("Windows features"),
                      tweakTab(QStringLiteral("debloating")));
        // The category id stays "browseroffice", because every catalog entry and the
        // deep-link route use it.
        tabs->addTab(QStringLiteral("globe"), tr("Browsers and apps"),
                      tweakTab(QStringLiteral("browseroffice")));
        // BloatwareView carries the full page inset itself and its table scrolls
        // natively, so it goes in bare.
        tabs->addTab(QStringLiteral("boxes"), tr("Preinstalled apps"),
                      [](QWidget* p) -> QWidget* { return new BloatwareView(p); });

        // All three are Pro, so the tab is gated once here.
        if (!vth::meetsTier(vth::models::ProductTier::Pro)) {
            auto* lock = new vth::widgets::TierLockOverlay(
                tabs, vth::models::ProductTier::Pro);
            lock->setFeatureName(tr("Debloat"));
            lock->show();
            lock->raise();
        }
    }
}
