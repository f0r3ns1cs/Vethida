#include "TweakEngine.h"

// Distinct Win32PrioritySeparation presets, a DWM input latency variant, two
// security hardening toggles that undo existing disable tweaks, the shutdown
// reason prompt, and Game Mode auto-activation.

namespace vth::tweaks::detail {

void appendDefsGaps(QVector<TweakSpec>& c) {
    const QString kPriority = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PriorityControl");
    const QString kWindows  = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows");

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.sched.w32ps_latency");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("CPU scheduling");
        t.name = QStringLiteral("Process scheduling: lowest latency (21)");
        t.description = QStringLiteral("Sets Win32PrioritySeparation to 21 (0x15): short, variable CPU time-slices with a small foreground bias. It measurably changes the thread scheduler but the real-world gaming difference is minor; it mainly helps if you also set the game's process priority manually. Reverts to the Windows default (2).");
        t.iconName = QStringLiteral("gauge");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Removes automatic foreground boost - best paired with a manual priority for the game."),
        };
        t.registryOps = {
            dwordOp(kPriority, QStringLiteral("Win32PrioritySeparation"), 21u, 2u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.sched.w32ps_balanced");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("CPU scheduling");
        t.name = QStringLiteral("Process scheduling: balanced (22)");
        t.description = QStringLiteral("Sets Win32PrioritySeparation to 22 (0x16): short, variable CPU time-slices with a strong foreground bias, giving the active app markedly more CPU than background tasks. A reasonable default for gaming while Discord/streaming run behind. Reverts to the Windows default (2).");
        t.iconName = QStringLiteral("gauge");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.registryOps = {
            dwordOp(kPriority, QStringLiteral("Win32PrioritySeparation"), 22u, 2u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sec.enable-smartscreen-shell");
        t.category = QStringLiteral("security");
        t.subCategory = QStringLiteral("SmartScreen");
        t.name = QStringLiteral("Enable SmartScreen for shell-launched apps");
        t.description = QStringLiteral("Forces SmartScreen reputation checks on every executable launched from File Explorer. Helps catch unsigned and unrecognized binaries.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System"), QStringLiteral("EnableSmartScreen"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sec.enable-uac-admin-approval");
        t.category = QStringLiteral("security");
        t.subCategory = QStringLiteral("UAC");
        t.name = QStringLiteral("Force UAC consent for admin operations");
        t.description = QStringLiteral("Sets ConsentPromptBehaviorAdmin to 2, so every elevation shows a consent prompt on the secure desktop, including Windows binaries that the default (value 5) would elevate silently. This hardens UAC; it does not change behavior of the built-in Administrator account (that is governed by FilterAdministratorToken). Reverts to the Windows default (5).");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"), QStringLiteral("ConsentPromptBehaviorAdmin"), 2u, 5u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.show-detail-on-shutdown");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Shutdown");
        t.name = QStringLiteral("Show shutdown reason / app names");
        t.description = QStringLiteral("Enables the Shutdown Event Tracker (ShutdownReasonOn=1): Windows prompts for a reason each time you shut down or restart, and after an unexpected shutdown, recording it in the event log for troubleshooting. Note this adds a prompt to shutdowns; it does not reveal the names of apps blocking shutdown. Reverts to disabled (0).");
        t.iconName = QStringLiteral("info");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Reliability"), QStringLiteral("ShutdownReasonOn"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.game.mode");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Game Mode");
        t.name = QStringLiteral("Enable Game Mode auto-activation");
        t.description = QStringLiteral("Enables Windows Game Mode auto-activation (AllowAutoGameMode and AutoGameModeEnabled). Prioritizes the foreground game for CPU/GPU and suppresses background updates and driver resets during gameplay.");
        t.iconName = QStringLiteral("device-gamepad");
        t.impact = Impact::Low;
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\GameBar"), QStringLiteral("AllowAutoGameMode"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\GameBar"), QStringLiteral("AutoGameModeEnabled"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.wu.driver_search_target");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Windows Update");
        t.name = QStringLiteral("Disable driver auto-install and lock Windows version");
        t.description = QStringLiteral("Sets SearchOrderConfig=3 so Windows Update never automatically installs drivers (useful for keeping stable GPU drivers), and TargetReleaseVersion=1. Note: TargetReleaseVersion=1 alone does not pin your Windows version - it only takes effect when the companion TargetReleaseVersionInfo value (the specific version, e.g. '23H2') is also set, which this tweak does not write. Both revert to 0.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DriverSearching"), QStringLiteral("SearchOrderConfig"), 3u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate"), QStringLiteral("TargetReleaseVersion"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.latency.dwm_input");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("Latency");
        t.name = QStringLiteral("Disable DWM input processing");
        t.description = QStringLiteral("Disables Desktop Window Manager input processing via IO completion ports and turns off desktop heap logging. Reduces DWM overhead for lower input-to-display latency.");
        t.iconName = QStringLiteral("stopwatch");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.registryOps = {
            dwordOp(kWindows, QStringLiteral("DesktopHeapLogging"), 0u, 1u),
            dwordOp(kWindows, QStringLiteral("DwmInputUsesIoCompletionPort"), 0u, 1u),
            dwordOp(kWindows, QStringLiteral("EnableDwmInputProcessing"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.input.data_queues");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Input");
        t.name = QStringLiteral("Reduce input data queue sizes");
        t.description = QStringLiteral("Lowers the keyboard and mouse class driver input queues from 100 to 50 entries (0x32), halving the buffer each one holds.");
        t.iconName = QStringLiteral("keyboard");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\kbdclass\\Parameters"), QStringLiteral("KeyboardDataQueueSize"), 50u, 100u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\mouclass\\Parameters"), QStringLiteral("MouseDataQueueSize"), 50u, 100u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sec.dma_guard.disable");
        t.category = QStringLiteral("security");
        t.subCategory = QStringLiteral("VBS");
        t.declaredTier = vth::models::ProductTier::Pro;
        t.name = QStringLiteral("Disable Kernel DMA Protection");
        t.description = QStringLiteral("Disables Kernel DMA Protection (DeviceGuard DmaGuard scenario Enable=0 and FVE DisableExternalDMAUnderLock=0). Reduces DMA-related latency at the cost of Thunderbolt/PCIe DMA attack protection.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Removes protection against physical DMA attacks (Thunderbolt/PCIe)."),
            QStringLiteral("Encryption keys become extractable via forensic DMA tools."),
            QStringLiteral("Some anti-cheat (Vanguard, FACEIT) require Kernel DMA Protection ON."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\DeviceGuard\\Scenarios\\DmaGuard"), QStringLiteral("Enable"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\FVE"), QStringLiteral("DisableExternalDMAUnderLock"), 0u, std::nullopt),
        };
        c.push_back(t);
    }
}

}
