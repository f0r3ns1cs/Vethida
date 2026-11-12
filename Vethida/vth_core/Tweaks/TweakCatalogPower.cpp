#include "TweakEngine.h"

// Power. PowerThrottlingOff and USB selective suspend already ship in the starter
// set. Granular SUB_PROCESSOR powercfg overrides (core parking / idle-disable /
// throttle-min / perf time-check) are deferred: their revert can't reliably
// restore the user's exact prior per-plan defaults via static args, and the
// High-Performance plan switch already removes down-clocking reversibly.

namespace vth::tweaks::detail {

    namespace {
        const QString kPower = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power");
        const QString kGfxPower = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Power");

        CommandOp powercfg(const QStringList& applyArgs, const QStringList& revertArgs) {
            CommandOp op;
            op.program = QStringLiteral("powercfg");
            op.applyArgs = applyArgs;
            op.revertArgs = revertArgs;
            return op;
        }
    }

    void appendPower(QVector<TweakSpec>& c) {
        {
            TweakSpec t;
            t.id = QStringLiteral("power.high-performance-plan");
            t.category = QStringLiteral("general");
            t.subCategory = QStringLiteral("Power plan");
            t.name = QStringLiteral("High performance power plan");
            t.description = QStringLiteral(
                "Switches the active power plan to High Performance, which keeps the CPU "
                "from down-clocking and disables core parking.");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.commandOps = {
                powercfg({ QStringLiteral("/setactive"), QStringLiteral("8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c") },
                         { QStringLiteral("/setactive"), QStringLiteral("381b4222-f694-41f0-9685-ff5bb260df2e") }),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("power.hibernate-off");
            t.category = QStringLiteral("general");
            t.subCategory = QStringLiteral("Hibernation");
            t.name = QStringLiteral("Disable hibernation");
            t.description = QStringLiteral(
                "Turns off hibernation, deletes hiberfil.sys and disables Fast Startup.");
            t.iconName = QStringLiteral("power");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.warnings = { QStringLiteral("Disables Fast Startup; first boot after this is slightly slower.") };
            t.commandOps = {
                powercfg({ QStringLiteral("/h"), QStringLiteral("off") },
                         { QStringLiteral("/h"), QStringLiteral("on") }),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("power.connected-standby-off");
            t.category = QStringLiteral("power");
            t.subCategory = QStringLiteral("Modern Standby");
            t.name = QStringLiteral("Disable connected standby");
            t.description = QStringLiteral("Disables Modern Standby (S0 low-power idle) on laptops that support it by writing PlatformAoAcOverride=0 (and the legacy CsEnabled=0, which newer builds ignore). This can reduce sleep battery drain, but only helps if the firmware also exposes classic S3 sleep; on desktops it does nothing.");
            t.iconName = QStringLiteral("power");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "laptops: removes Modern Standby; some AOAC platforms misbehave without it. "
                "Desktops are unaffected.");
            t.registryOps = {
                dwordOp(kPower, QStringLiteral("CsEnabled"), 0, std::nullopt),
                dwordOp(kPower, QStringLiteral("PlatformAoAcOverride"), 0, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("power.user-presence-qos-off");
            t.category = QStringLiteral("general");
            t.subCategory = QStringLiteral("Quality of Service");
            t.name = QStringLiteral("Disable user-inactivity throttling");
            t.description = QStringLiteral("Sets the Microsoft-documented DisableUserPresenceQos value so Windows stops demoting a foreground app's quality-of-service after a stretch of no keyboard/mouse input. Note this inactivity demotion only happens on battery, so it mainly matters for laptops running unattended workloads or benchmarks; on AC power it changes nothing.");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerThrottling"),
                        QStringLiteral("DisableUserPresenceQos"), 1, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("power.energy-telemetry-off");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Telemetry");
            t.name = QStringLiteral("Disable Energy/Sleep telemetry");
            t.description = QStringLiteral(
                "Turns off energy estimation, Sleep Study and sleep-reliability diagnostics.");
            t.iconName = QStringLiteral("eye-off");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.registryOps = {
                dwordOp(kPower, QStringLiteral("EnergyEstimationEnabled"), 0, std::nullopt),
                dwordOp(kPower, QStringLiteral("SleepStudyDisabled"), 1, std::nullopt),
                dwordOp(kPower, QStringLiteral("SleepReliabilityDetailedDiagnostics"), 0, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("power.config-mgr-self-healing-off");
            t.category = QStringLiteral("power");
            t.name = QStringLiteral("Disable config manager self-healing");
            t.description = QStringLiteral("Disables Configuration Manager's background self-healing.");
            t.iconName = QStringLiteral("settings");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral("placebo: no measured benefit on a non-managed PC.");
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Configuration Manager"),
                        QStringLiteral("EnableSelfHealing"), 0, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("power.gfx-latency-tolerance");
            t.category = QStringLiteral("power");
            t.name = QStringLiteral("Minimum graphics latency tolerance");
            t.description = QStringLiteral(
                "Sets every GraphicsDrivers power latency-tolerance/exit-latency value to "
                "its minimum, forcing the GPU to stay in its highest-performance state.");
            t.iconName = QStringLiteral("monitor");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "controversial: minimum tolerance can cause screen flicker on some GPUs; "
                "benefit is unmeasured. Remove and reboot if you see flickering.");
            QVector<RegistryOp> ops;
            for (const QString& name : {
                QStringLiteral("DefaultD3TransitionLatencyActivelyUsed"),
                QStringLiteral("DefaultD3TransitionLatencyIdleLongTime"),
                QStringLiteral("DefaultD3TransitionLatencyIdleMonitorOff"),
                QStringLiteral("DefaultD3TransitionLatencyIdleNoContext"),
                QStringLiteral("DefaultD3TransitionLatencyIdleShortTime"),
                QStringLiteral("DefaultD3TransitionLatencyIdleVeryLongTime"),
                QStringLiteral("DefaultLatencyToleranceIdle0"),
                QStringLiteral("DefaultLatencyToleranceIdle0MonitorOff"),
                QStringLiteral("DefaultLatencyToleranceIdle1"),
                QStringLiteral("DefaultLatencyToleranceIdle1MonitorOff"),
                QStringLiteral("DefaultLatencyToleranceMemory"),
                QStringLiteral("DefaultLatencyToleranceNoContext"),
                QStringLiteral("DefaultLatencyToleranceNoContextMonitorOff"),
                QStringLiteral("DefaultLatencyToleranceOther"),
                QStringLiteral("DefaultLatencyToleranceTimerPeriod"),
                QStringLiteral("DefaultMemoryRefreshLatencyToleranceActivelyUsed"),
                QStringLiteral("DefaultMemoryRefreshLatencyToleranceMonitorOff"),
                QStringLiteral("DefaultMemoryRefreshLatencyToleranceNoContext"),
                QStringLiteral("ExitLatency"),
                QStringLiteral("ExitLatencyCheckEnabled"),
                QStringLiteral("Latency"),
                QStringLiteral("LatencyToleranceDefault"),
                QStringLiteral("LatencyToleranceFSVP"),
                QStringLiteral("LatencyToleranceIdle0"),
                QStringLiteral("LatencyToleranceIdle0MonitorOff"),
                QStringLiteral("LatencyToleranceIdle1"),
                QStringLiteral("LatencyToleranceIdle1MonitorOff"),
                QStringLiteral("LatencyToleranceMemory"),
                QStringLiteral("LatencyToleranceNoContext"),
                QStringLiteral("LatencyToleranceNoContextMonitorOff"),
                QStringLiteral("LatencyToleranceOther"),
                QStringLiteral("LatencyToleranceTimerPeriod"),
                QStringLiteral("MonitorLatencyTolerance"),
                QStringLiteral("MonitorRefreshLatencyTolerance"),
                QStringLiteral("TransitionLatency") })
                ops.push_back(dwordOp(kGfxPower, name, 1, std::nullopt));
            t.registryOps = ops;
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("power.dpc-isr-latency-tolerance");
            t.category = QStringLiteral("power");
            t.name = QStringLiteral("DPC/ISR latency tolerance overrides");
            t.description = QStringLiteral(
                "Forces the platform performance latency-tolerance override and graphics "
                "latency-bucket/Miracast tracking values to their minimum.");
            t.iconName = QStringLiteral("gauge");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "undocumented tuning values with no published measurement; overlaps with the "
                "existing graphics latency-tolerance tweak.");
            t.registryOps = {
                dwordOp(kPower, QStringLiteral("LatencyTolerancePerfOverride"), 1, std::nullopt),
                dwordOp(kPower, QStringLiteral("LatencyToleranceScreenOffIR"), 1, std::nullopt),
                dwordOp(kPower, QStringLiteral("RtlCapabilityCheckLatency"), 1, std::nullopt),
                dwordOp(kGfxPower, QStringLiteral("MaxIAverageGraphicsLatencyInOneBucket"), 1, std::nullopt),
                dwordOp(kGfxPower, QStringLiteral("MiracastPerfTrackGraphicsLatency"), 1, std::nullopt),
            };
            c.push_back(t);
        }
    }

} // namespace vth::tweaks::detail

