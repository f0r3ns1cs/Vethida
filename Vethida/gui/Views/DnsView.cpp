#include "pch.h"
#include "DnsView.h"

#include "../Pages/SystemTools.h"

#include "../../vth_core/Runtime/Threading.h"
#include "../Dialogs/Confirm.h"
#include "../Theme/DesignTokens.h"
#include "../Utils/TablerIcon.h"
#include "../Widgets/CommandButton.h"
#include "../Widgets/Dropdown.h"
#include "../Widgets/ToastManager.h"

#include "../../vth_core/Runtime/ServiceRegistry.h"

#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QRegularExpression>
#include <QToolButton>
#include <QVBoxLayout>

namespace vth::ui
{
    namespace
    {
        namespace nw     = vth::win::network;
        namespace pal    = vth::theme::palette;
        namespace tokens = vth::theme::tokens;

        using vth::win::network::DnsPreset;
        using vth::win::network::dnsPresets;
        using vth::win::network::dnsPresetNameFor;

        bool isValidIpv4(const QString& s)
        {
            if (s.isEmpty()) return false;
            static const QRegularExpression re(
                QStringLiteral(
                    "^(?:25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])"
                    "(?:\\.(?:25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])){3}$"));
            return re.match(s).hasMatch();
        }
    }

    DnsView::DnsView(QWidget* parent) : QWidget(parent)
    {
        VTH_ASSERT_UI_THREAD();
        setObjectName(QStringLiteral("DnsView"));
        buildUi();
        reloadAdapters();
    }

    DnsView::~DnsView()
    {
        adaptersToken_.cancel();
        applyDnsToken_.cancel();
    }

    void DnsView::buildUi()
    {
        auto* outer = new QVBoxLayout(this);
        outer->setContentsMargins(0, 0, 0, 0);
        outer->setSpacing(0);

        auto* card = new QFrame(this);
        card->setProperty("uiRole", QStringLiteral("card"));
        card->setAttribute(Qt::WA_StyledBackground, true);
        form_ = new QFormLayout(card);
        QFormLayout* form = form_;
        form->setContentsMargins(tokens::CardInsetH, tokens::CardInsetV,
                                 tokens::CardInsetH, tokens::CardInsetV);
        form->setHorizontalSpacing(tokens::CardMargin);
        form->setVerticalSpacing(tokens::CardMargin);
        form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

        // Both dropdowns take one fixed width, so their right edges line up.
        adapterCombo_ = new vth::ui::Dropdown;
        adapterCombo_->setAccessibleName(tr("Network adapter"));
        adapterCombo_->setFixedWidth(tokens::FieldWidthWide);
        connect(adapterCombo_, &vth::ui::Dropdown::currentIndexChanged,
                this, &DnsView::onAdapterChanged);

        auto* refresh = new QToolButton;
        refresh->setCursor(Qt::PointingHandCursor);
        refresh->setAccessibleName(tr("Refresh adapters"));
        refresh->setToolTip(tr("Re-read network adapters and current DNS."));
        vth::icons::applyIconButton(refresh, QStringLiteral("refresh-cw"), &pal::textSecondary, 18);
        connect(refresh, &QToolButton::clicked, this, &DnsView::onRefreshAdapters);

        auto* adapterRow = new QWidget;
        auto* adapterLay = new QHBoxLayout(adapterRow);
        adapterLay->setContentsMargins(0, 0, 0, 0);
        adapterLay->setSpacing(tokens::CompactGap);
        adapterLay->addWidget(adapterCombo_, 0);
        adapterLay->addWidget(refresh, 0, Qt::AlignVCenter);
        adapterLay->addStretch(1);

        adapterDescr_ = new QLabel;
        adapterDescr_->setObjectName(QStringLiteral("settingsRowDesc"));
        adapterDescr_->setWordWrap(true);

        currentDnsLabel_ = new QLabel;
        currentDnsLabel_->setProperty("uiRole", QStringLiteral("fieldValue"));
        currentDnsLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);

        presetCombo_ = new vth::ui::Dropdown;
        presetCombo_->setAccessibleName(tr("DNS provider"));
        presetCombo_->setFixedWidth(tokens::FieldWidthWide);
        for (const auto& p : dnsPresets())
            presetCombo_->addItem(p.label);
        presetCombo_->addItem(tr("Automatic (from your router)"));
        presetCombo_->addItem(tr("Custom..."));
        connect(presetCombo_, &vth::ui::Dropdown::currentIndexChanged,
                this, &DnsView::onDnsPresetChanged);

        customRow_ = new QWidget;
        QWidget* customRow = customRow_;
        auto* customLay = new QHBoxLayout(customRow);
        customLay->setContentsMargins(0, 0, 0, 0);
        customLay->setSpacing(tokens::CompactGap);
        primaryEdit_   = new QLineEdit;
        secondaryEdit_ = new QLineEdit;
        primaryEdit_  ->setPlaceholderText(tr("Primary IPv4 (required)"));
        secondaryEdit_->setPlaceholderText(tr("Secondary IPv4 (optional)"));
        primaryEdit_  ->setAccessibleName(tr("Primary IPv4 DNS server (required)"));
        secondaryEdit_->setAccessibleName(tr("Secondary IPv4 DNS server (optional)"));
        customLay->addWidget(primaryEdit_);
        customLay->addWidget(secondaryEdit_);

        applyDnsBtn_ = new vth::widgets::CommandButton(vth::widgets::CommandButton::Variant::Primary, tr("Apply DNS"));
        applyDnsBtn_->setCursor(Qt::PointingHandCursor);
        connect(applyDnsBtn_, &QPushButton::clicked, this, &DnsView::onApplyDnsButton);

        auto* buttonRow = new QWidget;
        auto* buttonLay = new QHBoxLayout(buttonRow);
        buttonLay->setContentsMargins(0, 0, 0, 0);
        buttonLay->addStretch(1);
        buttonLay->addWidget(applyDnsBtn_);

        form->addRow(tr("Adapter"),     adapterRow);
        form->addRow(tr("Hardware"),    adapterDescr_);
        form->addRow(tr("Current DNS"), currentDnsLabel_);
        form->addRow(tr("Preset"),      presetCombo_);
        form->addRow(tr("Custom"),      customRow);
        form->addRow(QString{},         buttonRow);

        // Preset 0 is selected, so the Custom row starts hidden.
        primaryEdit_  ->setEnabled(false);
        secondaryEdit_->setEnabled(false);
        form->setRowVisible(customRow, false);

        outer->addWidget(card);
    }

    void DnsView::reloadAdapters()
    {
        VTH_ASSERT_UI_THREAD();
        adaptersToken_ = vth::fetch<QVector<nw::Adapter>>(
            this,
            []() { return nw::enumerateAdapters(); },
            [this](QVector<nw::Adapter> adapters) {
                VTH_ASSERT_UI_THREAD();
                adapters_ = std::move(adapters);

                const QString prevName = adapterCombo_->currentText();
                QStringList names;
                names.reserve(adapters_.size());
                for (const auto& a : adapters_)
                    names << a.friendlyName;
                // setItems / setCurrentIndex are silent on Dropdown (only a user
                // selection emits), so no blockSignals; render explicitly after.
                adapterCombo_->setItems(names);
                const int restoreIdx = names.indexOf(prevName);
                if (restoreIdx >= 0)
                    adapterCombo_->setCurrentIndex(restoreIdx);
                renderAdapter(adapterCombo_->currentIndex());
            });
    }

    void DnsView::renderAdapter(int index)
    {
        if (index < 0 || index >= adapters_.size()) {
            adapterDescr_   ->setText({});
            currentDnsLabel_->setText({});
            applyDnsBtn_    ->setEnabled(false);
            return;
        }

        const auto& a = adapters_[index];
        adapterDescr_->setText(a.description);

        QString dnsText = a.dnsServers.join(QStringLiteral(", "));
        if (const QString vendor = dnsPresetNameFor(a.dnsServers); !vendor.isEmpty())
            dnsText = tr("%1, %2").arg(vendor, dnsText);
        if (a.dhcp && !dnsText.isEmpty())
            dnsText += tr(" (DHCP)");
        if (dnsText.isEmpty())
            dnsText = tr("(none)");
        currentDnsLabel_->setText(dnsText);
        applyDnsBtn_    ->setEnabled(true);
    }

    void DnsView::onAdapterChanged(int index)
    {
        renderAdapter(index);
    }

    void DnsView::onDnsPresetChanged(int index)
    {
        const int presetCount = dnsPresets().size();
        const bool isCustom = (index == presetCount + 1);
        // setRowVisible takes the "Custom" label with the field.
        primaryEdit_  ->setEnabled(isCustom);
        secondaryEdit_->setEnabled(isCustom);
        if (form_ && customRow_)
            form_->setRowVisible(customRow_, isCustom);
        if (!isCustom) {
            primaryEdit_  ->clear();
            secondaryEdit_->clear();
        }
    }

    void DnsView::onApplyDnsButton()
    {
        VTH_ASSERT_UI_THREAD();
        const int adapterIdx = adapterCombo_->currentIndex();
        if (adapterIdx < 0 || adapterIdx >= adapters_.size()) return;
        const nw::Adapter snapshot = adapters_[adapterIdx];
        const QString name = snapshot.friendlyName;

        const int presetIdx = presetCombo_->currentIndex();
        const int presetCount = dnsPresets().size();

        // Going back to DHCP is never gated, so a lapsed license can't leave someone stuck
        // on a resolver Vethida set.
        if (presetIdx != presetCount
            && !vth::ui::tools::ensureTier(this, vth::models::ProductTier::Pro))
            return;

        struct Request
        {
            bool    useDhcp = false;
            QString primary;
            QString secondary;
            QString primaryV6;
            QString secondaryV6;
        } req;

        if (presetIdx < presetCount) {
            const auto& p = dnsPresets()[presetIdx];
            req.primary     = p.primary;
            req.secondary   = p.secondary;
            req.primaryV6   = p.primaryV6;
            req.secondaryV6 = p.secondaryV6;
        } else if (presetIdx == presetCount) {
            req.useDhcp = true;
        } else {
            const QString primary   = primaryEdit_  ->text().trimmed();
            const QString secondary = secondaryEdit_->text().trimmed();
            if (!isValidIpv4(primary)) {
                vth::dialogs::warn(this, tr("Invalid DNS address"),
                    tr("Primary DNS must be a valid IPv4 address."));
                return;
            }
            if (!secondary.isEmpty() && !isValidIpv4(secondary)) {
                vth::dialogs::warn(this, tr("Invalid DNS address"),
                    tr("Secondary DNS must be a valid IPv4 address, or empty."));
                return;
            }
            req.primary   = primary;
            req.secondary = secondary;
        }

        if (applyDnsBtn_) applyDnsBtn_->setLoading(true);

        applyDnsToken_ = vth::fetch<nw::DnsResult>(
            this,
            [name, req]() {
                return req.useDhcp
                    ? nw::setDhcpDns(name)
                    : nw::setStaticDns(name, req.primary, req.secondary,
                                              req.primaryV6, req.secondaryV6);
            },
            [this, name](nw::DnsResult result) {
                if (applyDnsBtn_) applyDnsBtn_->setLoading(false);

                if (result == nw::DnsResult::Ok) {
                    if (auto* toasts = vth::ServiceRegistry::instance()
                                           .get<vth::widgets::ToastManager>())
                        toasts->success(tr("DNS"), tr("DNS updated on %1.").arg(name));
                    reloadAdapters();
                } else {
                    vth::dialogs::warn(this, tr("DNS not changed"),
                                       nw::dnsResultLabel(result));
                }
            });
    }

    void DnsView::onRefreshAdapters()
    {
        reloadAdapters();
    }
}
