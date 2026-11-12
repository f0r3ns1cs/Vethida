#include "TweakEngine.h"

// Security toggles. Every entry here trades away a real security control and
// carries a warning.

namespace vth::tweaks::detail {

namespace {
CommandOp bcdedit(const QStringList& applyArgs, const QStringList& revertArgs) {
    CommandOp op;
    op.program = QStringLiteral("bcdedit");
    op.applyArgs = applyArgs;
    op.revertArgs = revertArgs;
    return op;
}
}

void appendSecurity(QVector<TweakSpec>& c) {
    {
        TweakSpec t;
        t.id = QStringLiteral("sec.defender-deprioritize");
        t.category = QStringLiteral("security");
        t.subCategory = QStringLiteral("Defender");
        t.name = QStringLiteral("Deprioritize Defender (MsMpEng)");
        t.description = QStringLiteral(
            "Keeps Defender running but pins its scan engine MsMpEng.exe to Idle CPU and "
            "Very-Low I/O priority, so background scans stop stealing foreground cycles.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.concern = QStringLiteral(
            "safer alternative to a full disable: protection stays on, only its scheduling "
            "priority drops. Real-time scans are still synchronous, so the win is bounded.");
        const QString ifeo = QStringLiteral(
            "HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution "
            "Options\\MsMpEng.exe\\PerfOptions");
        t.registryOps = {
            dwordOp(ifeo, QStringLiteral("CpuPriorityClass"), 1, std::nullopt),
            dwordOp(ifeo, QStringLiteral("IoPriority"), 0, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sec.disable-coinstallers");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Driver installation");
        t.name = QStringLiteral("Block driver co-installers");
        t.description = QStringLiteral(
            "Stops Windows from auto-installing vendor bloat (Razer Synapse, Logitech "
            "G HUB, etc.) when a peripheral is plugged in.");
        t.iconName = QStringLiteral("ban");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Device Installer"),
                    QStringLiteral("DisableCoInstallers"), 1, 0),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sec.smartscreen-off");
        t.category = QStringLiteral("security");
        t.name = QStringLiteral("Disable SmartScreen");
        t.description = QStringLiteral(
            "Turns off the SmartScreen reputation check for downloaded files and apps.");
        t.iconName = QStringLiteral("shield-off");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = { QStringLiteral("Removes warnings about unrecognized/malicious downloads.") };
        t.registryOps = {
            stringOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer"),
                     QStringLiteral("SmartScreenEnabled"), QStringLiteral("Off"), QStringLiteral("Warn")),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer"),
                     QStringLiteral("SmartScreenEnabled"), QStringLiteral("Off"), QStringLiteral("Warn")),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System"),
                    QStringLiteral("EnableSmartScreen"), 0, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sec.uac-off");
        t.category = QStringLiteral("security");
        t.name = QStringLiteral("Disable UAC");
        t.description = QStringLiteral(
            "Disables User Account Control: no elevation prompts, no secure desktop, no "
            "installer detection.");
        t.iconName = QStringLiteral("shield-off");
        t.impact = Impact::Critical;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Every process runs with full admin rights silently. Strongly "
                           "discouraged on any machine that browses the web or opens documents."),
        };
        t.concern = QStringLiteral(
            "risky: turning off EnableLUA also breaks UWP/Store apps, which refuse to run "
            "without UAC. Acceptable only on an isolated single-user box.");
        const QString sys = QStringLiteral(
            "HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System");
        t.registryOps = {
            dwordOp(sys, QStringLiteral("EnableLUA"), 0, 1),
            dwordOp(sys, QStringLiteral("ConsentPromptBehaviorAdmin"), 0, 5),
            dwordOp(sys, QStringLiteral("PromptOnSecureDesktop"), 0, 1),
            dwordOp(sys, QStringLiteral("EnableInstallerDetection"), 0, 1),
            dwordOp(sys, QStringLiteral("EnableSecureUIAPaths"), 0, 1),
            dwordOp(sys, QStringLiteral("EnableVirtualization"), 0, 1),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sec.bitlocker-prevent");
        t.category = QStringLiteral("security");
        t.name = QStringLiteral("Prevent automatic BitLocker");
        t.description = QStringLiteral(
            "Stops Windows 11 from silently auto-enabling BitLocker device encryption.");
        t.iconName = QStringLiteral("lock-open");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.concern = QStringLiteral(
            "for users who don't want surprise encryption (e.g. risk of losing the recovery "
            "key). Does not decrypt an already-encrypted drive.");
        const QString fve = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\FVE");
        t.registryOps = {
            dwordOp(fve, QStringLiteral("UseAdvancedStartup"), 0, std::nullopt),
            dwordOp(fve, QStringLiteral("EnableBDEWithNoTPM"), 0, std::nullopt),
            dwordOp(fve, QStringLiteral("UseTPM"), 0, std::nullopt),
            dwordOp(fve, QStringLiteral("UseTPMPIN"), 0, std::nullopt),
            dwordOp(fve, QStringLiteral("UseTPMKey"), 0, std::nullopt),
            dwordOp(fve, QStringLiteral("UseTPMKeyPIN"), 0, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sec.elam-off");
        t.category = QStringLiteral("security");
        t.name = QStringLiteral("Disable Early-Launch Anti-Malware");
        t.description = QStringLiteral(
            "Sets the ELAM driver-load policy to allow all boot drivers and tells the boot "
            "loader to skip ELAM drivers.");
        t.iconName = QStringLiteral("shield-off");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral(
            "risky: ELAM is the boot-time check that blocks malicious boot drivers. "
            "Disabling it shaves a sliver off boot but removes early rootkit defense.");
        // concern is never shown in the app, so the warning has to say that this
        // switches off a named protection.
        t.warnings = {
            QStringLiteral("Removes early rootkit defense: ELAM is the boot-time check that stops malicious drivers loading before Windows starts"),
            QStringLiteral("Saves a fraction of boot time and nothing else"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\EarlyLaunch"),
                    QStringLiteral("DriverLoadPolicy"), 0, std::nullopt),
        };
        t.commandOps = {
            bcdedit({ QStringLiteral("/set"), QStringLiteral("disableelamdrivers"), QStringLiteral("Yes") },
                    { QStringLiteral("/deletevalue"), QStringLiteral("disableelamdrivers") }),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sec.lsa-ppl-off");
        t.category = QStringLiteral("security");
        t.name = QStringLiteral("Disable LSA protection (RunAsPPL)");
        t.description = QStringLiteral(
            "Turns off LSASS Protected-Process-Light, which some debugging and credential "
            "tools require.");
        t.iconName = QStringLiteral("shield-off");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral(
            "risky: RunAsPPL is what stops tools like Mimikatz from reading credentials out "
            "of LSASS. No performance benefit; disable only if a specific tool needs it.");
        t.warnings = {
            QStringLiteral("LSA Protection is what stops credential-dumping tools reading your saved passwords and sign-in tokens out of memory"),
            QStringLiteral("No performance benefit at all - turn this on only if a specific tool requires it"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Lsa"),
                    QStringLiteral("RunAsPPL"), 0, 1),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sec.vuln-driver-blocklist-off");
        t.category = QStringLiteral("security");
        t.name = QStringLiteral("Disable vulnerable driver blocklist");
        t.description = QStringLiteral(
            "Turns off Microsoft's blocklist of known-exploitable drivers, required by some "
            "older kernel-mode tools (RW-Everything, MSI Afterburner helpers).");
        t.iconName = QStringLiteral("shield-off");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral(
            "risky: this list blocks drivers actively used in BYOVD attacks. Re-enable as "
            "soon as the tool that needs it is done.");
        t.warnings = {
            QStringLiteral("The blocked drivers are ones actively used in real attacks to get kernel access on a machine the attacker already reached"),
            QStringLiteral("Turn this back on as soon as the tool that needed it is finished"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\CI\\Config"),
                    QStringLiteral("VulnerableDriverBlocklistEnable"), 0, 1),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sec.disable-mitigations-extra");
        t.category = QStringLiteral("security");
        t.name = QStringLiteral("Disable Win32k/DMA guard mitigations");
        t.description = QStringLiteral(
            "Disallows Win32k system calls in the kernel policy check, turns off the HVCI "
            "memory-attestation requirement, and disables Kernel DMA Protection (DmaGuard) "
            "and BitLocker's external-DMA-under-lock block.");
        t.iconName = QStringLiteral("shield-off");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral(
            "risky: disables Spectre/Meltdown-class and DMA-attack mitigations for "
            "performance; real security loss, particularly on laptops exposed to Thunderbolt "
            "DMA attacks.");
        t.warnings = {
            QStringLiteral("Turns off Spectre/Meltdown-class CPU mitigations, so processes can read memory belonging to other processes"),
            QStringLiteral("Turns off Kernel DMA Protection: a device plugged into Thunderbolt or USB4 can read system memory directly, including while the machine is locked"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Kernel"),
                    QStringLiteral("DisallowWin32kSystemCalls"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DeviceGuard"),
                    QStringLiteral("HVCIMATRequired"), 0, std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\DeviceGuard\\Scenarios\\DmaGuard"),
                     QStringLiteral("Enable"), QStringLiteral("N"), std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\FVE"),
                    QStringLiteral("DisableExternalDMAUnderLock"), 0, std::nullopt),
        };
        c.push_back(t);
    }
}

} // namespace vth::tweaks::detail
