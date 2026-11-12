#pragma once

#include "../../vth_core/Runtime/Fetch.h"
#include "../../vth_core/Windows/Startup.h"

#include <QVector>
#include <QWidget>

class QLabel;
class QLineEdit;
class QSortFilterProxyModel;
class QTreeView;

namespace vth::widgets { class SwitchButton; }
namespace vth::ui::models { class StartupListModel; }

namespace vth::ui
{
    class EmptyState;

    // Everything that launches with Windows, from Run keys and startup folders, in a
    // sortable table with a per-row enable switch.
    class StartupAppsView final : public QWidget
    {
        Q_OBJECT

    public:
        explicit StartupAppsView(QWidget* parent = nullptr);
        ~StartupAppsView() override;

    private:
        void buildUi();
        void reload();
        // Everything above the tree that depends on what's in it.
        void onLoaded();
        // Draws the Status-column switches. Must re-run after any filter change.
        void placeStatusToggles();
        void applyToggle(int sourceRow, bool enabled);
        void noteFirstDisable();
        // Puts a row's switch back to what the model says, without re-entering applyToggle.
        void resetToggle(int sourceRow);
        // Reads the last boot's timings off the event log and writes the header
        // line. Runs on a worker: EvtQuery is IO.
        void refreshBootTime();

        QLabel*                bootLabel_ = nullptr;
        vth::ui::models::StartupListModel* model_ = nullptr;
        QLineEdit*             search_ = nullptr;
        QSortFilterProxyModel* proxy_  = nullptr;
        QTreeView*             tree_   = nullptr;
        // The ListCard around tree_. It, not the view, is what the layout holds,
        // so it is also what gets shown and hidden.
        QWidget*               list_   = nullptr;
        EmptyState*            status_ = nullptr;

        // The switch drawn in each source row's Status cell, or null while filtered out.
        QVector<vth::widgets::SwitchButton*>     toggles_;          // indexed by source row
        FetchToken bootToken_;
    };
}
