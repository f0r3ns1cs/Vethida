#include "TweakEngine.h"

namespace vth::tweaks::detail {

void appendDefsPowerPort(QVector<TweakSpec>& c) {
    {
        TweakSpec t;
        t.id = QStringLiteral("power.driver_power_saving.disable");
        t.category = QStringLiteral("power");
        t.subCategory = QStringLiteral("Telemetry");
        t.name = QStringLiteral("Disable Battery/Energy estimation background processing");
        t.description = QStringLiteral("Disables the background battery-discharge estimator and energy-estimation processing that Windows runs continuously to power Task Manager's per-app power usage and battery time-remaining predictions.");
        t.iconName = QStringLiteral("battery-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Task Manager per-app power usage and battery time-remaining estimates may become less accurate or stop updating"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("EnergyEstimationDisabled"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("UserBatteryDischargeEstimator"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("power.ui.hide_hard_disk_turnoff_setting");
        t.category = QStringLiteral("power");
        t.subCategory = QStringLiteral("Power plan");
        t.name = QStringLiteral("Hide 'turn off hard disk after' from power options");
        t.description = QStringLiteral("Marks the 'Turn off hard disk after' advanced power setting as hidden (Attributes=1) so it no longer appears in the Power Options control panel. Purely cosmetic - the underlying idle-disk-sleep timer and its current value are unaffected.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("'Turn off hard disk after' no longer appears under Power Options > Advanced settings"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\0012ee47-9041-4b5d-9b77-535fba8b1442\\6738e2c4-e8a5-4a42-b16a-e040e769756e"), QStringLiteral("Attributes"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("power.timer_coalescing.disable_all");
        t.category = QStringLiteral("power");
        t.subCategory = QStringLiteral("Timer coalescing");
        t.name = QStringLiteral("Disable timer coalescing (all known locations)");
        t.description = QStringLiteral("Writes CoalescingTimerInterval=0 under every Session Manager and Power-related key the reference tool targets, in an attempt to disable Windows timer coalescing (the batching of periodic timer wakeups to save power) system-wide.");
        t.iconName = QStringLiteral("clock-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("CoalescingTimerInterval is not a Microsoft-documented setting under these keys; real-world effect is unverified"),
        };
        t.concern = QStringLiteral("CoalescingTimerInterval is not a published Windows setting under Session Manager/Power and community reports disagree on its effect. Maintainer note only: this does not gate recommendation, which is decided on potential harm alone.");
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Power"), QStringLiteral("CoalescingTimerInterval"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("CoalescingTimerInterval"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("CoalescingTimerInterval"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Executive"), QStringLiteral("CoalescingTimerInterval"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager"), QStringLiteral("CoalescingTimerInterval"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\ModernSleep"), QStringLiteral("CoalescingTimerInterval"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("CoalescingTimerInterval"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control"), QStringLiteral("CoalescingTimerInterval"), 0u, std::nullopt),
        };
        c.push_back(t);
    }
}

}
