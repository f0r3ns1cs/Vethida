#include "pch.h"
#include "PowerPlansView.h"

#include "../Pages/SystemTools.h"
#include "../Theme/DesignTokens.h"

#include "../../vth_core/Models/ProductTier.h"
#include "../../vth_core/Windows/Power.h"

#include <QButtonGroup>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QVBoxLayout>

namespace vth::ui
{
    namespace tokens = vth::theme::tokens;

    namespace
    {
        // What the active plan actually does, in one line.
        QString describePolicy(const vth::win::power::ProcessorPolicy& p)
        {
            if (!p.valid)
                return {};

            QStringList parts;
            parts << PowerPlansView::tr("Processor %1-%2%")
                       .arg(p.minStatePercent)
                       .arg(p.maxStatePercent);
            // A 100 percent minimum means the scheduler may never park one.
            parts << (p.minCoresPercent >= 100
                          ? PowerPlansView::tr("no core parking")
                          : PowerPlansView::tr("core parking on"));
            return parts.join(QStringLiteral(" - "));
        }
    }

    PowerPlansView::PowerPlansView(QWidget* parent) : QWidget(parent)
    {
        setObjectName(QStringLiteral("powerPlansView"));
        buildUi();
        refreshActivePlan();
    }

    PowerPlansView::~PowerPlansView()
    {
        activeToken_.cancel();
    }

    void PowerPlansView::buildUi()
    {
        auto* lay = new QVBoxLayout(this);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(tokens::CompactGap);

        auto* body = new QWidget;
        auto* bodyCol = new QVBoxLayout(body);
        bodyCol->setContentsMargins(0, 0, 0, 0);
        bodyCol->setSpacing(tokens::CardMargin);

        group_ = new QButtonGroup(this);
        group_->setExclusive(true);

        // One row per plan, rules between them. Not SettingsList: that owns a
        // QScrollArea and this pane is already inside the shell's.
        const auto& plans = vth::ui::tools::powerPlans();
        for (int i = 0; i < plans.size(); ++i) {
            const auto& plan = plans.at(i);
            if (i > 0) {
                auto* rule = new QFrame(body);
                rule->setObjectName(QStringLiteral("settingsRule"));
                rule->setAttribute(Qt::WA_StyledBackground, true);
                bodyCol->addWidget(rule);
            }

            auto* row = new QVBoxLayout;
            row->setContentsMargins(0, 0, 0, 0);
            row->setSpacing(tokens::Space2);

            auto* titleRow = new QHBoxLayout;
            titleRow->setContentsMargins(0, 0, 0, 0);
            titleRow->setSpacing(tokens::CompactGap);

            auto* choice = new QRadioButton(plan.name, body);
            choice->setCursor(Qt::PointingHandCursor);
            group_->addButton(choice, i);
            rows_.insert(i, choice);
            titleRow->addWidget(choice, 0, Qt::AlignVCenter);

#ifdef VETHIDA_LICENSING
            // The tier is stated on the row rather than enforced by disabling
            // the control; the click on a locked row reaches the upgrade prompt.
            auto* pill = new QLabel(vth::models::tierLabel(plan.tier), body);
            pill->setObjectName(QStringLiteral("tweakCardTierPill"));
            pill->setAttribute(Qt::WA_StyledBackground, true);
            titleRow->addWidget(pill, 0, Qt::AlignVCenter);
#endif
            titleRow->addStretch(1);
            row->addLayout(titleRow);

            auto* detail = new QLabel(plan.detail, body);
            detail->setObjectName(QStringLiteral("settingsRowDesc"));
            detail->setWordWrap(true);
            row->addWidget(detail);

            bodyCol->addLayout(row);
        }

        // Windows' Balanced plan, named by powercfg (it's localized)
        balancedHost_ = new QWidget(body);
        auto* balCol = new QVBoxLayout(balancedHost_);
        balCol->setContentsMargins(0, 0, 0, 0);
        balCol->setSpacing(tokens::CardMargin);

        auto* balRule = new QFrame(balancedHost_);
        balRule->setObjectName(QStringLiteral("settingsRule"));
        balRule->setAttribute(Qt::WA_StyledBackground, true);
        balCol->addWidget(balRule);

        auto* balRow = new QVBoxLayout;
        balRow->setContentsMargins(0, 0, 0, 0);
        balRow->setSpacing(tokens::Space2);
        balancedRow_ = new QRadioButton(balancedHost_);
        balancedRow_->setCursor(Qt::PointingHandCursor);
        group_->addButton(balancedRow_, plans.size());
        balRow->addWidget(balancedRow_, 0, Qt::AlignVCenter);

        balancedDetail_ = new QLabel(
            tr("What Windows uses out of the box."), balancedHost_);
        balancedDetail_->setObjectName(QStringLiteral("settingsRowDesc"));
        balancedDetail_->setWordWrap(true);
        balRow->addWidget(balancedDetail_);
        balCol->addLayout(balRow);

        balancedHost_->setVisible(false);
        bodyCol->addWidget(balancedHost_);

        // idClicked, because a selection settled by refreshActivePlan() must not import.
        connect(group_, &QButtonGroup::idClicked, this, [this](int id) {
            const auto& all = vth::ui::tools::powerPlans();

            // The Balanced row. No tier gate and no import - it makes an
            // existing scheme active, which is the undo direction.
            if (id == all.size()) {
                if (balancedGuid_.isEmpty())
                    return;
                setRowsEnabled(false);
                status_->setText(tr("Applying %1").arg(balancedRow_->text()));
                status_->setVisible(true);
                vth::ui::tools::setActivePowerScheme(
                    balancedGuid_, balancedRow_->text(),
                    [this](bool) { refreshActivePlan(); });
                return;
            }

            if (id < 0 || id >= all.size())
                return;

            setRowsEnabled(false);
            status_->setText(tr("Applying %1").arg(all.at(id).name));
            status_->setVisible(true);

            // Settled by re-reading the machine, so a locked plan or a refused import
            // can't leave a row checked that powercfg never made active.
            vth::ui::tools::applyPowerPlan(this, all.at(id),
                                           [this](bool) { refreshActivePlan(); });
        });

        status_ = new QLabel(body);
        status_->setProperty("uiRole", QStringLiteral("metaText"));
        status_->setWordWrap(true);
        bodyCol->addWidget(status_);

        auto* card = new QFrame(this);
        card->setProperty("uiRole", QStringLiteral("card"));
        card->setAttribute(Qt::WA_StyledBackground, true);
        auto* cardCol = new QVBoxLayout(card);
        cardCol->setContentsMargins(tokens::CardInsetH, tokens::CardInsetV,
                                    tokens::CardInsetH, tokens::CardInsetV);
        cardCol->setSpacing(0);
        body->setParent(card);
        cardCol->addWidget(body);

        lay->addWidget(card);
        lay->addStretch(1);
    }

    void PowerPlansView::setRowsEnabled(bool enabled)
    {
        for (auto* row : std::as_const(rows_))
            row->setEnabled(enabled);
        // Returning to Windows' own plan needs the same rights as importing
        // one, so it follows the same gate.
        if (balancedRow_)
            balancedRow_->setEnabled(enabled);
    }

    void PowerPlansView::refreshActivePlan()
    {
        activeToken_.cancel();
        setRowsEnabled(false);
        status_->setText(tr("Reading the active plan"));
        status_->setVisible(true);

        struct Snapshot {
            vth::ui::tools::ActivePowerPlan active;
            vth::ui::tools::ActivePowerPlan balanced;
            vth::win::power::ProcessorPolicy policy;
        };
        activeToken_ = vth::fetch<Snapshot>(
            this,
            []() {
                // The policy read needs the active scheme's guid.
                Snapshot s{ vth::ui::tools::readActivePowerPlan(),
                            vth::ui::tools::readWindowsBalancedPlan(), {} };
                if (s.active.ok)
                    s.policy = vth::win::power::readProcessorPolicy(s.active.guid);
                return s;
            },
            [this](Snapshot snap) {
                const vth::ui::tools::ActivePowerPlan active = snap.active;

                // The row can only exist once powercfg has named the scheme.
                balancedGuid_ = snap.balanced.ok ? snap.balanced.guid : QString();
                if (snap.balanced.ok) {
                    balancedRow_->setText(snap.balanced.name);
                    balancedHost_->setVisible(true);
                } else {
                    balancedHost_->setVisible(false);
                }

                if (!active.ok) {
                    for (auto* row : std::as_const(rows_)) {
                        const QSignalBlocker block(row);
                        row->setAutoExclusive(false);
                        row->setChecked(false);
                        row->setAutoExclusive(true);
                    }
                    if (balancedRow_) {
                        const QSignalBlocker block(balancedRow_);
                        balancedRow_->setAutoExclusive(false);
                        balancedRow_->setChecked(false);
                        balancedRow_->setAutoExclusive(true);
                    }
                    status_->setText(
                        tr("Vethida could not read the active power plan. Open "
                           "Power Options in Windows to see which one is in "
                           "use."));
                    status_->setVisible(true);
                    setRowsEnabled(true);
                    return;
                }

                // Matched on the name inside the .pow, so a plan imported by an
                // earlier install still lights its own row.
                const auto& all = vth::ui::tools::powerPlans();
                int match = -1;
                for (int i = 0; i < all.size(); ++i)
                    if (all.at(i).schemeGuid.compare(active.guid, Qt::CaseInsensitive) == 0
                        || all.at(i).schemeName.compare(active.name, Qt::CaseInsensitive) == 0)
                        match = i;

                for (auto it = rows_.cbegin(); it != rows_.cend(); ++it) {
                    const QSignalBlocker block(it.value());
                    it.value()->setAutoExclusive(false);
                    it.value()->setChecked(it.key() == match);
                    it.value()->setAutoExclusive(true);
                }

                // Balanced is matched on its GUID, which is the same on every
                // install, rather than on the localized name.
                const bool onBalanced = match < 0 && !balancedGuid_.isEmpty()
                    && active.guid.compare(balancedGuid_, Qt::CaseInsensitive) == 0;
                if (balancedRow_) {
                    const QSignalBlocker block(balancedRow_);
                    balancedRow_->setAutoExclusive(false);
                    balancedRow_->setChecked(onBalanced);
                    balancedRow_->setAutoExclusive(true);
                }

                // The active plan is named only when no row here is checked.
                const QString policy = describePolicy(snap.policy);
                if (match >= 0 || onBalanced) {
                    status_->setText(policy);
                } else if (policy.isEmpty()) {
                    status_->setText(
                        tr("None of these is active. Windows is using %1.")
                            .arg(active.name));
                } else {
                    status_->setText(
                        tr("None of these is active. Windows is using %1. %2.")
                            .arg(active.name, policy));
                }
                status_->setVisible(!status_->text().isEmpty());
                setRowsEnabled(true);
            });
    }
}
