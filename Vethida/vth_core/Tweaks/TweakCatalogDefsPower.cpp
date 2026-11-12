#include "TweakEngine.h"

namespace vth::tweaks::detail {

void appendDefsPower(QVector<TweakSpec>& c) {
    {
        TweakSpec t;
        t.id = QStringLiteral("pwr.throttle.disable");
        t.category = QStringLiteral("power");
        t.subCategory = QStringLiteral("Power throttling");
        t.name = QStringLiteral("Disable all power throttling");
        t.description = QStringLiteral("Disables both power throttling and energy estimation at the system level.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Increased power consumption on battery-powered devices"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("PowerThrottlingOff"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("EnergyEstimationEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerThrottling"), QStringLiteral("PowerThrottlingOff"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("pwr.hibernate.energy_estimation");
        t.category = QStringLiteral("power");
        t.subCategory = QStringLiteral("Telemetry");
        t.name = QStringLiteral("Disable energy estimation engine");
        t.description = QStringLiteral("Disables the Energy Estimation Engine service that profiles application power usage.");
        t.iconName = QStringLiteral("battery");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Per-app power usage column in Task Manager shows no data on laptops"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Eee"), QStringLiteral("Start"), 4u, 2u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("power.disable-pci-aspm");
        t.category = QStringLiteral("power");
        t.subCategory = QStringLiteral("PCI Express");
        t.name = QStringLiteral("Disable PCIe active state power management");
        t.description = QStringLiteral("Tells PCIe links to stay at full power instead of negotiating L0s/L1 idle states. Removes a class of stutter caused by lazy NVMe / GPU wake-up.");
        t.iconName = QStringLiteral("topology-star");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\501a4d13-42af-4429-9fd1-a8218c268e20\\ee12f906-d277-404b-b6da-e5fa1a576df5"), QStringLiteral("ValueMax"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("pwr.hibernate.sleep_study");
        t.category = QStringLiteral("power");
        t.subCategory = QStringLiteral("Telemetry");
        t.name = QStringLiteral("Disable Sleep Study");
        t.description = QStringLiteral("Disables the SleepStudy service that logs detailed sleep/wake diagnostic data.");
        t.iconName = QStringLiteral("moon");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("powercfg /sleepstudy and /batteryreport lose detailed modern-standby data"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\SleepStudy"), QStringLiteral("Start"), 4u, 2u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("pwr.telemetry.storage_sense");
        t.category = QStringLiteral("storage");
        t.subCategory = QStringLiteral("Storage Sense");
        t.name = QStringLiteral("Disable Storage Sense");
        t.description = QStringLiteral("Disables Storage Sense, which automatically deletes temporary files and empties the Recycle Bin.");
        t.iconName = QStringLiteral("database");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Temp files, Recycle Bin, and old Downloads no longer cleaned automatically"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\StorageSense\\Parameters\\StoragePolicy"), QStringLiteral("01"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("pwr.telemetry.tagged_energy");
        t.category = QStringLiteral("power");
        t.subCategory = QStringLiteral("Telemetry");
        t.name = QStringLiteral("Disable tagged energy logging");
        t.description = QStringLiteral("Disables per-process tagged energy accounting used for battery usage attribution.");
        t.iconName = QStringLiteral("battery");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Per-process energy attribution unavailable in Task Manager and Settings battery usage view"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Eee\\Parameters"), QStringLiteral("TaggedEnergy"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\EnergyEstimation\\TaggedEnergy"), QStringLiteral("DisableTaggedEnergyLogging"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\EnergyEstimation\\TaggedEnergy"), QStringLiteral("TelemetryMaxApplication"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\EnergyEstimation\\TaggedEnergy"), QStringLiteral("TelemetryMaxTagPerApplication"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("pwr.gpu.low_latency");
        t.category = QStringLiteral("power");
        t.subCategory = QStringLiteral("GPU latency");
        t.name = QStringLiteral("GPU low latency power transitions");
        t.description = QStringLiteral("Reduces GPU power state transition latencies for lower frame-time spikes.");
        t.iconName = QStringLiteral("monitor");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Slightly increased GPU idle power consumption"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Power"), QStringLiteral("DefaultD3TransitionLatencyActivelyUsed"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Power"), QStringLiteral("DefaultD3TransitionLatencyIdleLongTime"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("pwr.hibernate.away_mode");
        t.category = QStringLiteral("power");
        t.subCategory = QStringLiteral("Modern standby");
        t.name = QStringLiteral("Disable Away Mode");
        t.description = QStringLiteral("Disables Away Mode (AwayModeEnabled=0) which keeps the system awake with screen off.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Away Mode unavailable system will sleep instead of staying awake with screen off"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("AwayModeEnabled"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("pwr.hibernate.latency_tolerance");
        t.category = QStringLiteral("power");
        t.subCategory = QStringLiteral("Latency");
        t.name = QStringLiteral("Minimize DPC/ISR latency tolerance");
        t.description = QStringLiteral("Sets exit latency, sensor watchdog, and all latency tolerance values to minimum (1) to prevent deep idle states and reduce C-state transition latency spikes.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Deep CPU and sensor idle states blocked; higher idle power and heat"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("ExitLatency"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("DisableSensorWatchdog"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("Latency"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("LatencyToleranceDefault"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("LatencyToleranceFSVP"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("LatencyTolerancePerfOverride"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("LatencyToleranceVSyncEnabled"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("LatencyToleranceScreenOffIR"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("RtlCapabilityCheckLatency"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("power.lock-cpu-min-state-100");
        t.category = QStringLiteral("power");
        t.subCategory = QStringLiteral("Processor state");
        t.name = QStringLiteral("Lock CPU minimum state to 100%");
        t.description = QStringLiteral("Pins the active power plan's CPU minimum frequency to 100%. Disables turbo-boost downscaling at the cost of higher idle wattage.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Idle power usage and CPU temperatures rise noticeably; battery life on laptops shortens"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\54533251-82be-4824-96c1-47b60b740d00\\893dee8e-2bef-41e0-89c6-b55d0929964c"), QStringLiteral("ValueMin"), 100u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("pwr.plan.advanced_options");
        t.category = QStringLiteral("power");
        t.subCategory = QStringLiteral("Power plan");
        t.name = QStringLiteral("Unhide advanced power options");
        t.description = QStringLiteral("Sets Attributes=0 on hidden power settings to expose all advanced processor, USB, display, PCI Express, and disk power options in the Power Options control panel.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Hidden advanced power options exposed in Control Panel; values themselves unchanged"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\54533251-82be-4824-96c1-47b60b740d00\\be337238-0d82-4146-a960-4f3749d470c7"), QStringLiteral("Attributes"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\54533251-82be-4824-96c1-47b60b740d00\\45bcc044-d885-43e2-8605-ee0ec6e96b59"), QStringLiteral("Attributes"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\54533251-82be-4824-96c1-47b60b740d00\\893dee8e-2bef-41e0-89c6-b55d0929964d"), QStringLiteral("Attributes"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\54533251-82be-4824-96c1-47b60b740d00\\bc5038f7-23e0-4960-96da-33abaf5935ed"), QStringLiteral("Attributes"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\54533251-82be-4824-96c1-47b60b740d00\\3b04d4fd-1cc7-4f23-ab1c-d1337819c4bb"), QStringLiteral("Attributes"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\54533251-82be-4824-96c1-47b60b740d00\\ea062031-0e34-4ff1-9b6d-eb1059334028"), QStringLiteral("Attributes"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\54533251-82be-4824-96c1-47b60b740d00\\75b0ae3f-bce0-45a7-8c89-c9611c25e100"), QStringLiteral("Attributes"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\54533251-82be-4824-96c1-47b60b740d00\\4d2b0152-7d5c-498b-88e2-34345392a2c5"), QStringLiteral("Attributes"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\0012ee47-9041-4b5d-9b77-535fba8b1442\\0b2d69d7-a2a1-449c-9680-f91c70521c60"), QStringLiteral("Attributes"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\238C9FA8-0AAD-41ED-83F4-97BE242C8F20\\7bc4a2f9-d8fc-4469-b07b-33eb785aaca0"), QStringLiteral("Attributes"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\238C9FA8-0AAD-41ED-83F4-97BE242C8F20\\abfc2519-3608-4c2a-94ea-171b0ed546ab"), QStringLiteral("Attributes"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\238C9FA8-0AAD-41ED-83F4-97BE242C8F20\\d4c1d4c8-d5cc-43d3-b83e-fc51215cb04d"), QStringLiteral("Attributes"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("pwr.sleep.enforce_disconnected");
        t.category = QStringLiteral("power");
        t.subCategory = QStringLiteral("Modern standby");
        t.name = QStringLiteral("Enforce disconnected standby");
        t.description = QStringLiteral("Forces disconnected standby mode instead of connected standby, preventing network activity during sleep.");
        t.iconName = QStringLiteral("wifi");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Wake-on-LAN and push notifications during sleep disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("EnforceDisconnectedStandby"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("pwr.cpu.turbo_boost_control");
        t.category = QStringLiteral("power");
        t.subCategory = QStringLiteral("Turbo boost");
        t.name = QStringLiteral("CPU turbo boost optimization");
        t.description = QStringLiteral("Sets CPU boost mode to Aggressive and boost policy to maximum for fastest frequency ramp-up.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("CPU will jump to maximum boost clocks more aggressively, drawing more power and producing more heat"),
            QStringLiteral("Laptops will see noticeably shorter battery life and more fan noise"),
            QStringLiteral("Thin-and-light systems may throttle sooner if cooling cannot keep up with the more aggressive boost"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\54533251-82be-4824-96c1-47b60b740d00\\be337238-0d82-4146-a960-4f3749d470c7"), QStringLiteral("ACSettingIndex"), 2u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\54533251-82be-4824-96c1-47b60b740d00\\be337238-0d82-4146-a960-4f3749d470c7"), QStringLiteral("DCSettingIndex"), 2u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\54533251-82be-4824-96c1-47b60b740d00\\45bcc044-d885-43e2-8605-ee0ec6e96b59"), QStringLiteral("ACSettingIndex"), 100u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\54533251-82be-4824-96c1-47b60b740d00\\45bcc044-d885-43e2-8605-ee0ec6e96b59"), QStringLiteral("DCSettingIndex"), 100u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("pwr.plan.ultimate_performance");
        t.category = QStringLiteral("power");
        t.subCategory = QStringLiteral("Processor state");
        t.name = QStringLiteral("Force ultimate performance power plan");
        t.description = QStringLiteral("Configures Windows to run at maximum CPU frequency at all times by setting minimum processor state to 100%.");
        t.iconName = QStringLiteral("flame");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Significantly increases idle power consumption and heat output"),
            QStringLiteral("May reduce CPU lifespan on laptops due to constant high voltage"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\54533251-82be-4824-96c1-47b60b740d00\\893dee8e-2bef-41e0-89c6-b55d0929964c"), QStringLiteral("ACSettingIndex"), 100u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\54533251-82be-4824-96c1-47b60b740d00\\893dee8e-2bef-41e0-89c6-b55d0929964c"), QStringLiteral("DCSettingIndex"), 100u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\54533251-82be-4824-96c1-47b60b740d00\\bc5038f7-23e0-4960-96da-33abaf5935ec"), QStringLiteral("ACSettingIndex"), 100u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\54533251-82be-4824-96c1-47b60b740d00\\bc5038f7-23e0-4960-96da-33abaf5935ec"), QStringLiteral("DCSettingIndex"), 100u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("pwr.usb.power_drain_recovery");
        t.category = QStringLiteral("power");
        t.subCategory = QStringLiteral("USB power");
        t.name = QStringLiteral("Disable USB auto power drain recovery");
        t.description = QStringLiteral("Disables automatic recovery from USB power drain conditions, preventing surprise device reconnections.");
        t.iconName = QStringLiteral("usb");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("USB devices causing power drain no longer auto-reconnect; manual replug required"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\USB\\AutomaticSurpriseRemoval"), QStringLiteral("AttemptRecoveryFromUsbPowerDrain"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("power.ultimate-performance-plan");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Power plan");
        t.name = QStringLiteral("Activate ultimate performance plan");
        t.description = QStringLiteral("Enables and selects the hidden Ultimate Performance power plan. Disables every CPU power-saving feature; trades watts for raw responsiveness.");
        t.iconName = QStringLiteral("bolt");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("This removal is not fully reversible."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$out = powercfg /duplicatescheme e9a42b02-d5df-448d-aa00-03f14749eb61 2>$null; if ($LASTEXITCODE -eq 0 -and $out) {   $guid = ($out -split ':')[1].Trim().Split(' ')[0];   powercfg /setactive $guid 2>$null | Out-Null} else {   $existing = (powercfg /list | Select-String 'Ultimate Performance').Line;   if ($existing) {     $guid = ($existing -split ':')[1].Trim().Split(' ')[0];     powercfg /setactive $guid 2>$null | Out-Null   }}") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("powercfg /setactive 381b4222-f694-41f0-9685-ff5bb260df2e 2>$null | Out-Null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("pwr.cstate.idle_disable");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("C-States");
        t.name = QStringLiteral("Disable CPU idle states");
        t.description = QStringLiteral("Sets IdleDisable=1 via powercfg to keep CPU always in C0 state, preventing all C-state transitions.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::Critical;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("CPU held in C0 at all times; significantly higher idle power and heat"),
            QStringLiteral("Battery life on laptops is severely reduced"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("powercfg.exe /SETACVALUEINDEX SCHEME_CURRENT SUB_PROCESSOR IdleDisable 1") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("powercfg.exe /SETACVALUEINDEX SCHEME_CURRENT SUB_PROCESSOR IdleDisable 0") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("pwr.cstate.processor_idle_disable");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("C-States");
        t.name = QStringLiteral("Disable processor idle (PowerWriteACValueIndex)");
        t.description = QStringLiteral("Uses PowerWriteACValueIndex with the processor idle disable GUID {5d76a2ca-e8c0-402f-a133-2158492d58ad} to prevent the CPU from entering idle states. This is the API-level equivalent of the powercfg IdleDisable setting.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::Critical;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Processor never idles; constant high power draw and elevated temperatures"),
            QStringLiteral("Laptop battery life drops dramatically"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("powercfg.exe /SETACVALUEINDEX SCHEME_CURRENT SUB_PROCESSOR 5d76a2ca-e8c0-402f-a133-2158492d58ad 1") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("powercfg.exe /SETACVALUEINDEX SCHEME_CURRENT SUB_PROCESSOR 5d76a2ca-e8c0-402f-a133-2158492d58ad 0") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("power.disable-hibernation");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Hibernation");
        t.name = QStringLiteral("Disable hibernation entirely");
        t.description = QStringLiteral("Runs powercfg /h off to disable hibernation and delete hiberfil.sys (roughly the size of installed RAM), reclaiming disk space. This also disables Fast Startup.");
        t.iconName = QStringLiteral("moon");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Hybrid sleep falls back to plain sleep - laptops without battery might lose state on power loss"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("powercfg /h off") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("powercfg /h on") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("pwr.mem.disable_compression");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Memory");
        t.name = QStringLiteral("Disable memory compression");
        t.description = QStringLiteral("Disables Windows memory compression via MMAgent - stops the OS from compressing memory pages in RAM.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Higher working-set RAM usage."),
            QStringLiteral("Systems with less than 16 GB RAM may hit out-of-memory conditions sooner."),
            QStringLiteral("Takes effect after reboot."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("Disable-MMAgent -MemoryCompression") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("Enable-MMAgent -MemoryCompression") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("pwr.plan.disk_never");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Sleep timeouts");
        t.name = QStringLiteral("Prevent disk sleep (AC)");
        t.description = QStringLiteral("Sets the disk idle timeout to zero so hard drives never spin down while on AC power.");
        t.iconName = QStringLiteral("database");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Hard drives stay spinning on AC; higher idle power and drive wear"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("powercfg.exe /change disk-timeout-ac 0") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("powercfg.exe /change disk-timeout-ac ") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("pwr.plan.monitor_never");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Sleep timeouts");
        t.name = QStringLiteral("Prevent monitor sleep (AC)");
        t.description = QStringLiteral("Sets the monitor idle timeout to zero so the display never turns off while on AC power.");
        t.iconName = QStringLiteral("monitor");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Display stays on indefinitely on AC; risk of OLED burn-in"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("powercfg.exe /change monitor-timeout-ac 0") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("powercfg.exe /change monitor-timeout-ac ") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("pwr.plan.sleep_never");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Sleep timeouts");
        t.name = QStringLiteral("Prevent system sleep (AC)");
        t.description = QStringLiteral("Sets the standby timeout to zero so the system never enters sleep while on AC power.");
        t.iconName = QStringLiteral("moon");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("System never enters sleep automatically on AC; higher idle power and heat"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("powercfg.exe /change standby-timeout-ac 0") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("powercfg.exe /change standby-timeout-ac ") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("pwr.plan.balanced_tune");
        t.category = QStringLiteral("power");
        t.subCategory = QStringLiteral("Power plan");
        t.name = QStringLiteral("Balanced power plan performance values");
        t.description = QStringLiteral("Applies performance-oriented settings to the Balanced power scheme: sets min processor state to 100%, disables PCI Express and USB power management on AC, and disables core parking.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.reversible = false;
        CommandOp xo0;
        xo0.program = QStringLiteral("powercfg");
        xo0.applyArgs = { QStringLiteral("-setacvalueindex"), QStringLiteral("381b4222-f694-41f0-9685-ff5bb260df2e"), QStringLiteral("54533251-82be-4824-96c1-47b60b740d00"), QStringLiteral("893dee8e-2bef-41e0-89c6-b55d0929964d"), QStringLiteral("100") };
        xo0.revertArgs = { QStringLiteral("/?") };
        t.commandOps.push_back(xo0);
        CommandOp xo1;
        xo1.program = QStringLiteral("powercfg");
        xo1.applyArgs = { QStringLiteral("-setacvalueindex"), QStringLiteral("381b4222-f694-41f0-9685-ff5bb260df2e"), QStringLiteral("54533251-82be-4824-96c1-47b60b740d00"), QStringLiteral("bc5038f7-23e0-4960-96da-33abaf5935ed"), QStringLiteral("100") };
        xo1.revertArgs = { QStringLiteral("/?") };
        t.commandOps.push_back(xo1);
        CommandOp xo2;
        xo2.program = QStringLiteral("powercfg");
        xo2.applyArgs = { QStringLiteral("-setacvalueindex"), QStringLiteral("381b4222-f694-41f0-9685-ff5bb260df2e"), QStringLiteral("501a4d13-42af-4429-9fd1-a8218c268e20"), QStringLiteral("ee12f906-d277-404b-b6da-e5fa1a576df5"), QStringLiteral("0") };
        xo2.revertArgs = { QStringLiteral("/?") };
        t.commandOps.push_back(xo2);
        CommandOp xo3;
        xo3.program = QStringLiteral("powercfg");
        xo3.applyArgs = { QStringLiteral("-setacvalueindex"), QStringLiteral("381b4222-f694-41f0-9685-ff5bb260df2e"), QStringLiteral("2a737441-1930-4402-8d77-b2bebba308a3"), QStringLiteral("48e6b7a6-50f5-4782-a5d4-53bb8f07e226"), QStringLiteral("0") };
        xo3.revertArgs = { QStringLiteral("/?") };
        t.commandOps.push_back(xo3);
        CommandOp xo4;
        xo4.program = QStringLiteral("powercfg");
        xo4.applyArgs = { QStringLiteral("-setacvalueindex"), QStringLiteral("381b4222-f694-41f0-9685-ff5bb260df2e"), QStringLiteral("54533251-82be-4824-96c1-47b60b740d00"), QStringLiteral("3b04d4fd-1cc7-4f23-ab1c-d1337819c4bb"), QStringLiteral("0") };
        xo4.revertArgs = { QStringLiteral("/?") };
        t.commandOps.push_back(xo4);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("pwr.plan.custom_performance");
        t.category = QStringLiteral("power");
        t.subCategory = QStringLiteral("Power plan");
        t.name = QStringLiteral("Custom performance power plan");
        t.description = QStringLiteral("Duplicates the Ultimate Performance power plan with a fixed GUID, sets it active, configures 100% min/max processor state, aggressive performance boost mode, disables USB selective suspend, and unparks all CPU cores for maximum throughput.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.reversible = false;
        CommandOp xo0;
        xo0.program = QStringLiteral("powercfg");
        xo0.applyArgs = { QStringLiteral("-duplicatescheme"), QStringLiteral("e9a42b02-d5df-448d-aa00-03f14749eb61"), QStringLiteral("99999999-9999-9999-9999-999999999999") };
        xo0.revertArgs = { QStringLiteral("/?") };
        t.commandOps.push_back(xo0);
        CommandOp xo1;
        xo1.program = QStringLiteral("powercfg");
        xo1.applyArgs = { QStringLiteral("-setactive"), QStringLiteral("99999999-9999-9999-9999-999999999999") };
        xo1.revertArgs = { QStringLiteral("/?") };
        t.commandOps.push_back(xo1);
        CommandOp xo2;
        xo2.program = QStringLiteral("powercfg");
        xo2.applyArgs = { QStringLiteral("-setacvalueindex"), QStringLiteral("scheme_current"), QStringLiteral("sub_processor"), QStringLiteral("PROCTHROTTLEMIN"), QStringLiteral("100") };
        xo2.revertArgs = { QStringLiteral("/?") };
        t.commandOps.push_back(xo2);
        CommandOp xo3;
        xo3.program = QStringLiteral("powercfg");
        xo3.applyArgs = { QStringLiteral("-setacvalueindex"), QStringLiteral("scheme_current"), QStringLiteral("sub_processor"), QStringLiteral("PROCTHROTTLEMAX"), QStringLiteral("100") };
        xo3.revertArgs = { QStringLiteral("/?") };
        t.commandOps.push_back(xo3);
        CommandOp xo4;
        xo4.program = QStringLiteral("powercfg");
        xo4.applyArgs = { QStringLiteral("-setacvalueindex"), QStringLiteral("scheme_current"), QStringLiteral("sub_processor"), QStringLiteral("PERFBOOSTMODE"), QStringLiteral("2") };
        xo4.revertArgs = { QStringLiteral("/?") };
        t.commandOps.push_back(xo4);
        CommandOp xo5;
        xo5.program = QStringLiteral("powercfg");
        xo5.applyArgs = { QStringLiteral("-setacvalueindex"), QStringLiteral("scheme_current"), QStringLiteral("2a737441-1930-4402-8d77-b2bebba308a3"), QStringLiteral("48e6b7a6-50f5-4782-a5d4-53bb8f07e226"), QStringLiteral("0") };
        xo5.revertArgs = { QStringLiteral("/?") };
        t.commandOps.push_back(xo5);
        CommandOp xo6;
        xo6.program = QStringLiteral("powercfg");
        xo6.applyArgs = { QStringLiteral("-setacvalueindex"), QStringLiteral("scheme_current"), QStringLiteral("sub_processor"), QStringLiteral("CPMINCORES"), QStringLiteral("100") };
        xo6.revertArgs = { QStringLiteral("/?") };
        t.commandOps.push_back(xo6);
        CommandOp xo7;
        xo7.program = QStringLiteral("powercfg");
        xo7.applyArgs = { QStringLiteral("-setactive"), QStringLiteral("scheme_current") };
        xo7.revertArgs = { QStringLiteral("/?") };
        t.commandOps.push_back(xo7);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("pwr.plan.hibernate_off");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Hibernation");
        t.name = QStringLiteral("Disable hibernation (powercfg)");
        t.description = QStringLiteral("Runs 'powercfg -h off' to disable hibernation and delete hiberfil.sys, freeing disk space equal to RAM size.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Hibernation unavailable; Fast Startup disabled and boot takes slightly longer"),
            QStringLiteral("hiberfil.sys deleted; restoring hibernation recreates it at RAM-sized disk cost"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("powercfg.exe /hibernate off") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("powercfg.exe /hibernate on") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("pwr.plan.high_perf_tune");
        t.category = QStringLiteral("power");
        t.subCategory = QStringLiteral("Power plan");
        t.name = QStringLiteral("High performance power plan values");
        t.description = QStringLiteral("Applies performance-oriented settings to the High Performance power scheme: disables turbo boost limiting, disables PCI Express power management, disables USB suspend, sets min/max processor state to 100%, and disables core parking.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.reversible = false;
        CommandOp xo0;
        xo0.program = QStringLiteral("powercfg");
        xo0.applyArgs = { QStringLiteral("-setacvalueindex"), QStringLiteral("8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c"), QStringLiteral("54533251-82be-4824-96c1-47b60b740d00"), QStringLiteral("be337238-0d82-4146-a960-4f3749d470c7"), QStringLiteral("0") };
        xo0.revertArgs = { QStringLiteral("/?") };
        t.commandOps.push_back(xo0);
        CommandOp xo1;
        xo1.program = QStringLiteral("powercfg");
        xo1.applyArgs = { QStringLiteral("-setacvalueindex"), QStringLiteral("8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c"), QStringLiteral("54533251-82be-4824-96c1-47b60b740d00"), QStringLiteral("45bcc044-d885-43e2-8605-ee0ec6e96b59"), QStringLiteral("100") };
        xo1.revertArgs = { QStringLiteral("/?") };
        t.commandOps.push_back(xo1);
        CommandOp xo2;
        xo2.program = QStringLiteral("powercfg");
        xo2.applyArgs = { QStringLiteral("-setacvalueindex"), QStringLiteral("8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c"), QStringLiteral("54533251-82be-4824-96c1-47b60b740d00"), QStringLiteral("893dee8e-2bef-41e0-89c6-b55d0929964d"), QStringLiteral("100") };
        xo2.revertArgs = { QStringLiteral("/?") };
        t.commandOps.push_back(xo2);
        CommandOp xo3;
        xo3.program = QStringLiteral("powercfg");
        xo3.applyArgs = { QStringLiteral("-setacvalueindex"), QStringLiteral("8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c"), QStringLiteral("54533251-82be-4824-96c1-47b60b740d00"), QStringLiteral("bc5038f7-23e0-4960-96da-33abaf5935ed"), QStringLiteral("100") };
        xo3.revertArgs = { QStringLiteral("/?") };
        t.commandOps.push_back(xo3);
        CommandOp xo4;
        xo4.program = QStringLiteral("powercfg");
        xo4.applyArgs = { QStringLiteral("-setacvalueindex"), QStringLiteral("8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c"), QStringLiteral("501a4d13-42af-4429-9fd1-a8218c268e20"), QStringLiteral("ee12f906-d277-404b-b6da-e5fa1a576df5"), QStringLiteral("0") };
        xo4.revertArgs = { QStringLiteral("/?") };
        t.commandOps.push_back(xo4);
        CommandOp xo5;
        xo5.program = QStringLiteral("powercfg");
        xo5.applyArgs = { QStringLiteral("-setacvalueindex"), QStringLiteral("8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c"), QStringLiteral("2a737441-1930-4402-8d77-b2bebba308a3"), QStringLiteral("48e6b7a6-50f5-4782-a5d4-53bb8f07e226"), QStringLiteral("0") };
        xo5.revertArgs = { QStringLiteral("/?") };
        t.commandOps.push_back(xo5);
        CommandOp xo6;
        xo6.program = QStringLiteral("powercfg");
        xo6.applyArgs = { QStringLiteral("-setacvalueindex"), QStringLiteral("8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c"), QStringLiteral("54533251-82be-4824-96c1-47b60b740d00"), QStringLiteral("3b04d4fd-1cc7-4f23-ab1c-d1337819c4bb"), QStringLiteral("0") };
        xo6.revertArgs = { QStringLiteral("/?") };
        t.commandOps.push_back(xo6);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("pwr.plan.import_pow");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Power plan");
        t.name = QStringLiteral("Import custom power plan (.pow)");
        t.description = QStringLiteral("Imports a pre-configured opnaive.pow power plan from the app temp directory and sets it active (does nothing if the file is missing). It does not delete other power plans. Reverting runs powercfg -restoredefaultschemes, which resets all schemes to Windows defaults.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Active power plan replaced; previous Balanced/Power Saver settings overridden"),
            QStringLiteral("Other power plans may be deleted; custom OEM schemes lost without backup"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$p = Join-Path $env:TEMP 'opnaive.pow'; if (-not (Test-Path $p)) { exit 1 }; powercfg.exe /import $p 99999999-9999-9999-9999-999999999999; powercfg.exe /setactive 99999999-9999-9999-9999-999999999999; Remove-Item $p -Force") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("powercfg.exe -restoredefaultschemes") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("pwr.cstate.registry_healing");
        t.category = QStringLiteral("power");
        t.name = QStringLiteral("Disable registry self-healing");
        t.description = QStringLiteral("Disables automatic registry self-healing and periodic backup operations to reduce background disk I/O during gameplay.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Registry hive corruption will not auto-repair; keep a System Restore point before applying"),
            QStringLiteral("Periodic registry backup under RegBack stops being refreshed"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Configuration Manager"), QStringLiteral("SelfHealingEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Configuration Manager"), QStringLiteral("EnablePeriodicBackup"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("pwr.gpu.write_combining");
        t.category = QStringLiteral("power");
        t.name = QStringLiteral("Disable GPU write combining");
        t.description = QStringLiteral("Disables write combining on GPU memory path to reduce latency at the potential cost of throughput.");
        t.iconName = QStringLiteral("monitor");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("GPU memory throughput reduced; rendering-heavy workloads may drop frames"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Power"), QStringLiteral("DisableWriteCombining"), 1u, 0u),
        };
        c.push_back(t);
    }
}

} // namespace vth::tweaks::detail
