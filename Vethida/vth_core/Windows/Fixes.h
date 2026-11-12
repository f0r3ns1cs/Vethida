#pragma once

#include <QString>

// One-click repairs for common Windows faults. They need elevation, and some run
// for minutes or hours, so call them from a worker.
namespace vth::win::fixes
{
    enum class FixId {
        // Sound, printing, devices
        RestartAudioServices,
        RestoreBluetoothServices,
        ClearPrintQueue,

        // Network
        RenewNetworkAddress,
        RepairConnectivityDetection,
        ClearProxySettings,
        RepairNetworkStack,
        ResetFirewallRules,

        // Windows and its apps
        ResetWindowsUpdate,
        ResetStoreCache,
        RepairStartMenuAndSearch,
        SyncSystemClock,
        ClearOrganizationPolicies,

        // Explorer and search
        RebuildIconCache,
        ResetFolderViews,
        RebuildSearchIndex,
        ClearFontCache,
        RepairRecycleBin,

        // Graphics and games
        ClearShaderCaches,
        ClearStandbyMemory,
        RestoreNvidiaControlPanel,

        // Storage and power
        ClearTemporaryFiles,
        CompactComponentStore,
        RestoreDefaultPowerPlans,

        // Deep repair, minutes to hours
        RunSystemFileCheck,
        ScheduleDiskCheck,
        SecureWipeFreeSpace,

        RestorePowerService,
        RestoreWindowsInstaller,
        RepairMicrosoftStore,
        RepairSettingsApp,
        RestoreMicCameraAccess,
        RestoreGameServices,
        ResetInputBuffers,
        RestoreNetworkServices,
        RestoreEventLog,
        RestorePageFile,
        FixXboxNetworking,
        FixGamingOverlayPopup,
        FixTypingInSearch,
        FixWindowSnapping,
        ResetTimerSettings,
        FixAntiCheatBlockers,
    };

    struct FixResult {
        bool    ok          = false;
        bool    needsReboot = false;
        QString message;
        QString detail;    // tool output or error text, may be empty
    };

    FixResult run(FixId id);

    struct Symptom {
        bool    present = false;
        QString evidence;  // what was found, empty if not present
    };

    // Whether the fault is present right now. Read-only, but it can block. Most
    // fixes have no detector, and those always report not present.
    Symptom detect(FixId id);
}
