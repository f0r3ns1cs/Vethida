#include "pch.h"
#include "StartupAppsView.h"

#include "../../vth_core/Windows/Privileges.h"

#include "../Dialogs/Confirm.h"
#include "../Dialogs/StartupEntryDetails.h"
#include "../Pages/SystemTools.h"
#include "../Theme/DesignTokens.h"
#include "../../vth_core/Runtime/Tier.h"
#include "../Utils/TablerIcon.h"
#include "../Widgets/EmptyState.h"
#include "../Widgets/GlyphLabel.h"
#include "../Widgets/ListContextMenu.h"
#include "../Widgets/SectionNav.h"
#include "../Widgets/ListView.h"
#include "../Widgets/SwitchButton.h"
#include "../Widgets/ToastManager.h"
#include "../Settings/Preferences.h"
#include "../../vth_core/Runtime/ServiceRegistry.h"
#include "../../vth_core/Runtime/Settings.h"
#include "../Models/StartupListModel.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QSortFilterProxyModel>
#include <QTreeView>
#include <QVBoxLayout>

namespace vth::ui
{
    namespace
    {
        namespace su = vth::win::startup;

        constexpr int kColApp     = models::StartupListModel::ColumnApp;
        constexpr int kColCompany = models::StartupListModel::ColumnCompany;
        constexpr int kColFile    = models::StartupListModel::ColumnFile;
        constexpr int kColType    = models::StartupListModel::ColumnType;
        constexpr int kColStatus  = models::StartupListModel::ColumnStatus;
    }

    StartupAppsView::StartupAppsView(QWidget* parent)
        : QWidget(parent)
    {
        setObjectName(QStringLiteral("startupApplicationsPage"));
        buildUi();
        reload();
    }

    StartupAppsView::~StartupAppsView()
    {
        bootToken_.cancel();
    }

    void StartupAppsView::refreshBootTime()
    {
        if (!bootLabel_)
            return;
        bootLabel_->setText(tr("Reading last boot..."));

        bootToken_ = vth::fetch<vth::win::startup::BootMetrics>(
            this,
            []() { return vth::win::startup::bootMetrics(); },
            [this](vth::win::startup::BootMetrics m) {
                if (!bootLabel_)
                    return;
                if (!m.valid) {
                    // Windows only writes Event 100 on a full boot, so this can be empty.
                    bootLabel_->clear();
                    return;
                }
                // Seconds to one decimal. Milliseconds on a number that varies
                // by hundreds between identical boots is false precision.
                const double total = m.bootTimeMs / 1000.0;
                if (m.mainPathBootMs > 0) {
                    bootLabel_->setText(
                        tr("Last boot %1 s, %2 s of it starting these.")
                            .arg(total, 0, 'f', 1)
                            .arg(m.postBootMs / 1000.0, 0, 'f', 1));
                } else {
                    bootLabel_->setText(tr("Last boot %1 s.").arg(total, 0, 'f', 1));
                }
            });
    }

    void StartupAppsView::buildUi()
    {
        // A Self pane, because the table virtualizes, so it owns its inset.
        auto* root = new QVBoxLayout(this);
        vth::widgets::SectionNav::applySelfPaneInset(root);
        root->setSpacing(vth::theme::tokens::CardMargin);

        auto* header = new QWidget(this);
        auto* headerRow = new QHBoxLayout(header);
        headerRow->setContentsMargins(0, 0, 0, 0);
        headerRow->setSpacing(vth::theme::tokens::CompactGap);

        // Last boot time, on the left of the same row.
        bootLabel_ = new QLabel(header);
        bootLabel_->setProperty("uiRole", QStringLiteral("metaText"));
        headerRow->addWidget(bootLabel_, 0, Qt::AlignVCenter);
        refreshBootTime();

        headerRow->addStretch(1);

        search_ = new QLineEdit(header);
        search_->setPlaceholderText(tr("Search startup apps"));
        search_->setAccessibleName(tr("Search startup apps"));
        search_->setClearButtonEnabled(true);
        search_->setMinimumWidth(vth::theme::tokens::FilterControlWidth);
        headerRow->addWidget(search_);
        root->addWidget(header);

        model_ = new models::StartupListModel(this);
        connect(model_, &models::StartupListModel::loadingChanged, this, [this]() {
            if (!model_->loading())
                onLoaded();
        });

        proxy_ = new QSortFilterProxyModel(this);
        proxy_->setSourceModel(model_);
        proxy_->setSortRole(models::StartupListModel::SortRole);
        // Every column, so a search hits the publisher and the file name too,
        // not just the display name.
        proxy_->setFilterKeyColumn(-1);
        proxy_->setFilterCaseSensitivity(Qt::CaseInsensitive);
        connect(search_, &QLineEdit::textChanged, this, [this](const QString& text) {
            proxy_->setFilterFixedString(text);
            placeStatusToggles();   // filtering destroys them
        });

        auto* view = new vth::widgets::ListView(this);
        tree_ = view;
        tree_->setModel(proxy_);
        tree_->sortByColumn(kColApp, Qt::AscendingOrder);

        listmenu::installShortcuts(tree_, listmenu::Hooks{
            .onQuickFilter = [this] { search_->setFocus(Qt::ShortcutFocusReason); },
        });

        list_ = new vth::widgets::ListCard(view, this);
        root->addWidget(list_, 1);

        connect(tree_, &QTreeView::doubleClicked, this, [this](const QModelIndex& idx) {
            const int sourceRow = proxy_->mapToSource(idx).row();
            const su::StartupEntry entry = model_->entryAt(sourceRow);
            if (entry.id.isEmpty())
                return;
            const auto newState = vth::dialogs::showStartupEntryDetails(this, entry);
            if (newState && *newState != entry.enabled) {
                // through the switch if the row is visible
                if (auto* toggle = toggles_.value(sourceRow))
                    toggle->setChecked(*newState);
                else
                    applyToggle(sourceRow, *newState);
            }
        });

        status_ = new EmptyState(this);
        root->addWidget(status_, 1);
    }

    void StartupAppsView::reload()
    {
        model_->clear();
        toggles_.clear();

        list_->setVisible(false);
        status_->setVisible(true);
        status_->setBusy(true);   // loader-only; result message set in onLoaded

        model_->reload();
    }

    void StartupAppsView::onLoaded()
    {
        toggles_.assign(model_->rowCount(), nullptr);

        if (model_->rowCount() == 0) {
            list_->setVisible(false);
            status_->setVisible(true);
            status_->setBusy(false);
            status_->setTitle(tr("No startup entries found"));
            return;
        }

        status_->setVisible(false);
        list_->setVisible(true);

        placeStatusToggles();

        // The name takes whatever width is left, as on Installed apps, so the
        // table fills the pane instead of stopping two-thirds of the way.
        tree_->header()->setSectionResizeMode(kColApp, QHeaderView::Stretch);
        tree_->setColumnWidth(kColCompany, 190);
        tree_->setColumnWidth(kColFile,    200);
        tree_->setColumnWidth(kColType,    180);
        tree_->setColumnWidth(kColStatus,   90);
    }

    // Per-row enable switch in the Status column, as an index widget.
    void StartupAppsView::placeStatusToggles()
    {
        // Sized, because operator[] doesn't grow a QVector.
        toggles_.assign(model_->rowCount(), nullptr);
        for (int srcRow = 0; srcRow < model_->rowCount(); ++srcRow) {
            const QModelIndex source = model_->index(srcRow, kColStatus);
            const QModelIndex idx    = proxy_->mapFromSource(source);
            if (!idx.isValid())
                continue;   // filtered out; it gets a fresh widget when it returns

            auto* host = new QWidget(tree_);
            auto* hl = new QHBoxLayout(host);
            hl->setContentsMargins(0, 0, vth::theme::tokens::CompactGap, 0);
            hl->setSpacing(vth::theme::tokens::CompactGap);
            hl->addStretch(1);
            // value in both the live key and the mirror = an earlier disable didn't finish
            if (source.data(models::StartupListModel::StaleMirrorRole).toBool()) {
                auto* mark = new vth::widgets::GlyphLabel(
                    QStringLiteral("alert-triangle"),
                    &vth::theme::palette::warningText,
                    vth::theme::tokens::GlyphMd, host);
                mark->setToolTip(su::staleMirrorNotice());
                mark->setCursor(Qt::PointingHandCursor);
                hl->addWidget(mark, 0, Qt::AlignVCenter);
            }
            auto* toggle = new vth::widgets::SwitchButton(host);
            toggle->setChecked(source.data(models::StartupListModel::EnabledRole).toBool());
            toggle->setAccessibleName(model_->entryAt(srcRow).displayName);
            hl->addWidget(toggle, 0, Qt::AlignVCenter);
            tree_->setIndexWidget(idx, host);

            toggles_[srcRow] = toggle;
            connect(toggle, &QAbstractButton::toggled, this,
                    [this, srcRow](bool on) { applyToggle(srcRow, on); });
        }
    }

    void StartupAppsView::resetToggle(int sourceRow)
    {
        auto* toggle = toggles_.value(sourceRow);
        if (!toggle)
            return;
        // Blocked, so putting the switch back does not run applyToggle again.
        QSignalBlocker block(toggle);
        toggle->setChecked(model_->entryAt(sourceRow).enabled);
    }

    // one-time note after the first disable (it only takes effect at next sign-in)
    void StartupAppsView::noteFirstDisable()
    {
        auto* settings = vth::ServiceRegistry::instance().get<vth::Settings>();
        const QString key = QStringLiteral("tips/startupFirstDisable");
        if (!vth::prefs::showTips() || !settings || settings->value(key, false).toBool())
            return;
        settings->setValue(key, true);
        if (auto* toasts = vth::ServiceRegistry::instance().get<vth::widgets::ToastManager>())
            toasts->info(tr("Turned off at startup"),
                         tr("It won't start with Windows from your next sign-in. If something "
                            "stops working, switch it back on here."));
    }

    void StartupAppsView::applyToggle(int sourceRow, bool enabled)
    {
        const su::StartupEntry entry = model_->entryAt(sourceRow);
        if (entry.id.isEmpty() || entry.enabled == enabled)
            return;

        // Free lists startup entries; changing one is Pro. The switch has
        // already moved by the time this runs, so it is put back.
        if (!vth::ui::tools::ensureTier(this, vth::models::ProductTier::Pro)) {
            resetToggle(sourceRow);
            return;
        }

        vth::fetch<su::ActionResult>(
            this,
            [entry, enabled]() { return su::setEnabled(entry, enabled); },
            [this, sourceRow, enabled](su::ActionResult result) {
                if (result == su::ActionResult::Ok) {
                    // the views update from the model (re-sorts by status too)
                    model_->setEnabled(sourceRow, enabled);
                    if (!enabled)
                        noteFirstDisable();
                    return;
                }

                resetToggle(sourceRow);

                const QString detail = result != su::ActionResult::AccessDenied
                    ? su::actionResultLabel(result)
                    : vth::win::isElevated()
                        ? tr("Windows refused the change. The program that owns this entry, or "
                             "security software, is protecting it.")
                        : tr("That change needs administrator rights. Close Vethida, open it "
                             "again, and allow it when Windows asks.");
                vth::dialogs::warn(this, tr("Startup entry not changed"), detail);
            });
    }
}
