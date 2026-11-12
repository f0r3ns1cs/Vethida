#include "pch.h"
#include "BloatwareView.h"

#include "../../vth_core/Windows/Privileges.h"

#include "../Dialogs/Confirm.h"
#include "../Pages/SystemTools.h"
#include "../Theme/DesignTokens.h"
#include "../Utils/TablerIcon.h"
#include "../Widgets/CommandButton.h"
#include "../Widgets/EmptyState.h"
#include "../Widgets/ListContextMenu.h"
#include "../Widgets/ListView.h"
#include "../Models/BloatwareModel.h"

#include "../../vth_core/Logger.h"
#include "../../vth_core/Models/ProductTier.h"
#include "../../vth_core/Runtime/Dispatcher.h"
#include "../../vth_core/Runtime/ServiceRegistry.h"
#include "../../vth_core/Runtime/Settings.h"
#include "../../vth_core/Runtime/Threading.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStackedLayout>
#include <QStringLiteral>
#include <QTreeView>
#include <QVBoxLayout>

namespace vth::ui
{
    namespace
    {
        namespace apps = vth::win::apps;

        VTH_DEFINE_LOG_CATEGORY(catDebloat, "vth.debloat")

        namespace tokens = vth::theme::tokens;
    }

    BloatwareView::BloatwareView(QWidget* parent) : QWidget(parent)
    {
        VTH_ASSERT_UI_THREAD();

        setObjectName(QStringLiteral("bloatwareView"));
        buildUi();
        reload();
    }

    BloatwareView::~BloatwareView() = default;

    void BloatwareView::buildUi()
    {
        // A Self pane, because the tree virtualizes, so it owns its inset.
        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(tokens::PageMarginH, tokens::PageMarginTop,
                                 tokens::PageMarginH, tokens::PageMarginBottom);
        root->setSpacing(tokens::CardMargin);

        auto* headerRow = new QHBoxLayout;
        headerRow->setContentsMargins(0, 0, 0, 0);
        headerRow->setSpacing(vth::theme::tokens::CompactGap);
        headerRow->addStretch(1);
        search_ = new QLineEdit(this);
        search_->setPlaceholderText(tr("Search preinstalled apps"));
        search_->setAccessibleName(tr("Search preinstalled apps"));
        search_->setClearButtonEnabled(true);
        search_->setMinimumWidth(tokens::FilterControlWidth);
        headerRow->addWidget(search_);
        root->addLayout(headerRow);

        auto* host = new QWidget(this);
        stack_ = new QStackedLayout(host);
        stack_->setContentsMargins(0, 0, 0, 0);

        loading_ = new EmptyState(host);
        loading_->setBusy(true);   // loader-only; the list replaces it once loaded

        model_ = new models::BloatwareModel(this);
        connect(model_, &models::BloatwareModel::loadingChanged, this, [this]() {
            if (!model_->loading())
                onLoaded();
        });

        // A grouped checkbox tree with no row selection or sorting. Recursive filtering
        // keeps a group whose children match, so results stay under their headings.
        connect(model_, &models::BloatwareModel::checkedChanged,
                this, &BloatwareView::applyListState);

        proxy_ = new QSortFilterProxyModel(this);
        proxy_->setSourceModel(model_);
        proxy_->setRecursiveFilteringEnabled(true);
        proxy_->setFilterKeyColumn(-1);
        proxy_->setFilterCaseSensitivity(Qt::CaseInsensitive);
        connect(search_, &QLineEdit::textChanged, this, [this](const QString& text) {
            proxy_->setFilterFixedString(text);
            tree_->expandAll();   // a filtered group is useless collapsed
        });

        auto* view = new vth::widgets::ListView(host);
        tree_ = view;
        tree_->setModel(proxy_);
        tree_->setRootIsDecorated(true);
        tree_->setSelectionMode(QAbstractItemView::NoSelection);
        tree_->setSortingEnabled(false);
        // These columns do not sort, so the header does not claim to be clickable.
        tree_->header()->setCursor(Qt::ArrowCursor);
        tree_->header()->setStretchLastSection(true);
        tree_->header()->setSectionResizeMode(0, QHeaderView::Interactive);
        tree_->header()->setSectionResizeMode(1, QHeaderView::Interactive);
        // descriptions wrap (the list is small)
        tree_->setWordWrap(true);
        tree_->setUniformRowHeights(false);
        tree_->setTextElideMode(Qt::ElideNone);

        // After setSelectionMode, which the helper reads to decide whether Copy applies.
        listmenu::installShortcuts(tree_, listmenu::Hooks{
            .onQuickFilter = [this] { search_->setFocus(Qt::ShortcutFocusReason); },
        });

        list_ = new vth::widgets::ListCard(view, host);

        stack_->addWidget(loading_);
        stack_->addWidget(list_);
        stack_->setCurrentWidget(loading_);
        root->addWidget(host, 1);

        auto* footerRow = new QHBoxLayout;
        footerRow->setContentsMargins(0, 0, 0, 0);
        footerRow->setSpacing(tokens::CompactGap);
        footer_ = new QLabel(this);
        footer_->setProperty("uiRole", QStringLiteral("metaText"));
        footerRow->addWidget(footer_, 1, Qt::AlignVCenter);

        // secondary, the primary button is "remove selected"
        autoRemove_ = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Secondary,
            tr("Remove recommended"), this);
        autoRemove_->setCursor(Qt::PointingHandCursor);
        autoRemove_->setEnabled(false);
        connect(autoRemove_, &QPushButton::clicked, this,
                &BloatwareView::onRemoveRecommended);
        footerRow->addWidget(autoRemove_, 0, Qt::AlignVCenter);

        remove_ = new vth::widgets::CommandButton(vth::widgets::CommandButton::Variant::Primary,
                                    tr("Remove selected"), this);
        remove_->setCursor(Qt::PointingHandCursor);
        remove_->setEnabled(false);
        connect(remove_, &QPushButton::clicked, this, &BloatwareView::onRemove);
        footerRow->addWidget(remove_, 0, Qt::AlignVCenter);
        root->addLayout(footerRow);
    }

    void BloatwareView::reload()
    {
        stack_->setCurrentWidget(loading_);
        remove_->setEnabled(false);
        autoRemove_->setEnabled(false);
        footer_->clear();

        model_->reload();
    }

    void BloatwareView::onLoaded()
    {
        tree_->expandAll();
        tree_->setColumnWidth(0, 240);
        tree_->setColumnWidth(1, 220);
        stack_->setCurrentWidget(list_);
        applyListState();
    }

    // Everything above the tree that depends on what is in it.
    void BloatwareView::applyListState()
    {
        const int total       = model_->appCount();
        const int recommended = model_->recommendedCount();

        remove_->setEnabled(!removing_ && model_->anyChecked());

        // Disabled rather than hidden, because a control that comes and goes between
        // machines reads as a bug.
        autoRemove_->setEnabled(!removing_ && recommended > 0);
        autoRemove_->setToolTip(recommended > 0
            ? (recommended == 1
                ? tr("Ticks the one app under Recommended, then asks you to confirm.")
                : tr("Ticks the %1 apps under Recommended, then asks you to confirm.")
                      .arg(recommended))
            : tr("None of the apps Vethida recommends removing are installed here."));

        footer_->setText(total == 0
            ? tr("No removable preinstalled apps found on this PC.")
            : total == 1
                ? tr("1 removable preinstalled app found.")
                : tr("%1 removable preinstalled apps found.").arg(total));
    }

    void BloatwareView::onRemoveRecommended()
    {
        if (removing_)
            return;
        if (!vth::ui::tools::ensureTier(this, vth::models::ProductTier::Pro))
            return;

        if (model_->checkRecommended() == 0) {
            vth::dialogs::inform(this, tr("Nothing to remove"),
                tr("None of the apps Vethida recommends removing are installed "
                   "on this PC."));
            return;
        }

        // keep the selection if they cancel
        onRemove();
    }

    void BloatwareView::onRemove()
    {
        if (removing_)
            return;
        if (!vth::ui::tools::ensureTier(this, vth::models::ProductTier::Pro))
            return;

        const QVector<models::BloatwareModel::Selected> selected = model_->checkedApps();

        if (selected.isEmpty()) {
            vth::dialogs::inform(
                this,
                tr("No apps checked"),
                tr("Tick at least one app to remove."));
            return;
        }

        QStringList selectedNames;
        QVector<QString> fullNames;
        selectedNames.reserve(selected.size());
        fullNames.reserve(selected.size());
        for (const auto& s : selected) {
            selectedNames.append(s.displayName);
            fullNames.append(s.packageFullName);
        }

        const QString headline = (selected.size() == 1)
            ? tr("Remove 1 app")
            : tr("Remove %1 apps").arg(selected.size());

        QString detail;
        constexpr int kPreviewLimit = 8;
        if (selectedNames.size() <= kPreviewLimit) {
            detail = tr(
                "These apps will be removed:\n\n"
                "    - %1\n\n"
                "This cannot be undone.")
                .arg(selectedNames.join(QStringLiteral("\n    - ")));
        } else {
            const QStringList preview = selectedNames.mid(0, kPreviewLimit - 1);
            const int rest = selectedNames.size() - preview.size();
            detail = tr(
                "These apps will be removed:\n\n"
                "    - %1\n"
                "    - and %2 more\n\n"
                "This cannot be undone.")
                .arg(preview.join(QStringLiteral("\n    - "))).arg(rest);
        }

        if (!vth::dialogs::confirm(this, headline, detail, tr("Remove"),
                                   vth::dialogs::PromptTone::Warning)) {
            return;
        }

        // Each removal shells out to PowerShell and can take seconds, so the
        // batch runs on a worker; the view stays responsive and shows state.
        removing_ = true;
        tree_->setEnabled(false);
        remove_->setLoading(true);
        autoRemove_->setEnabled(false);

        QPointer<BloatwareView> guard(this);
        vth::Dispatcher::instance().dispatch([guard, fullNames]() {
            int ok = 0, accessDenied = 0, notFound = 0, otherError = 0;
            for (const QString& fullName : fullNames) {
                const auto result = apps::removeUwpPackage(fullName);
                switch (result) {
                    case apps::UwpRemoveResult::Ok:           ++ok;           break;
                    case apps::UwpRemoveResult::AccessDenied: ++accessDenied; break;
                    case apps::UwpRemoveResult::NotFound:     ++notFound;     break;
                    default:                                  ++otherError;   break;
                }
                if (result != apps::UwpRemoveResult::Ok) {
                    qCWarning(catDebloat).noquote()
                        << "Remove-AppxPackage" << fullName
                        << "-" << apps::uwpRemoveResultLabel(result);
                }
            }
            const int total = static_cast<int>(fullNames.size());
            vth::Dispatcher::instance().dispatchUi(
                [guard, ok, accessDenied, notFound, otherError, total]() {
                    if (guard)
                        guard->finishRemoval(ok, accessDenied, notFound, otherError, total);
                });
        });
    }

    void BloatwareView::finishRemoval(int okCount, int accessDenied, int notFound,
                                      int otherError, int total)
    {
        removing_ = false;
        tree_->setEnabled(true);
        remove_->setLoading(false);
        // The reload below sets the real state when it lands. This covers no reload.
        applyListState();

        QString summary = total == 1
            ? (okCount == 1 ? tr("The app was removed.") : tr("The app was not removed."))
            : tr("Removed %1 of %2 apps.").arg(okCount).arg(total);
        if (accessDenied > 0) {
            summary += QStringLiteral("\n\n");
            if (vth::win::isElevated()) {
                // Elevated and still refused: Windows marks the package as part
                // of the system, and no amount of rights removes it per user.
                summary += accessDenied == 1
                    ? tr("Windows refused one removal: it treats that app as part of the system.")
                    : tr("Windows refused %1 removals: it treats those apps as part of the system.")
                          .arg(accessDenied);
            } else {
                summary += accessDenied == 1
                    ? tr("One needs administrator rights.")
                    : tr("%1 need administrator rights.").arg(accessDenied);
                summary += QStringLiteral(" ");
                summary += tr("Close Vethida, open it again, and allow it when Windows asks.");
            }
        }
        if (notFound > 0) {
            summary += QStringLiteral("\n");
            summary += (notFound == 1
                ? tr("One was already removed.")
                : tr("%1 were already removed.").arg(notFound));
        }
        if (otherError > 0) {
            summary += QStringLiteral("\n");
            summary += (otherError == 1
                ? tr("One failed with a Windows error.")
                : tr("%1 failed with a Windows error.").arg(otherError));
        }

        if (accessDenied > 0 || otherError > 0) {
            vth::dialogs::warn(this, tr("Some apps not removed"), summary);
        } else {
            vth::dialogs::inform(this, tr("Apps removed"), summary);
        }

        // The catalog just changed under us; re-enumerate so removed rows go
        // away instead of sitting there checked and already gone.
        if (okCount > 0) {
            reload();
        }
    }
}
