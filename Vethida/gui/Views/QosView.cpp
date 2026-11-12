#include "pch.h"
#include "QosView.h"

#include "../Settings/Preferences.h"

#include "../../vth_core/Windows/Privileges.h"

#include "../../vth_core/Runtime/Threading.h"
#include "../Dialogs/Confirm.h"
#include "../Theme/DesignTokens.h"
#include "../Utils/TablerIcon.h"
#include "../Widgets/CommandButton.h"
#include "../Widgets/Dropdown.h"
#include "../Widgets/GlyphLabel.h"
#include "../Widgets/SwitchButton.h"
#include "../Widgets/TierLockOverlay.h"

#include "../../vth_core/Models/ProductTier.h"
#include "../../vth_core/Runtime/Tier.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>   // std::find_if
#include <iterator>    // std::size
#include <QHash>

namespace vth::ui
{
    namespace
    {
        namespace q      = vth::win::qos;
        namespace pal    = vth::theme::palette;
        namespace tokens = vth::theme::tokens;

        // The bandwidth caps offered in the composer.
        constexpr int kLimitPickerWidth = 120;

        struct LimitChoice { const char* label; const char* rate; };
        const LimitChoice kLimits[] = {
            { QT_TRANSLATE_NOOP("QosView", "No limit"),  "Unlimited" },
            { QT_TRANSLATE_NOOP("QosView", "1 Mbps"),    "1 Mbps"    },
            { QT_TRANSLATE_NOOP("QosView", "5 Mbps"),    "5 Mbps"    },
            { QT_TRANSLATE_NOOP("QosView", "10 Mbps"),   "10 Mbps"   },
            { QT_TRANSLATE_NOOP("QosView", "25 Mbps"),   "25 Mbps"   },
            { QT_TRANSLATE_NOOP("QosView", "50 Mbps"),   "50 Mbps"   },
            { QT_TRANSLATE_NOOP("QosView", "100 Mbps"),  "100 Mbps"  },
        };

        bool isLimited(const q::Policy& p)
        {
            return q::parseThrottleMbps(p.throttleRate) > 0.0;
        }

        // one-line summary, no DSCP
        QString ruleSummary(const q::Policy& p)
        {
            const QString what = isLimited(p)
                ? QosView::tr("Limited to %1").arg(p.throttleRate)
                : QosView::tr("Prioritized");
            return QosView::tr("%1, %2, %3")
                .arg(what, q::directionLabel(p.direction), q::protocolLabel(p.protocol));
        }
    }

    QosView::QosView(QWidget* parent) : QWidget(parent)
    {
        VTH_ASSERT_UI_THREAD();
        setObjectName(QStringLiteral("QosView"));
        buildUi();
        reload();

        if (!vth::meetsTier(vth::models::ProductTier::Pro)) {
            auto* lock = new vth::widgets::TierLockOverlay(
                this, vth::models::ProductTier::Pro);
            lock->setFeatureName(tr("Traffic priority"));
            lock->show();
            lock->raise();
        }
    }

    QosView::~QosView()
    {
        systemToken_.cancel();
        loadToken_.cancel();
    }

    void QosView::buildUi()
    {
        // A Managed pane, because the Network rail supplies the scroll area and inset.
        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(0, 0, 0, 0);
        root->setSpacing(tokens::CardMargin);

        summaryLabel_ = new QLabel(this);
        summaryLabel_->setObjectName(QStringLiteral("settingsRowDesc"));
        root->addWidget(summaryLabel_);

        root->addWidget(buildComposer());
        root->addWidget(buildRuleList());
    }

    QWidget* QosView::buildComposer()
    {
        auto* card = new QFrame(this);
        card->setObjectName(QStringLiteral("sectionCard"));
        card->setAttribute(Qt::WA_StyledBackground, true);
        auto* col = new QVBoxLayout(card);
        col->setContentsMargins(tokens::CardInsetH, tokens::CardInsetV,
                                tokens::CardInsetH, tokens::CardInsetV);
        col->setSpacing(tokens::CardMargin);

        auto* heading = new QLabel(tr("Add a rule"), card);
        heading->setProperty("uiRole", QStringLiteral("sectionTitle"));
        col->addWidget(heading);

        // Row 1: what kind of rule. The preset supplies direction, protocol and
        // DSCP, so none of the three is asked for separately.
        presetSel_ = new Dropdown(card);
        presetSel_->setAccessibleName(tr("Rule type"));
        for (const q::Preset& p : q::presets())
            presetSel_->addItem(p.name);
        presetSel_->setCurrentIndex(0);
        connect(presetSel_, &Dropdown::currentIndexChanged,
                this, &QosView::onPresetChanged);
        col->addWidget(presetSel_);

        presetDescr_ = new QLabel(card);
        presetDescr_->setObjectName(QStringLiteral("settingsRowDesc"));
        presetDescr_->setWordWrap(true);
        col->addWidget(presetDescr_);

        // Row 2: which app, and an optional cap, then the single verb.
        auto* row = new QHBoxLayout;
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(tokens::CompactGap);

        appEdit_ = new QLineEdit(card);
        appEdit_->setPlaceholderText(tr("Application name, or leave blank for all"));
        appEdit_->setAccessibleName(tr("Application to prioritize"));
        row->addWidget(appEdit_, 1);

        limitSel_ = new Dropdown(card);
        limitSel_->setAccessibleName(tr("Bandwidth limit"));
        for (const LimitChoice& l : kLimits)
            limitSel_->addItem(tr(l.label));
        limitSel_->setCurrentIndex(0);
        limitSel_->setMinimumWidth(kLimitPickerWidth);
        row->addWidget(limitSel_, 0);

        addBtn_ = new vth::widgets::CommandButton(vth::widgets::CommandButton::Variant::Primary, tr("Add rule"), card);
        addBtn_->setCursor(Qt::PointingHandCursor);
        connect(addBtn_, &QPushButton::clicked, this, &QosView::onAddRule);
        row->addWidget(addBtn_, 0);
        col->addLayout(row);

        onPresetChanged(0);
        return card;
    }

    QWidget* QosView::buildRuleList()
    {
        auto* card = new QFrame(this);
        card->setObjectName(QStringLiteral("sectionCard"));
        card->setAttribute(Qt::WA_StyledBackground, true);
        auto* col = new QVBoxLayout(card);
        col->setContentsMargins(tokens::CardInsetH, tokens::CardInsetV,
                                tokens::CardInsetH, tokens::CardInsetV);
        col->setSpacing(tokens::TightGap);

        auto* heading = new QLabel(tr("Your rules"), card);
        heading->setProperty("uiRole", QStringLiteral("sectionTitle"));
        col->addWidget(heading);

        emptyLabel_ = new QLabel(
            tr("No rules yet."), card);
        emptyLabel_->setObjectName(QStringLiteral("settingsRowDesc"));
        emptyLabel_->setWordWrap(true);
        col->addWidget(emptyLabel_);

        ruleListLay_ = new QVBoxLayout;
        ruleListLay_->setContentsMargins(0, 0, 0, 0);
        ruleListLay_->setSpacing(tokens::TightGap);
        col->addLayout(ruleListLay_);

        return card;
    }

    void QosView::onPresetChanged(int index)
    {
        const auto& all = q::presets();
        if (!presetDescr_ || index < 0 || index >= all.size())
            return;
        presetDescr_->setText(all.at(index).description);
    }

    void QosView::reload()
    {
        VTH_ASSERT_UI_THREAD();
        // The rules file is read on a worker: it lives in the profile folder,
        // which can be a slow or redirected drive.
        loadToken_.cancel();
        loadToken_ = vth::fetch<QVector<q::Policy>>(
            this,
            []() { return q::loadStore(); },
            [this](QVector<q::Policy> loaded) {
                policies_ = std::move(loaded);
                rebuildRules();
                refreshSummary();
            });
    }

    void QosView::refreshSummary()
    {
        if (!summaryLabel_)
            return;
        const q::Summary s = q::summarize(policies_);
        summaryLabel_->setVisible(s.total > 0);
        if (s.total == 0)
            return;
        summaryLabel_->setText(s.total == 1
            ? tr("1 rule, %1 active.").arg(s.active)
            : tr("%1 rules, %2 active.").arg(s.total).arg(s.active));
    }

    void QosView::rebuildRules()
    {
        if (!ruleListLay_)
            return;
        while (QLayoutItem* it = ruleListLay_->takeAt(0)) {
            if (QWidget* w = it->widget())
                w->deleteLater();
            delete it;
        }
        if (emptyLabel_)
            emptyLabel_->setVisible(policies_.isEmpty());

        QWidget* host = ruleListLay_->parentWidget();
        for (const q::Policy& p : policies_) {
            auto* row = new QFrame(host);
            row->setObjectName(QStringLiteral("settingsRow"));
            row->setAttribute(Qt::WA_StyledBackground, true);
            auto* lay = new QHBoxLayout(row);
            lay->setContentsMargins(tokens::RowInsetH, tokens::RowInsetV,
                                    tokens::RowInsetH, tokens::RowInsetV);
            lay->setSpacing(tokens::CardMargin);

            auto* icon = new vth::widgets::GlyphLabel(
                isLimited(p) ? QStringLiteral("minus") : QStringLiteral("rocket"),
                &pal::textSecondary, tokens::GlyphLg, row);
            lay->addWidget(icon, 0, Qt::AlignVCenter);

            auto* textCol = new QVBoxLayout;
            textCol->setContentsMargins(0, 0, 0, 0);
            textCol->setSpacing(tokens::Space2);
            auto* title = new QLabel(
                p.application.isEmpty() ? p.name : tr("%1 - %2").arg(p.name, p.application), row);
            title->setObjectName(QStringLiteral("settingsRowTitle"));
            auto* desc = new QLabel(ruleSummary(p), row);
            desc->setObjectName(QStringLiteral("settingsRowDesc"));
            textCol->addWidget(title);
            textCol->addWidget(desc);
            lay->addLayout(textCol, 1);

            auto* toggle = new vth::widgets::SwitchButton(row);
            toggle->setChecked(p.enabled);
            toggle->setToolTip(tr("Apply this rule to Windows"));
            // Named per rule, because a column of identical switches says nothing to a
            // screen reader.
            toggle->setAccessibleName(
                p.application.isEmpty() ? p.name : tr("%1 - %2").arg(p.name, p.application));
            const QString id = p.id;
            connect(toggle, &QAbstractButton::toggled, this,
                    [this, id](bool on) { setRuleEnabled(id, on); });
            lay->addWidget(toggle, 0, Qt::AlignVCenter);

            auto* del = new QToolButton(row);
            del->setProperty("uiRole", QStringLiteral("iconButton"));
            del->setCursor(Qt::PointingHandCursor);
            del->setAccessibleName(tr("Delete rule"));
            del->setToolTip(tr("Delete rule"));
            vth::icons::applyIconButton(del, QStringLiteral("trash-2"), &pal::textSecondary, 16);
            connect(del, &QToolButton::clicked, this, [this, id]() { deleteRule(id); });
            lay->addWidget(del, 0, Qt::AlignVCenter);

            ruleListLay_->addWidget(row);
        }
    }

    void QosView::onAddRule()
    {
        VTH_ASSERT_UI_THREAD();
        const auto& all = q::presets();
        const int idx = presetSel_ ? presetSel_->currentIndex() : -1;
        if (idx < 0 || idx >= all.size())
            return;
        const q::Preset& preset = all.at(idx);

        q::Policy p;
        p.name        = preset.name;
        p.application = appEdit_ ? appEdit_->text().trimmed() : QString();
        p.direction   = preset.direction;
        p.protocol    = preset.protocol;
        p.dscp        = preset.dscp;
        p.enabled     = true;

        const int limitIdx = limitSel_ ? limitSel_->currentIndex() : 0;
        if (limitIdx > 0 && limitIdx < static_cast<int>(std::size(kLimits)))
            p.throttleRate = QString::fromLatin1(kLimits[limitIdx].rate);

        // Composed before the duplicate check, because the system stores this name.
        if (!p.application.isEmpty())
            p.name = tr("%1 (%2)").arg(preset.name, p.application);

        // The Group-Policy rule is keyed by name, so two rules sharing one would
        // overwrite each other on the system while looking distinct here.
        for (const q::Policy& existing : policies_) {
            if (existing.name.compare(p.name, Qt::CaseInsensitive) == 0
                && existing.application.compare(p.application, Qt::CaseInsensitive) == 0) {
                vth::dialogs::warn(this, tr("Rule not added"),
                    tr("There is already a \"%1\" rule for that application.").arg(p.name));
                return;
            }
        }

        const q::Policy saved = q::createPolicy(p);
        if (saved.id.isEmpty()) {
            vth::dialogs::warn(this, tr("Rule not added"),
                               tr("Vethida could not save that rule."));
            return;
        }
        if (appEdit_)
            appEdit_->clear();
        reload();
        pushToSystem(saved, true, addBtn_);
    }

    void QosView::setRuleEnabled(const QString& id, bool on)
    {
        VTH_ASSERT_UI_THREAD();
        const auto it = std::find_if(policies_.cbegin(), policies_.cend(),
                                     [&id](const q::Policy& p) { return p.id == id; });
        if (it == policies_.cend())
            return;
        const q::Policy policy = *it;

        if (!q::togglePolicy(id)) {
            vth::dialogs::warn(this, tr("Rule not changed"),
                               tr("Vethida could not change that rule."));
            reload();
            return;
        }
        reload();
        // Enabling installs the Windows rule. Disabling removes it.
        pushToSystem(policy, on, nullptr);
    }

    void QosView::deleteRule(const QString& id)
    {
        VTH_ASSERT_UI_THREAD();
        const auto it = std::find_if(policies_.cbegin(), policies_.cend(),
                                     [&id](const q::Policy& p) { return p.id == id; });
        if (it == policies_.cend())
            return;
        const q::Policy policy = *it;

        if (!vth::prefs::askLess()
            && !vth::dialogs::confirm(this, tr("Delete rule"),
                tr("\"%1\" will be removed from Windows and from Vethida.").arg(policy.name),
                tr("Delete")))
            return;

        // Off the system first: dropping the local record first would leave an
        // orphaned Group-Policy rule with nothing left pointing at it.
        pushToSystem(policy, false, nullptr);
        if (!q::deletePolicy(id))
            vth::dialogs::warn(this, tr("Rule not deleted"),
                               tr("Vethida could not delete that rule."));
        reload();
    }

    void QosView::pushToSystem(const q::Policy& policy, bool install, QWidget* busy)
    {
        struct Outcome { q::Result result; QString error; };

        if (busy)
            busy->setEnabled(false);
        systemToken_.cancel();
        systemToken_ = vth::fetch<Outcome>(
            this,
            [policy, install]() {
                Outcome o;
                o.result = install ? q::applyToSystem(policy, &o.error)
                                   : q::removeFromSystem(policy.name, &o.error);
                return o;
            },
            [this, busy, install](Outcome o) {
                VTH_ASSERT_UI_THREAD();
                if (busy)
                    busy->setEnabled(true);
                // Removing something Windows does not have is the expected result
                // of disabling a rule that was never installed, not a failure.
                if (o.result == q::Result::Ok
                    || (!install && o.result == q::Result::NotFound))
                    return;
                if (o.result == q::Result::AccessDenied) {
                    vth::dialogs::warn(this, tr("Rule not applied to Windows"),
                        vth::win::isElevated()
                            ? tr("Windows refused the rule. A policy on this PC may manage "
                                 "network priority rules. The rule is saved in Vethida.")
                            : tr("Changing network priority rules needs administrator rights. "
                                 "The rule is saved in Vethida; open Vethida again and allow "
                                 "it when Windows asks to apply it."));
                    return;
                }
                vth::dialogs::warn(this, tr("Rule not applied to Windows"),
                    o.error.isEmpty() ? q::resultLabel(o.result) : o.error);
            });
    }
}

namespace vth::ui
{
namespace
{
        QString presetIconName(const QString& iconKey)
        {
            static const QHash<QString, QString> kMap = {
                { QStringLiteral("gamepad"),           QStringLiteral("gamepad-2") },
                { QStringLiteral("phone"),             QStringLiteral("phone") },
                { QStringLiteral("projection-screen"), QStringLiteral("video") },
                { QStringLiteral("briefcase"),         QStringLiteral("briefcase") },
                { QStringLiteral("minus"),             QStringLiteral("minus") },
            };
            const auto it = kMap.find(iconKey);
            return it != kMap.end() ? it.value() : QStringLiteral("sliders-horizontal");
        }
}
}

