#pragma once

#include "../../vth_core/Runtime/Fetch.h"
#include "../../vth_core/Windows/Qos.h"

#include <QString>
#include <QVector>
#include <QWidget>

namespace vth::widgets { class CommandButton; }
class QLabel;
class QLineEdit;
class QVBoxLayout;

namespace vth::ui
{
    class Dropdown;

    // QoS rules: give an app priority on the network, or cap what it can use.
    class QosView : public QWidget
    {
        Q_OBJECT

    public:
        explicit QosView(QWidget* parent = nullptr);
        ~QosView() override;

    private:
        void buildUi();
        QWidget* buildComposer();
        QWidget* buildRuleList();

        void reload();
        void rebuildRules();
        void refreshSummary();
        void onPresetChanged(int index);
        void onAddRule();
        void setRuleEnabled(const QString& id, bool on);
        void deleteRule(const QString& id);

        // New-NetQosPolicy / Remove-NetQosPolicy shell out and need elevation, so
        // they never run on the UI thread. `busy` is disabled while in flight.
        void pushToSystem(const vth::win::qos::Policy& policy, bool install, QWidget* busy);

        QLabel*        summaryLabel_ = nullptr;
        Dropdown*      presetSel_    = nullptr;
        QLabel*        presetDescr_  = nullptr;
        QLineEdit*     appEdit_      = nullptr;
        Dropdown*      limitSel_     = nullptr;
        vth::widgets::CommandButton* addBtn_       = nullptr;

        QVBoxLayout* ruleListLay_ = nullptr;
        QLabel*      emptyLabel_  = nullptr;

        QVector<vth::win::qos::Policy> policies_;
        vth::FetchToken systemToken_;
        vth::FetchToken loadToken_;
    };
}
