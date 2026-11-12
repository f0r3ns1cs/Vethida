#include "pch.h"
#include "FixesView.h"

#include "../Settings/Preferences.h"

#include "../Dialogs/Confirm.h"
#include "../Models/FixCatalog.h"
#include "../Pages/TweakController.h"
#include "../Theme/DesignTokens.h"
#include "../Widgets/CommandButton.h"
#include "../Widgets/ListGroup.h"
#include "../Widgets/ToastManager.h"

#include "../../vth_core/Runtime/Fetch.h"
#include "../../vth_core/Runtime/ServiceRegistry.h"
#include "../../vth_core/Windows/Fixes.h"

#include <QLayoutItem>
#include <QPointer>
#include <QPushButton>
#include <QVBoxLayout>

namespace vth::ui
{
    namespace tokens = vth::theme::tokens;

    namespace
    {
        using vth::win::fixes::Symptom;

        QString foundHeading()
        {
            return FixesView::tr("Found on this PC");
        }

        // Every detector, on one worker, keyed by catalog index. Only what was found is
        // carried.
        QHash<int, Symptom> sweep()
        {
            QHash<int, Symptom> out;
            const QVector<models::FixDef>& catalog = models::fixCatalog();
            for (int i = 0; i < catalog.size(); ++i) {
                const Symptom s = vth::win::fixes::detect(catalog.at(i).id);
                if (s.present)
                    out.insert(i, s);
            }
            return out;
        }
    }

    FixesView::FixesView(TweakController* controller, QWidget* parent)
        : QWidget(parent)
        , controller_(controller)
    {
        setObjectName(QStringLiteral("fixesView"));
        buildUi();
        // The catalog is laid out first and reordered when the sweep lands.
        rebuild();
        rescan();
    }

    FixesView::~FixesView()
    {
        sweepToken_.cancel();
    }

    void FixesView::buildUi()
    {
        list_ = new QVBoxLayout(this);
        list_->setContentsMargins(0, 0, 0, 0);
        list_->setSpacing(tokens::RegionMargin);
    }

    // rebuilt after every sweep, found problems go in the first card
    void FixesView::rebuild()
    {
        if (!list_)
            return;

        while (QLayoutItem* item = list_->takeAt(0)) {
            if (QWidget* w = item->widget()) {
                w->setParent(nullptr);
                w->deleteLater();
            }
            delete item;
        }
        cards_.clear();

        const QVector<models::FixDef>& catalog = models::fixCatalog();

        const auto addRow = [this, &catalog](vth::widgets::ListGroup* group, int i) {
            const models::FixDef& f = catalog.at(i);
            vth::widgets::ListRow* row = group->addRow(f.name, f.description);

            auto* run = new vth::widgets::CommandButton(
                vth::widgets::CommandButton::Variant::Secondary, tr("Run"), row);
            run->setAccessibleName(tr("Run %1").arg(f.name));
            connect(run, &QPushButton::clicked, this, [this, i]() { runFix(i); });
            row->addTrailing(run);

            cards_.insert(i, CardRefs{ row, run });
            refreshCard(i);
        };

        if (!symptoms_.isEmpty()) {
            auto* found = new vth::widgets::ListGroup(foundHeading(), this);
            for (int i = 0; i < catalog.size(); ++i)
                if (symptoms_.contains(i))
                    addRow(found, i);
            list_->addWidget(found);
        }

        vth::widgets::ListGroup* group = nullptr;
        for (int i = 0; i < catalog.size(); ++i) {
            const models::FixDef& f = catalog.at(i);
            if (!f.group.isEmpty() || !group) {
                // A category whose every fix was found above still opens a
                // group here; it is dropped below if it stays empty.
                if (group && group->rowCount() == 0)
                    group->deleteLater();
                else if (group)
                    list_->addWidget(group);
                group = new vth::widgets::ListGroup(f.group, this);
            }
            if (symptoms_.contains(i))
                continue;
            addRow(group, i);
        }
        if (group && group->rowCount() > 0)
            list_->addWidget(group);
        else if (group)
            group->deleteLater();
    }

    void FixesView::refreshCard(int catalogIndex)
    {
        const auto it = cards_.constFind(catalogIndex);
        if (it == cards_.cend())
            return;

        const RunState st = state_.value(catalogIndex);

        if (it->run)
            it->run->setLoading(st.phase == RunState::Phase::Running);

        if (!it->row)
            return;

        using Tone = vth::widgets::ListRow::Tone;
        switch (st.phase) {
        case RunState::Phase::Done:
        case RunState::Phase::Failed: {
            // Drawn in once, when the result arrives; a later rebuild shows it
            // already in place.
            const bool first = !announced_.contains(catalogIndex);
            announced_.insert(catalogIndex);
            it->row->setStatus(st.outcome,
                               st.phase == RunState::Phase::Done ? Tone::Good : Tone::Bad,
                               first);
            return;
        }
        case RunState::Phase::Running:
            it->row->setStatus(tr("Running. This can take a while; you can keep using Vethida."));
            return;
        case RunState::Phase::Idle:
            it->row->setStatus(symptoms_.value(catalogIndex).evidence);
            return;
        }
    }

    void FixesView::rescan()
    {
        if (scanning_)
            return;
        scanning_ = true;

        sweepToken_ = vth::fetch<QHash<int, vth::win::fixes::Symptom>>(
            this,
            []() { return sweep(); },
            [this](QHash<int, vth::win::fixes::Symptom> found) {
                scanning_ = false;
                symptoms_ = std::move(found);
                rebuild();
            });
    }

    void FixesView::runFix(int catalogIndex)
    {
        const models::FixDef* def = nullptr;
        const QVector<models::FixDef>& catalog = models::fixCatalog();
        if (catalogIndex >= 0 && catalogIndex < catalog.size())
            def = &catalog.at(catalogIndex);
        if (!def)
            return;

        // Asked of the state, because a rebuilt button isn't loading.
        if (state_.value(catalogIndex).phase == RunState::Phase::Running)
            return;

        const bool ask = !def->confirm.isEmpty()
                      && (def->alwaysAsk || !vth::prefs::askLess());
        if (ask && !vth::dialogs::confirm(this, def->name, def->confirm, tr("Run")))
            return;

        announced_.remove(catalogIndex);
        RunState pending;
        pending.phase = RunState::Phase::Running;
        state_.insert(catalogIndex, pending);
        refreshCard(catalogIndex);

        const QString fixName = def->name;
        const vth::win::fixes::FixId fixId = def->id;
        QPointer<TweakController> tweaks(controller_);

        vth::fetch<vth::win::fixes::FixResult>(
            this,
            [fixId]() { return vth::win::fixes::run(fixId); },
            [this, catalogIndex, fixName, tweaks](vth::win::fixes::FixResult result) {
                // Most repairs that need a restart already say so; the line is
                // added only to the ones that do not, so it is never said twice.
                QString body = result.message;
                // A fix that stopped on an unexpected error has nothing to say.
                if (body.isEmpty() && !result.ok)
                    body = tr("This fix stopped unexpectedly. Restart the PC and run it "
                              "again; if it stops again, export a support bundle from "
                              "Settings and send it to us.");
                if (result.ok && result.needsReboot
                    && !body.contains(QLatin1String("Restart"), Qt::CaseInsensitive))
                    body += QLatin1Char(' ') + tr("Restart the PC to finish.");

                RunState st;
                st.phase   = result.ok ? RunState::Phase::Done
                                       : RunState::Phase::Failed;
                st.outcome = body;
                state_.insert(catalogIndex, st);
                refreshCard(catalogIndex);

                // Forced, because the repair wrote outside the controller and
                // its snapshot predates that.
                if (result.ok && tweaks)
                    tweaks->refreshDetection(true);

                // The symptom may be gone, so the top group is re-read.
                if (result.ok)
                    rescan();

                if (auto* toasts =
                        vth::ServiceRegistry::instance().get<vth::widgets::ToastManager>()) {
                    if (result.ok)
                        toasts->success(fixName, body);
                    else
                        toasts->warning(fixName, body);
                }
            });
    }
}
