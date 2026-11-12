#pragma once

#include "../../vth_core/Runtime/Fetch.h"
#include "../../vth_core/SystemRestore.h"

#include <QWidget>

namespace vth::widgets { class CommandButton; }
class QLabel;
class QLineEdit;
class QToolButton;
class QVBoxLayout;
class QFrame;

namespace vth::ui
{
    class EmptyState;

    // Create + list restore points. Offers to turn System Protection on if creating fails
    // because it's off.
    class SystemRestoreView : public QWidget
    {
        Q_OBJECT

    public:
        explicit SystemRestoreView(QWidget* parent = nullptr);
        ~SystemRestoreView() override;

    protected:
        void showEvent(QShowEvent* event) override;

    private Q_SLOTS:
        void onRefresh();
        void onCreate();

    private:
        void buildUi();
        QWidget* buildPointsCard();

        void startCreate(const QString& description, bool bypassThrottle, bool afterEnable);
        void promptEnableThenCreate(const QString& description, const QString& why);
        void promptRestore(const vth::win::sysrestore::RestorePoint& p, vth::widgets::CommandButton* btn);
        void promptDelete(const vth::win::sysrestore::RestorePoint& p);
        void promptDeleteAll();
        void applyPoints(QVector<vth::win::sysrestore::RestorePoint> points);
        void showLoadingPoints();
        void clearPoints();
        void addPointRow(const vth::win::sysrestore::RestorePoint& p, bool isLast);

        QLineEdit*     descEdit_     = nullptr;
        vth::widgets::CommandButton* createBtn_    = nullptr;
        QToolButton*   deleteAllBtn_ = nullptr;
        QToolButton*   refreshBtn_   = nullptr;
        QWidget*       pointsHost_   = nullptr;
        QVBoxLayout*   pointsLayout_ = nullptr;
        QWidget*       emptyWrap_    = nullptr;
        EmptyState*    emptyState_   = nullptr;

        FetchToken refreshToken_;
        bool       firstShown_ = false;
    };
}
