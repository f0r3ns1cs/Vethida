#pragma once

#include "../../vth_core/Windows/Apps.h"

#include <QString>
#include <QVector>
#include <QWidget>

class QLabel;
class QLineEdit;
class QSortFilterProxyModel;
class QStackedLayout;
class QTreeView;
namespace vth::widgets { class CommandButton; }
namespace vth::ui::models { class BloatwareModel; }

namespace vth::ui
{
    class EmptyState;

    // Preinstalled app remover (Debloat tab). Only installed packages are listed,
    // removal runs Remove-AppxPackage on a worker.
    class BloatwareView final : public QWidget
    {
        Q_OBJECT

    public:
        explicit BloatwareView(QWidget* parent = nullptr);
        ~BloatwareView() override;

    private:
        void buildUi();
        void reload();
        // The model finished enumerating: size the columns, show the list.
        void onLoaded();
        // Everything above the tree that depends on what is in it: the two
        // buttons and the footer line. The rows themselves are the model's.
        void applyListState();
        void onRemove();
        // Ticks every Recommended row and hands straight to onRemove(), so both paths
        // share one removal and one confirmation.
        void onRemoveRecommended();
        void updateRemoveEnabled();
        void finishRemoval(int okCount, int accessDenied, int notFound,
                           int otherError, int total);

        QStackedLayout*     stack_   = nullptr;   // loader / list
        EmptyState*         loading_ = nullptr;
        QTreeView*          tree_    = nullptr;
        // The ListCard around tree_. It, not the view, is what the stack holds.
        QWidget*            list_    = nullptr;
        QLineEdit*             search_ = nullptr;
        QSortFilterProxyModel* proxy_  = nullptr;
        vth::ui::models::BloatwareModel* model_ = nullptr;
        vth::widgets::CommandButton*      autoRemove_ = nullptr;
        vth::widgets::CommandButton*      remove_  = nullptr;
        QLabel*             footer_  = nullptr;
        bool removing_ = false;
    };
}
