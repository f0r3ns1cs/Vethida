#include "TweakEngine.h"

// Boot flags set through bcdedit, shown on the bios tab.

namespace vth::tweaks::detail {

    namespace {
        CommandOp bcdSet(const QString& name, const QString& value) {
            CommandOp op;
            op.program = QStringLiteral("bcdedit");
            op.applyArgs  = { QStringLiteral("/set"), name, value };
            op.revertArgs = { QStringLiteral("/deletevalue"), name };
            return op;
        }
    }

    void appendBoot(QVector<TweakSpec>& c) {
        Q_UNUSED(c);

        {
            TweakSpec t;
            t.id = QStringLiteral("bcd.disable-hpet");
            t.category = QStringLiteral("advanced");
            t.declaredTier = vth::models::ProductTier::Ultimate;
            t.name = QStringLiteral("Disable HPET (platform clock)");
            t.description = QStringLiteral(
                "Stops Windows using the High Precision Event Timer as its clock source, "
                "falling back to the lower-latency TSC.");
            t.iconName = QStringLiteral("clock");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = { QStringLiteral("Can cause Discord audio crackle, USB disconnects or clock drift - revert if you see those.") };
            t.concern = QStringLiteral(
                "AMD Ryzen: ~2-3% latency improvement. Modern Intel: marginal (already uses "
                "TSC). Pair with dynamic-tick disable or neither.");
            t.commandOps = { bcdSet(QStringLiteral("useplatformclock"), QStringLiteral("no")) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("bcd.disable-dynamic-tick");
            t.category = QStringLiteral("advanced");
            t.declaredTier = vth::models::ProductTier::Ultimate;
            t.name = QStringLiteral("Disable dynamic tick");
            t.description = QStringLiteral(
                "Disables the tickless-kernel optimization so timer interrupts fire at a "
                "constant rate, for more consistent timing.");
            t.iconName = QStringLiteral("clock");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            // "idle power" in this warning is what raisesThermalOrPowerLoad() reads to
            // make the tweak Ultimate.
            t.warnings = { QStringLiteral("Raises idle power draw, which costs battery life on a laptop.") };
            t.concern = QStringLiteral("companion to HPET disable; apply both or neither.");
            t.commandOps = { bcdSet(QStringLiteral("disabledynamictick"), QStringLiteral("yes")) };
            c.push_back(t);
        }
    }

}
