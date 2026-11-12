#include "pch.h"
#include "AppsView.h"

#include "../Settings/Preferences.h"

#include "../Theme/DesignTokens.h"
#include "../Pages/SystemTools.h"
#include "../../vth_core/Runtime/Tier.h"
#include "../Utils/TablerIcon.h"

#include "../../vth_core/Runtime/Fetch.h"
#include "../../vth_core/Runtime/ServiceRegistry.h"
#include "../../vth_core/Runtime/Threading.h"
#include "../Dialogs/AppDetailsDialog.h"
#include "../Dialogs/Confirm.h"
#include "../Widgets/EmptyState.h"
#include "../Widgets/SectionNav.h"
#include "../Widgets/ListContextMenu.h"
#include "../Widgets/ListView.h"
#include "../Widgets/ToastManager.h"
#include "../Models/AppListModel.h"

#include <QAction>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QItemSelectionModel>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMenu>
#include <QSortFilterProxyModel>
#include <QStackedLayout>
#include <QStringLiteral>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>

namespace vth::ui
{
    namespace
    {
        namespace apps = vth::win::apps;

        constexpr int kColName      = models::AppListModel::ColumnName;
        constexpr int kColPublisher = models::AppListModel::ColumnPublisher;
        constexpr int kColVersion   = models::AppListModel::ColumnVersion;
        constexpr int kColType      = models::AppListModel::ColumnType;

        QString formatSize(quint64 bytes)
        {
            if (bytes == 0) return QStringLiteral("-");
            constexpr quint64 GB = 1024ULL * 1024ULL * 1024ULL;
            constexpr quint64 MB = 1024ULL * 1024ULL;
            constexpr quint64 KB = 1024ULL;
            if (bytes >= GB) {
                return QString::asprintf("%.2f GB",
                    static_cast<double>(bytes) / static_cast<double>(GB));
            }
            if (bytes >= MB) {
                return QString::asprintf("%.0f MB",
                    static_cast<double>(bytes) / static_cast<double>(MB));
            }
            if (bytes >= KB) {
                return QString::asprintf("%.0f KB",
                    static_cast<double>(bytes) / static_cast<double>(KB));
            }
            return QString::number(bytes) + QStringLiteral(" B");
        }

        quint64 totalBytes(const QVector<apps::AppInfo>& list)
        {
            quint64 total = 0;
            for (const auto& a : list) total += a.estimatedBytes;
            return total;
        }
    }

    AppsView::AppsView(QWidget* parent) : QWidget(parent)
    {
        VTH_ASSERT_UI_THREAD();

        qRegisterMetaType<apps::AppInfo>("vth::win::apps::AppInfo");

        setObjectName(QStringLiteral("AppsView"));

        buildUi();
        reload();
    }

    AppsView::~AppsView() = default;

    void AppsView::buildUi()
    {
        // A Self pane, because the tree virtualizes, so it owns its inset.
        auto* layout = new QVBoxLayout(this);
        vth::widgets::SectionNav::applySelfPaneInset(layout);
        layout->setSpacing(vth::theme::tokens::TightGap);

        buildToolBar(layout);
        buildTree   (layout);
        buildFooter (layout);
    }

    void AppsView::buildToolBar(QVBoxLayout* into)
    {
        namespace pal = vth::theme::palette;

        auto* header = new QWidget(this);
        auto* row = new QHBoxLayout(header);
        row->setContentsMargins(vth::theme::tokens::Space2, 0,
                                vth::theme::tokens::Space2, 0);
        row->setSpacing(vth::theme::tokens::CompactGap);

        actionRefresh_ = new QAction(tr("Refresh"), this);
        actionRefresh_->setShortcut(QKeySequence::Refresh);
        connect(actionRefresh_, &QAction::triggered, this, &AppsView::onRefresh);
        addAction(actionRefresh_);

        auto* refreshBtn = new QToolButton(header);
        refreshBtn->setAutoRaise(true);
        refreshBtn->setCursor(Qt::PointingHandCursor);
        refreshBtn->setAccessibleName(tr("Refresh"));
        refreshBtn->setToolTip(tr("Refresh (F5)"));
        vth::icons::applyIconButton(refreshBtn, QStringLiteral("refresh-cw"), &pal::textSecondary, 16);
        connect(refreshBtn, &QToolButton::clicked, this, &AppsView::onRefresh);
        row->addWidget(refreshBtn);

        row->addStretch(1);

        searchEdit_ = new QLineEdit(header);
        searchEdit_->setPlaceholderText(tr("Search by name, publisher, version..."));
        searchEdit_->setAccessibleName(tr("Search installed apps"));
        searchEdit_->setClearButtonEnabled(true);
        searchEdit_->setMinimumWidth(vth::theme::tokens::FilterControlWidth);
        connect(searchEdit_, &QLineEdit::textChanged, this, &AppsView::onSearchChanged);
        row->addWidget(searchEdit_);

        into->addWidget(header);
    }

    void AppsView::buildTree(QVBoxLayout* into)
    {
        auto* host = new QWidget(this);
        stack_ = new QStackedLayout(host);
        stack_->setContentsMargins(0, 0, 0, 0);

        loading_ = new EmptyState(host);
        loading_->setBusy(true);   // loader-only; the list replaces it once loaded
        stack_->addWidget(loading_);

        model_ = new models::AppListModel(this);

        proxy_ = new QSortFilterProxyModel(this);
        proxy_->setSourceModel(model_);
        proxy_->setSortRole(models::AppListModel::SortRole);
        proxy_->setFilterKeyColumn(-1);
        proxy_->setFilterCaseSensitivity(Qt::CaseInsensitive);

        auto* view = new vth::widgets::ListView(host);
        tree_ = view;
        tree_->setModel(proxy_);
        tree_->sortByColumn(kColName, Qt::AscendingOrder);

        tree_->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(tree_, &QTreeView::customContextMenuRequested,
                this, &AppsView::onContextMenu);
        connect(tree_, &QTreeView::doubleClicked, this,
                [this](const QModelIndex&) { onShowDetails(); });

        actionDetails_ = new QAction(
            vth::icons::icon(QStringLiteral("info"), vth::icons::iconColor(), 16),
            tr("&Details..."), this);
        actionDetails_->setShortcut(QKeySequence(tr("Alt+E")));
        actionDetails_->setShortcutContext(Qt::WidgetShortcut);
        connect(actionDetails_, &QAction::triggered,
                this, &AppsView::onShowDetails);
        tree_->addAction(actionDetails_);

        actionUninstall_ = new QAction(
            vth::icons::icon(QStringLiteral("x"), vth::icons::iconColor(), 16),
            tr("&Uninstall..."), this);
        connect(actionUninstall_, &QAction::triggered,
                this, &AppsView::onUninstall);
        tree_->addAction(actionUninstall_);

        actionOpenFolder_ = new QAction(
            vth::icons::icon(QStringLiteral("folder-open"), vth::icons::iconColor(), 16),
            tr("&Open install folder"), this);
        connect(actionOpenFolder_, &QAction::triggered,
                this, &AppsView::onOpenInstallFolder);
        tree_->addAction(actionOpenFolder_);

        listmenu::installShortcuts(tree_, listmenu::Hooks{
            .onQuickFilter = [this] { searchEdit_->setFocus(Qt::ShortcutFocusReason); },
        });

        list_ = new vth::widgets::ListCard(view, host);
        stack_->addWidget(list_);
        stack_->setCurrentWidget(loading_);

        // The model publishes the desktop apps first and appends the Store apps. clear()
        // resets with nothing, so the guard keeps a reload from flashing an empty table.
        connect(model_, &QAbstractItemModel::modelReset, this, [this]() {
            if (model_->rowCount() == 0)
                return;
            applyColumnWidths();
            stack_->setCurrentWidget(list_);
            refreshFooter();
        });
        // The Store half, appended to rows already on screen.
        connect(model_, &QAbstractItemModel::rowsInserted, this,
                [this]() { refreshFooter(); });
        // A machine with nothing to list still has to stop showing the loader.
        connect(model_, &models::AppListModel::loadingChanged, this, [this]() {
            if (model_->loading() || model_->rowCount() > 0)
                return;
            stack_->setCurrentWidget(list_);
            refreshFooter();
        });

        into->addWidget(host, 1);
    }

    // Name takes whatever is left, so the table fits its pane at any window
    // width instead of running off the right edge.
    void AppsView::applyColumnWidths()
    {
        tree_->setColumnWidth(kColPublisher, 220);
        tree_->setColumnWidth(kColVersion,   140);
        tree_->setColumnWidth(kColType,       90);
        if (auto* hdr = tree_->header()) {
            hdr->setStretchLastSection(false);
            hdr->setSectionResizeMode(kColName, QHeaderView::Stretch);
        }
    }

    void AppsView::buildFooter(QVBoxLayout* into)
    {
        auto* footer = new QWidget(this);

        auto* row = new QHBoxLayout(footer);
        row->setContentsMargins(vth::theme::tokens::ListInset, vth::theme::tokens::ListInset,
                                vth::theme::tokens::ListInset, vth::theme::tokens::ListInset);
        row->setSpacing(vth::theme::tokens::CompactGap);

        row->addStretch(1);

        footerLabel_ = new QLabel(footer);
        footerLabel_->setProperty("uiRole", QStringLiteral("metaText"));
        row->addWidget(footerLabel_, 0, Qt::AlignVCenter);

        into->addWidget(footer);
    }

    void AppsView::reload()
    {
        VTH_ASSERT_UI_THREAD();

        stack_->setCurrentWidget(loading_);
        model_->clear();
        refreshFooter();

        model_->reload();
    }

    void AppsView::refreshFooter()
    {
        const QVector<apps::AppInfo>& apps_ = model_->apps();
        const int totalRows    = model_->rowCount();
        const int visibleRows  = proxy_->rowCount();
        const quint64 sumBytes = totalBytes(apps_);

        if (totalRows == 0) {
            footerLabel_->setText(tr("No apps found."));
            return;
        }

        int desktopCount = 0;
        int uwpCount     = 0;
        for (const auto& a : apps_) {
            if (a.type == apps::Type::UWP) ++uwpCount;
            else                            ++desktopCount;
        }

        QString visText;
        if (visibleRows == totalRows) {
            visText = tr("%1 desktop apps, %2 Store apps")
                          .arg(desktopCount).arg(uwpCount);
        } else {
            visText = tr("%1 of %2 apps shown")
                          .arg(visibleRows).arg(totalRows);
        }

        footerLabel_->setText(tr("%1, %2 total")
                                  .arg(visText, formatSize(sumBytes)));
    }

    apps::AppInfo AppsView::selectedApp() const
    {
        const auto sel = tree_->selectionModel()->selectedRows(kColName);
        if (sel.isEmpty()) return {};
        return model_->appAt(proxy_->mapToSource(sel.first()).row());
    }

    void AppsView::onSearchChanged(const QString& text)
    {
        proxy_->setFilterFixedString(text.trimmed());
        refreshFooter();
    }

    void AppsView::onContextMenu(const QPoint& pos)
    {
        const QModelIndex idx = tree_->indexAt(pos);
        QMenu menu(this);

        if (idx.isValid()) {
            tree_->setCurrentIndex(idx);
            menu.addAction(actionDetails_);
            menu.addSeparator();
            menu.addAction(actionUninstall_);
            menu.addAction(actionOpenFolder_);

            const apps::AppInfo app = selectedApp();
            const bool canUninstall =
                (app.type == apps::Type::Desktop &&
                    (!app.uninstallString.isEmpty() ||
                     !app.quietUninstallString.isEmpty()))
                || (app.type == apps::Type::UWP &&
                    !app.packageFullName.isEmpty() && !app.nonRemovable);
            actionUninstall_->setEnabled(canUninstall);
            actionOpenFolder_->setEnabled(!app.installLocation.isEmpty());
        }

        listmenu::appendStandardActions(&menu, tree_, pos);
        menu.exec(tree_->viewport()->mapToGlobal(pos));
    }

    void AppsView::onShowDetails()
    {
        const apps::AppInfo app = selectedApp();
        if (app.displayName.isEmpty()) return;

        if (vth::dialogs::showAppDetails(this, app))
            reload();
    }

    void AppsView::onUninstall()
    {
        const apps::AppInfo app = selectedApp();
        if (app.displayName.isEmpty()) return;
        // Gated before the confirmation, so nobody approves a removal that won't run.
        if (!vth::ui::tools::ensureTier(this, vth::models::ProductTier::Pro))
            return;
        // A desktop app's own uninstaller asks anyway. A Store app is removed outright,
        // so that one always asks.
        const bool ask = app.type == apps::Type::UWP || !vth::prefs::askLess();
        if (ask && !vth::dialogs::confirm(this, tr("Uninstall"),
                tr("Vethida starts the uninstaller for %1. Follow its prompts "
                   "to finish the removal.").arg(app.displayName),
                tr("Uninstall")))
            return;

        // On a worker, because a Store removal runs PowerShell and can take a minute.
        const bool isStore = app.type == apps::Type::UWP;
        const QString name = app.displayName;
        if (auto* toasts = vth::ServiceRegistry::instance().get<vth::widgets::ToastManager>();
            toasts && isStore)
            toasts->info(tr("Uninstalling %1").arg(name), tr("This can take a minute."));
        vth::fetch<bool>(
            this,
            [app]() { return apps::launchUninstall(app); },
            [this, isStore, name](bool started) {
                if (started) {
                    if (isStore)
                        reload();
                    return;
                }
                vth::dialogs::warn(this, tr("Uninstall failed"), isStore
                    ? tr("Windows refused to remove %1. Apps that are part of "
                         "Windows itself cannot be uninstalled.").arg(name)
                    : tr("Vethida could not start the uninstaller for %1. Remove "
                         "it from Settings > Apps instead.").arg(name));
            });
    }

    void AppsView::onOpenInstallFolder()
    {
        const apps::AppInfo app = selectedApp();
        if (app.displayName.isEmpty()) return;
        apps::openInstallLocation(app);
    }

    void AppsView::onRefresh()
    {
        reload();
    }
}
