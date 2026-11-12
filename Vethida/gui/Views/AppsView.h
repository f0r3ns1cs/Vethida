#pragma once

#include "../../vth_core/Windows/Apps.h"

#include <QString>
#include <QVector>
#include <QWidget>

class QAction;
class QLabel;
class QLineEdit;
class QPoint;
class QSortFilterProxyModel;
class QStackedLayout;
class QTreeView;
class QVBoxLayout;

namespace vth::ui::models { class AppListModel; }

namespace vth::ui
{
    class EmptyState;

    // Installed-applications pane: one sortable, filterable row per app.
    // Right-click and double-click both route to a modal AppDetailsDialog.
    class AppsView : public QWidget
    {
        Q_OBJECT

    public:
        explicit AppsView(QWidget* parent = nullptr);
        ~AppsView() override;

    private Q_SLOTS:
        void onSearchChanged(const QString& text);
        void onContextMenu(const QPoint& pos);
        void onShowDetails();
        void onUninstall();
        void onOpenInstallFolder();
        void onRefresh();

    private:
        void buildUi();
        void buildToolBar(QVBoxLayout* into);
        void buildTree   (QVBoxLayout* into);
        void buildFooter (QVBoxLayout* into);

        void reload();
        void applyColumnWidths();
        void refreshFooter();

        // AppInfo for the selected row, or a default-constructed AppInfo when
        // nothing is selected (check displayName.isEmpty()).
        vth::win::apps::AppInfo selectedApp() const;

        QAction*   actionRefresh_  = nullptr;
        QLineEdit* searchEdit_     = nullptr;

        QAction*   actionDetails_   = nullptr;
        QAction*   actionUninstall_ = nullptr;
        QAction*   actionOpenFolder_= nullptr;

        QTreeView*               tree_  = nullptr;
        // The ListCard around tree_. It, not the view, is what the stack holds.
        QWidget*                 list_  = nullptr;
        QStackedLayout*          stack_ = nullptr;
        vth::ui::models::AppListModel* model_ = nullptr;
        QSortFilterProxyModel*   proxy_ = nullptr;

        EmptyState*              loading_ = nullptr;

        QLabel* footerLabel_ = nullptr;
    };
}
