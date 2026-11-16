#include "pch.h"
#include "GameProfilesView.h"

#include "../Dialogs/Confirm.h"
#include "../Pages/SystemTools.h"
#include "../Theme/DesignTokens.h"
#include "../Widgets/CommandButton.h"
#include "../Widgets/EmptyState.h"
#include "../Widgets/PageScaffold.h"

#include "../../vth_core/Models/ProductTier.h"

#include "../../vth_core/Runtime/Fetch.h"
#include "../../vth_core/Runtime/Threading.h"
#include "../../vth_core/Windows/GameProfiles.h"

#include <QDir>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace vth::ui
{
    namespace gp     = vth::win::gameprofiles;
    namespace tokens = vth::theme::tokens;

    GameProfilesView::GameProfilesView(QWidget* parent) : QWidget(parent)
    {
        buildUi();
        populate();
    }

    void GameProfilesView::buildUi()
    {
        auto* outer = new QVBoxLayout(this);
        outer->setContentsMargins(0, 0, 0, 0);
        outer->setSpacing(0);

        // PageScaffold owns the header, the scroll region and the inset.
        auto* scaffold = new vth::widgets::PageScaffold(
            tr("Game profiles"),
            tr("Vethida replaces a game's settings file with one configured for "
               "frame rate rather than visual quality. Your current settings are "
               "backed up first."),
            this);
        outer->addWidget(scaffold);

        headline_ = new QLabel;
        headline_->setObjectName(QStringLiteral("settingsRowDesc"));
        headline_->setWordWrap(true);
        scaffold->addBodyWidget(headline_);

        auto* rowsHost = new QWidget;
        rows_ = new QVBoxLayout(rowsHost);
        rows_->setContentsMargins(0, 0, 0, 0);
        rows_->setSpacing(tokens::CompactGap);
        scaffold->addBodyWidget(rowsHost);
    }

    void GameProfilesView::populate()
    {
        VTH_ASSERT_UI_THREAD();
        // gp::enumerate() walks the filesystem looking for game configs, so it
        // never runs here. The headline carries the loading state until it lands.
        headline_->setText(tr("Looking for supported games..."));
        loadToken_ = vth::fetch<QVector<gp::Profile>>(
            this,
            []() { return gp::enumerate(); },
            [this](QVector<gp::Profile> profiles) { populateRows(profiles); });
    }

    void GameProfilesView::populateRows(const QVector<gp::Profile>& profiles)
    {
        VTH_ASSERT_UI_THREAD();
        int found = 0;
        for (const gp::Profile& p : profiles) {
            if (p.settingsPath.isEmpty())
                continue;   // config not present on this machine
            ++found;

            auto* row = new QFrame;
            row->setObjectName(QStringLiteral("settingsRow"));
            row->setAttribute(Qt::WA_StyledBackground, true);
            auto* rowLay = new QHBoxLayout(row);
            rowLay->setContentsMargins(tokens::RowInsetH, tokens::RowInsetV,
                                       tokens::RowInsetH, tokens::RowInsetV);
            rowLay->setSpacing(tokens::CardMargin);

            auto* text = new QVBoxLayout;
            text->setSpacing(tokens::TightGap);
            auto* name = new QLabel(p.name, row);
            name->setObjectName(QStringLiteral("settingsRowTitle"));
            text->addWidget(name);
            auto* sub = new QLabel(QDir::toNativeSeparators(p.settingsPath), row);
            sub->setObjectName(QStringLiteral("settingsRowDesc"));
            text->addWidget(sub);
            rowLay->addLayout(text, 1);

            auto* restore = new vth::widgets::CommandButton(vth::widgets::CommandButton::Variant::Secondary,
                                              tr("Restore"), row);
            connect(restore, &QPushButton::clicked, this, [this, p, restore]() {
                // File IO: worker only. The button carries the busy state so the
                // click is acknowledged without a modal or a frozen window.
                restore->setLoading(true);
                vth::fetch<gp::WriteResult>(
                    this,
                    [p]() { return gp::restoreBackup(p); },
                    [this, p, restore](gp::WriteResult r) {
                        restore->setLoading(false);
                        vth::dialogs::inform(this, tr("Restore %1").arg(p.name),
                            r.ok ? tr("Your previous %1 settings were restored.").arg(p.name)
                                 : (r.error.isEmpty()
                                        ? tr("There was no backup to restore.")
                                        : tr("Vethida could not restore your settings."
                                             "\n\n%1").arg(r.error)));
                    });
            });

            auto* optimize = new vth::widgets::CommandButton(vth::widgets::CommandButton::Variant::Primary,
                                               tr("Optimize"), row);
            connect(optimize, &QPushButton::clicked, this, [this, p, optimize]() {
                // Pro gates writing a profile, not undoing one. Restore stays
                // open at every tier: a user whose subscription lapsed must
                // still be able to put their own settings back, and paywalling
                // the undo of a change we made is not a thing to ship.
                if (!vth::ui::tools::ensureTier(this, vth::models::ProductTier::Pro))
                    return;
                const QVector<gp::Setting> rec = gp::recommendedProfile(p.id);
                if (rec.isEmpty()) {
                    vth::dialogs::inform(this, p.name,
                        tr("No profile is available for %1 yet.").arg(p.name));
                    return;
                }
                if (!vth::dialogs::confirm(this,
                        tr("Optimize %1").arg(p.name),
                        tr("Vethida writes a low-overhead settings profile for %1. "
                           "Your current settings are backed up first.").arg(p.name),
                        tr("Optimize"), vth::dialogs::PromptTone::Question))
                    return;
                const gp::WriteResult w = gp::writeSettings(p, rec);
                vth::dialogs::inform(this, tr("Optimize %1").arg(p.name),
                    w.ok ? tr("The profile is applied. A backup was saved to:\n%1")
                               .arg(QDir::toNativeSeparators(w.backupPath))
                         : tr("Vethida could not write the profile.\n\n%1").arg(w.error));
            });

            rowLay->addWidget(optimize, 0, Qt::AlignVCenter);
            rowLay->addWidget(restore, 0, Qt::AlignVCenter);
            rows_->addWidget(row);
        }

        if (found > 0) {
            headline_->setText(found == 1
                ? tr("1 supported game found on this PC.")
                : tr("%1 supported games found on this PC.").arg(found));
            headline_->setVisible(true);
            return;
        }

        // Names the supported titles, since "no supported games" alone reads as a
        // dead end.
        headline_->setVisible(false);

        auto* empty = new QFrame;
        empty->setProperty("uiRole", QStringLiteral("card"));
        empty->setAttribute(Qt::WA_StyledBackground, true);
        auto* col = new QVBoxLayout(empty);
        col->setContentsMargins(0, 0, 0, 0);
        col->setSpacing(0);

        // Names come from the enumeration itself, so this list cannot fall out
        // of step with what the module actually supports.
        QStringList names;
        for (const gp::Profile& p : profiles)
            names << p.name;
        names.removeDuplicates();

        auto* state = new EmptyState(empty);
        state->setTitle(tr("No supported games installed"));
        if (!names.isEmpty())
            state->setSubtitle(tr("Vethida supports %1.")
                                   .arg(names.join(QStringLiteral(", "))));
        col->addWidget(state);

        rows_->addWidget(empty);
    }
}
