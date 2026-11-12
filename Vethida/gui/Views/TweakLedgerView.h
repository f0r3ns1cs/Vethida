#pragma once

#include "../../vth_core/Runtime/Fetch.h"
#include "../../vth_core/Runtime/TweakStateTracker.h"

#include <QHash>
#include <QSet>
#include <QVector>
#include <QWidget>

class QLabel;
class QLineEdit;
class QShowEvent;
class QVBoxLayout;

namespace vth::widgets { class CommandButton; }

namespace vth::ui
{
    class EmptyState;

    // "Changes Vethida made", grouped by session, newest first
    class TweakLedgerView final : public QWidget
    {
        Q_OBJECT

    public:
        bool                          firstShown_ = false;
        explicit TweakLedgerView(QWidget* parent = nullptr);
        ~TweakLedgerView() override;

    protected:
        void showEvent(QShowEvent* event) override;

    private:
        void buildUi();
        void refresh();
        void applyGroups(QVector<vth::TweakBatchGroup> groups);
        void render();
        void renderSessions();
        void renderSearch(const QString& query);
        QWidget* buildSession(const vth::TweakBatchGroup& group);
        QWidget* buildChangeRow(const QString& domain, QWidget* parent);
        void setBusy(bool busy);

        enum class Scope { All, Batch, Domain };
        void requestUndo(Scope scope, const QString& key, const QString& what);

        QLabel*      summary_   = nullptr;
        QLineEdit*   search_    = nullptr;
        QWidget*     listHost_  = nullptr;
        QVBoxLayout* listLay_   = nullptr;
        EmptyState*  empty_     = nullptr;
        QWidget*     emptyWrap_ = nullptr;
        vth::widgets::CommandButton* undoAllBtn_ = nullptr;
        vth::widgets::CommandButton* moreBtn_    = nullptr;

        QVector<vth::TweakBatchGroup> groups_;
        QSet<QString>                 expanded_;
        int                           shown_ = 0;
        bool                          busy_  = false;
        FetchToken                    undoToken_;
        // The ledger read. Behind the tracker's lock, which an undo running on
        // a worker holds for as long as it takes.
        FetchToken                    readToken_;
    };
}

