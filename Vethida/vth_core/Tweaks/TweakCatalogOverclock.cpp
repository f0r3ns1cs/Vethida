#include "TweakEngine.h"

// Overclocking itself happens in the BIOS. The OS side is stopping Windows power
// management from clawing an overclock back: pin the processor performance state
// to 100% and, optionally, keep the CPU out of deep idle. Both are powercfg
// command tweaks on the active plan, on the bios tab.

namespace vth::tweaks::detail {

    namespace {
        const QString kSubProcessor   = QStringLiteral("54533251-82be-4824-96c1-47b60b740d00");
        const QString kProcThrottleMin = QStringLiteral("893dee8e-2bef-41e0-89c6-b55d0929964c");
        const QString kProcIdleDisable = QStringLiteral("5d76a2ca-e8c0-402f-a133-2158492d58ad");

        CommandOp powercfg(const QStringList& applyArgs, const QStringList& revertArgs) {
            CommandOp op;
            op.program = QStringLiteral("powercfg");
            op.applyArgs = applyArgs;
            op.revertArgs = revertArgs;
            return op;
        }

        CommandOp setProcessorValue(const QString& settingGuid, const QString& applied,
                                    const QString& reverted) {
            return powercfg(
                { QStringLiteral("/setacvalueindex"), QStringLiteral("scheme_current"),
                kSubProcessor, settingGuid, applied },
                { QStringLiteral("/setacvalueindex"), QStringLiteral("scheme_current"),
                kSubProcessor, settingGuid, reverted });
        }

        CommandOp activateScheme() {
            return powercfg({ QStringLiteral("/setactive"), QStringLiteral("scheme_current") },
                            { QStringLiteral("/setactive"), QStringLiteral("scheme_current") });
        }
    }

    void appendOverclock(QVector<TweakSpec>& c) {
        {
            TweakSpec t;
            t.id = QStringLiteral("oc.min-processor-state-100");
            t.category = QStringLiteral("power");
            t.subCategory = QStringLiteral("Processor state");
            t.declaredTier = vth::models::ProductTier::Ultimate;
            t.name = QStringLiteral("Lock minimum processor state to 100%");
            t.description = QStringLiteral(
                "Pins the minimum processor performance state to 100% on the active power "
                "plan, stopping Windows from down-clocking an overclocked CPU at low load.");
            t.iconName = QStringLiteral("cpu");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "complements a BIOS overclock; pointless on a stock auto-clocking chip and on "
                "laptops it kills idle power saving. Revert restores 5% (the typical balanced "
                "default), not a captured prior value.");
            t.commandOps = {
                setProcessorValue(kProcThrottleMin, QStringLiteral("100"), QStringLiteral("5")),
                activateScheme(),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("oc.disable-cpu-cstates");
            t.category = QStringLiteral("power");
            t.subCategory = QStringLiteral("C-States");
            t.declaredTier = vth::models::ProductTier::Ultimate;
            t.name = QStringLiteral("Disable CPU idle states (C-states)");
            t.description = QStringLiteral(
                "Holds the CPU out of deep idle on the active power plan, the software mirror of "
                "the BIOS C-state disable, for lower wake latency under an overclock.");
            t.iconName = QStringLiteral("cpu");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "raises idle power and temps noticeably; only worth it alongside an overclock or "
                "for hard real-time latency. Laptops will run hot and drain fast.");
            t.commandOps = {
                setProcessorValue(kProcIdleDisable, QStringLiteral("1"), QStringLiteral("0")),
                activateScheme(),
            };
            c.push_back(t);
        }
    }
}
