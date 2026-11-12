#include "pch.h"
#include "TweakSectionView.h"

#include "../Pages/TweakCategoryPage.h"
#include "../Pages/TweakController.h"
#include "../Dialogs/TweakDetailsDialog.h"

#include "../Theme/DesignTokens.h"
#include "../Widgets/ToastManager.h"

#include "../../vth_core/Runtime/ServiceRegistry.h"
#include "../../vth_core/Tweaks/Recommend.h"
#include "../../vth_core/Tweaks/TweakEngine.h"
#include "../../vth_core/Windows/Privileges.h"

#include <QFrame>
#include <QShowEvent>
#include <QVBoxLayout>

namespace vth::ui
{
    namespace tokens = vth::theme::tokens;

    TweakSectionView::TweakSectionView(TweakController* controller,
                                       const QString& category, const QString& title,
                                       QWidget* parent, const QString& idPrefix,
                                       Chrome chrome)
        : QWidget(parent)
        , controller_(controller)
    {
        auto* root = new QVBoxLayout(this);
        root->setSpacing(0);

        // With Chrome::Card, the same one-card shell the tweak tabs use. With Chrome::None
        // the host provides the card and its inset.
        QWidget* surface = this;
        if (chrome == Chrome::Card) {
            // Horizontal inset only, and the page's: the card lines up with the
            // header above it, which supplies the gap between them.
            root->setContentsMargins(tokens::PageMarginH, 0, tokens::PageMarginH, 0);
            auto* card = new QFrame(this);
            card->setObjectName(QStringLiteral("sectionCard"));
            card->setAttribute(Qt::WA_StyledBackground, true);
            auto* cardLay = new QVBoxLayout(card);
            cardLay->setContentsMargins(0, 0, 0, 0);
            cardLay->setSpacing(0);
            root->addWidget(card, 1);
            surface = card;
            root = cardLay;
        } else {
            root->setContentsMargins(0, 0, 0, 0);
        }

        page_ = new TweakCategoryPage(surface);
        page_->setTitle(title);

        const auto matches = [&idPrefix, &category](const vth::tweaks::TweakSpec& t) {
            return idPrefix.isEmpty()
                ? vth::tweaks::recommend::categoryMatches(t.category, category)
                : t.id.startsWith(idPrefix);
        };
        QVector<const vth::tweaks::TweakSpec*> specs;
        for (const auto& t : vth::tweaks::catalog())
            if (matches(t))
                specs.push_back(&t);

        // Bind before the specs, because bind pushes state that rebuilds the pane, so the
        // rows are realized once.
        controller_->bind(page_);
        page_->setTweaks(specs);

        connect(page_, &TweakCategoryPage::detailsRequested, this,
                [this](const QString& id) { showDetails(id); });

        // Reported here, so every embedded section reports its outcome.
        connect(controller_, &TweakController::bulkFinished, this,
                [this](const BulkReport& r) {
                    // One controller, many bound views: without this filter every
                    // open section reports every other section's result.
                    if (r.category == bulkCategory_ && !r.category.isEmpty())
                        report(r);
                });

        // `root` is the card's layout when there is a card, this widget's
        // otherwise; either way the list is the one stretching child.
        root->addWidget(page_, 1);
    }

    void TweakSectionView::report(const BulkReport& r)
    {
        auto* toasts = vth::ServiceRegistry::instance().get<vth::widgets::ToastManager>();
        if (!toasts)
            return;

        // The reader stopped it themselves, or asked before the pane could answer.
        if (r.end == BulkEnd::Abandoned)
            return;

        // warning, the user needs to do something
        if (r.end == BulkEnd::Blocked) {
            if (!r.reasons.isEmpty())
                toasts->warning(tr("Nothing was applied"), r.reasons.first());
            return;
        }

        if (r.action == BulkAction::Undo) {
            if (r.succeeded == 0 && r.failed == 0)
                toasts->info(tr("Nothing to undo"), tr("Those changes are already off."));
            else if (r.failed == 0)
                toasts->success(tr("Changes undone"),
                                tr("The changes that apply turned on are off again."));
            else
                toasts->warning(tr("Some changes are still on"),
                                tr("Open a row to see why it was refused."));
            return;
        }

        const bool all = r.action == BulkAction::Everything;
        if (r.succeeded == 0 && r.failed == 0) {
            toasts->info(tr("Nothing to apply"),
                         all ? tr("Everything in this section your plan allows is "
                                  "already on.")
                             : tr("Everything recommended for this PC in this section "
                                  "is already on."));
        } else if (r.failed == 0) {
            toasts->success(all ? tr("Everything applied") : tr("Recommended tweaks applied"),
                            all ? tr("Undo is at the top of this section.")
                                : tr("The rest of this section is left as it was."));
        } else if (r.succeeded == 0) {
            // Administrator rights only when they're actually missing.
            toasts->error(tr("Nothing could be applied"),
                          vth::win::isElevated()
                              ? tr("Windows refused these changes. Open a row to see why.")
                              : tr("Restart Vethida as administrator and try again."));
        } else {
            toasts->warning(tr("Some tweaks did not apply"),
                            tr("Open a row to see why it was refused."));
        }
    }

    void TweakSectionView::setPlaceholder(const QString& text)
    {
        if (page_)
            page_->setPlaceholder(text);
    }

    void TweakSectionView::setBulkCategory(const QString& category)
    {
        bulkCategory_ = category;
        if (page_)
            page_->setBulkCategory(category);
    }

    void TweakSectionView::showEvent(QShowEvent* e)
    {
        QWidget::showEvent(e);
        if (!shownOnce_) {
            shownOnce_ = true;
            controller_->refreshDetection();
        }
    }

    void TweakSectionView::showDetails(const QString& id)
    {
        const vth::tweaks::TweakSpec* spec = vth::tweaks::find(id);
        if (!spec)
            return;

        vth::dialogs::TweakDetailsContext ctx;
        ctx.lastFailureReason = controller_->failureReason(id);
        ctx.changedByVethida  = controller_->hasRecordedChange(id);
        ctx.changedAt         = controller_->lastRecordedChange(id);

        // The modal performs nothing. The controller is the one path that reverts.
        if (vth::dialogs::showTweakDetails(this, *spec, ctx)
            == vth::dialogs::TweakDetailsOutcome::RevertRequested) {
            controller_->revertTweak(id);
        }
    }
}
