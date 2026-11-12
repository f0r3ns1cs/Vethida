#pragma once

#include "../../vth_core/Runtime/Fetch.h"
#include "../../vth_core/Windows/Network.h"

#include <QVector>
#include <QWidget>

namespace vth::widgets { class CommandButton; }
class QFormLayout;
class QLabel;
class QLineEdit;

namespace vth::ui
{
    class Dropdown;

    // per-adapter DNS: presets, DHCP, or a custom pair
    class DnsView : public QWidget
    {
        Q_OBJECT

    public:
        explicit DnsView(QWidget* parent = nullptr);
        ~DnsView() override;

    private Q_SLOTS:
        void onAdapterChanged(int index);
        void onDnsPresetChanged(int index);
        void onApplyDnsButton();
        void onRefreshAdapters();

    private:
        void buildUi();
        void reloadAdapters();
        void renderAdapter(int index);

        Dropdown*      adapterCombo_    = nullptr;
        QLabel*        adapterDescr_    = nullptr;
        QLabel*        currentDnsLabel_ = nullptr;
        Dropdown*      presetCombo_     = nullptr;
        QLineEdit*     primaryEdit_     = nullptr;
        QLineEdit*     secondaryEdit_   = nullptr;
        // Kept so the Custom row can be hidden with its label when a preset is
        // chosen; QFormLayout::setRowVisible needs both.
        QFormLayout*   form_            = nullptr;
        QWidget*       customRow_       = nullptr;
        vth::widgets::CommandButton* applyDnsBtn_     = nullptr;

        QVector<vth::win::network::Adapter> adapters_;

        FetchToken adaptersToken_;
        FetchToken applyDnsToken_;
    };
}
