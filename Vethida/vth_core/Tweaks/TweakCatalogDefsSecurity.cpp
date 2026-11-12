#include "TweakEngine.h"

namespace vth::tweaks::detail {

void appendDefsSecurity(QVector<TweakSpec>& c) {
    {
        TweakSpec t;
        t.id = QStringLiteral("sec.disable-mdns-responder");
        t.category = QStringLiteral("security");
        t.subCategory = QStringLiteral("Network hardening");
        t.name = QStringLiteral("Disable mDNS responder");
        t.description = QStringLiteral("Stops Windows from answering multicast DNS queries on the LAN. Reduces broadcast surface; rarely needed on Windows desktops.");
        t.iconName = QStringLiteral("lock");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Dnscache\\Parameters"), QStringLiteral("EnableMDNS"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("security.disable_defender_sample_submission");
        t.category = QStringLiteral("security");
        t.subCategory = QStringLiteral("Defender");
        t.name = QStringLiteral("Disable Defender automatic sample submission");
        t.description = QStringLiteral("Stops Windows Defender from automatically uploading suspicious files to Microsoft.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("New/unknown threats may take longer to be identified without cloud analysis"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows Defender\\Spynet"), QStringLiteral("SubmitSamplesConsent"), 2u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sec.disable-wsh");
        t.category = QStringLiteral("security");
        t.subCategory = QStringLiteral("Script host");
        t.name = QStringLiteral("Disable Windows Script Host");
        t.description = QStringLiteral("Stops .vbs / .js / .wsf scripts from executing via wscript / cscript. Closes a long-standing macro-attack vector. May break enterprise tooling that relies on legacy VBS scripts.");
        t.iconName = QStringLiteral("script");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Any in-house tool, third-party installer, or legacy .vbs / .js script will stop running"),
            QStringLiteral("Some Microsoft system installers use cscript internally - patch experiences may break"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows Script Host\\Settings"), QStringLiteral("Enabled"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sec.enable-controlled-folder-access");
        t.category = QStringLiteral("security");
        t.subCategory = QStringLiteral("Defender");
        t.name = QStringLiteral("Enable Controlled Folder Access");
        t.description = QStringLiteral("Activates Microsoft Defender's anti-ransomware folder guard. Blocks unauthorized processes from writing to user document folders.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Custom build tools or sync clients may need to be manually allow-listed in Defender"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows Defender\\Windows Defender Exploit Guard\\Controlled Folder Access"), QStringLiteral("EnableControlledFolderAccess"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sec.enable-tamper-protection");
        t.category = QStringLiteral("security");
        t.subCategory = QStringLiteral("Defender");
        t.name = QStringLiteral("Enable Defender Tamper Protection");
        t.description = QStringLiteral("Attempts to turn on Microsoft Defender Tamper Protection by writing TamperProtection=5. Note: on current Windows this key is protected and usually can only be changed reliably through the Windows Security app, so a registry write may not take effect. Tamper Protection is not required for SmartScreen.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Once on, Defender refuses other registry edits to its own configuration - revert via the Windows Security app UI"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows Defender\\Features"), QStringLiteral("TamperProtection"), 5u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sec.lsa-ppl-on");
        t.category = QStringLiteral("security");
        t.subCategory = QStringLiteral("Credential protection");
        t.name = QStringLiteral("Enable LSA protected process light (PPL)");
        t.description = QStringLiteral("Runs LSASS as a protected process, so code without a verified Microsoft signature cannot read its memory. The main defense against credential-dumping tools like Mimikatz.");
        t.iconName = QStringLiteral("shield-check");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Plug-ins that hook into LSASS - some legacy SSO clients and old credential providers - stop loading"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Lsa"), QStringLiteral("RunAsPPL"), 1u, 0u),
        };
        c.push_back(t);
    }


    {
        TweakSpec t;
        t.id = QStringLiteral("sec.disable-smbv1");
        t.category = QStringLiteral("security");
        t.subCategory = QStringLiteral("Legacy protocols");
        t.name = QStringLiteral("Disable SMBv1");
        t.description = QStringLiteral("Removes the SMBv1 protocol stack. EternalBlue-class vulnerabilities; should always be off on consumer hardware.");
        t.iconName = QStringLiteral("lock");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Pre-2014 NAS / printer / industrial control hardware that only speaks SMBv1 will lose connectivity"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("Disable-WindowsOptionalFeature -Online -FeatureName SMB1Protocol -NoRestart -ErrorAction SilentlyContinue | Out-Null; Disable-WindowsOptionalFeature -Online -FeatureName SMB1Protocol-Client -NoRestart -ErrorAction SilentlyContinue | Out-Null; Disable-WindowsOptionalFeature -Online -FeatureName SMB1Protocol-Server -NoRestart -ErrorAction SilentlyContinue | Out-Null") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("Enable-WindowsOptionalFeature -Online -FeatureName SMB1Protocol -All -NoRestart -ErrorAction SilentlyContinue | Out-Null") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sec.defender.deprioritize");
        t.category = QStringLiteral("security");
        t.name = QStringLiteral("Deprioritize Defender to idle CPU class");
        t.description = QStringLiteral("Pins Defender scan-engine processes to Idle CPU priority class - Defender still runs but only uses CPU when nothing else needs it.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Disables CPU/kernel security mitigation - increases attack surface."),
            QStringLiteral("Defender real-time scanning response is delayed under CPU load."),
            QStringLiteral("Full scans take significantly longer to complete."),
            QStringLiteral("vanguard"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\MsMpEng.exe\\PerfOptions"), QStringLiteral("CpuPriorityClass"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\MsMpEngCP.exe\\PerfOptions"), QStringLiteral("CpuPriorityClass"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sec.defender.granular_controls");
        t.category = QStringLiteral("security");
        t.name = QStringLiteral("Reduce Defender scan scope (archive / network / removable / cloud)");
        // Narrows scan scope (archive, network, removable, catch-up) and cloud
        // reporting. Real-time protection stays on.
        t.description = QStringLiteral("Narrows Defender's scan scope - archive, network-share, removable-drive and catch-up scans - and lowers cloud/telemetry chatter. Real-time protection stays ON and Defender services keep running (Security Center stays green).");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Files in archives, network shares, and USB drives are no longer scanned on-demand - a real reduction in coverage."),
        };
        t.concern = QStringLiteral("reduces AV scan coverage; opt-in only, never recommended. Real-time protection is intentionally left enabled.");
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows Defender\\Spynet"), QStringLiteral("SpynetReporting"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows Defender\\Spynet"), QStringLiteral("SubmitSamplesConsent"), 2u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows Defender\\Reporting"), QStringLiteral("DisableEnhancedNotifications"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows Defender\\Scan"), QStringLiteral("DisableArchiveScanning"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows Defender\\Scan"), QStringLiteral("DisableCatchupFullScan"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows Defender\\Scan"), QStringLiteral("DisableCatchupQuickScan"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows Defender\\Scan"), QStringLiteral("DisableRemovableDriveScanning"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows Defender\\Scan"), QStringLiteral("DisableScanningNetworkFiles"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sec.lsa.disable_protection");
        t.category = QStringLiteral("security");
        t.name = QStringLiteral("Disable LSA protected process light (PPL)");
        t.description = QStringLiteral("Disables LSASS protected-process attestation - removes the extra check that blocks non-PPL code from injecting LSASS memory. Simplifies debugger and auth-plugin interop; credential-dumping tools work freely with admin rights.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Disables CPU/kernel security mitigation - increases attack surface."),
            QStringLiteral("Credential-dumping tools (Mimikatz, etc.) can extract NTLM/Kerberos from LSASS."),
            QStringLiteral("Pass-the-hash attacks become viable on the machine."),
            QStringLiteral("vanguard"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Lsa"), QStringLiteral("RunAsPPL"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Lsa"), QStringLiteral("RunAsPPLBoot"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Lsa"), QStringLiteral("LsaConfigFlags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System"), QStringLiteral("RunAsPPL"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sec.vbs.disable");
        t.category = QStringLiteral("security");
        t.name = QStringLiteral("Disable virtualization-based security (full - incl. HVCI scenarios)");
        t.description = QStringLiteral("Disables the Windows hypervisor shim (VBS, HVCI, Credential Guard). Scrubs the full set of scenario keys needed on Windows 11 22H2+. Can recover a few percent CPU in games - but anti-cheats detect the change.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::Critical;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Disables CPU/kernel security mitigation - increases attack surface."),
            QStringLiteral("HVCI disabled - kernel-mode code integrity no longer enforced."),
            QStringLiteral("Credential Guard unavailable while VBS is off."),
            QStringLiteral("Hypervisor-enforced memory protection disabled."),
            QStringLiteral("Anti-cheats (Vanguard, EAC, BattlEye) detect VBS/HVCI-off and can BLOCK the game or ban the account. Check every game before enabling."),
        };
        t.concern = QStringLiteral("highest-risk gate: anti-cheat lockout + HVCI/VBS loss. Never default-on; per-game anti-cheat warning mandatory.");
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\DeviceGuard"), QStringLiteral("EnableVirtualizationBasedSecurity"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\DeviceGuard"), QStringLiteral("RequirePlatformSecurityFeatures"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\DeviceGuard"), QStringLiteral("HypervisorEnforcedCodeIntegrity"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\DeviceGuard"), QStringLiteral("Locked"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\DeviceGuard\\Scenarios\\HypervisorEnforcedCodeIntegrity"), QStringLiteral("Enabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\DeviceGuard\\Scenarios\\HypervisorEnforcedCodeIntegrity"), QStringLiteral("Locked"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\DeviceGuard\\Scenarios\\HypervisorEnforcedCodeIntegrity"), QStringLiteral("WasEnabledBy"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DeviceGuard"), QStringLiteral("EnableVirtualizationBasedSecurity"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DeviceGuard"), QStringLiteral("HVCIMATRequired"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sec.smartscreen.extended");
        t.category = QStringLiteral("security");
        t.name = QStringLiteral("Disable extended SmartScreen and app install control");
        t.description = QStringLiteral("Disables additional SmartScreen policy manager values, Edge phishing filter, app install control, web content evaluation, and Defender SmartScreen configuration.");
        t.iconName = QStringLiteral("shield-off");
        t.impact = Impact::Critical;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Edge phishing filter disabled"),
            QStringLiteral("App install control set to Anywhere"),
            QStringLiteral("Web content evaluation disabled"),
            QStringLiteral("SmartScreen shell integration removed"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\Browser\\AllowSmartScreen"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\SmartScreen\\EnableSmartScreenInShell"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\SmartScreen\\EnableAppInstallControl"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\SmartScreen\\PreventOverrideForFilesInShell"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\AppContainer\\Storage\\microsoft.microsoftedge_8wekyb3d8bbwe\\MicrosoftEdge\\PhishingFilter"), QStringLiteral("EnabledV9"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\AppContainer\\Storage\\microsoft.microsoftedge_8wekyb3d8bbwe\\MicrosoftEdge\\PhishingFilter"), QStringLiteral("PreventOverride"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Edge"), QStringLiteral("SmartScreenEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows Defender\\SmartScreen"), QStringLiteral("ConfigureAppInstallControlEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\AppHost"), QStringLiteral("EnableWebContentEvaluation"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\AppHost"), QStringLiteral("PreventOverride"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sec.uac.disable");
        t.category = QStringLiteral("security");
        t.name = QStringLiteral("Disable user account control");
        t.description = QStringLiteral("Completely disables UAC (EnableLUA=0), suppresses all consent prompts for admins and standard users, enables admin token filter bypass, and disables secure desktop.");
        t.iconName = QStringLiteral("shield-off");
        t.impact = Impact::Critical;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("All programs run with full admin privileges"),
            QStringLiteral("No consent prompts for elevation"),
            QStringLiteral("Malware can silently gain admin access"),
            QStringLiteral("Secure desktop disabled - UI spoofing attacks possible"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"), QStringLiteral("EnableLUA"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"), QStringLiteral("ConsentPromptBehaviorAdmin"), 0u, 5u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"), QStringLiteral("ConsentPromptBehaviorUser"), 0u, 3u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"), QStringLiteral("FilterAdministratorToken"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"), QStringLiteral("LocalAccountTokenFilterPolicy"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"), QStringLiteral("EnableUIADesktopToggle"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"), QStringLiteral("ValidateAdminCodeSignatures"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"), QStringLiteral("EnableSecureUIAPaths"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"), QStringLiteral("DelayedDesktopSwitchTimeout"), 0u, 1000u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"), QStringLiteral("PromptOnSecureDesktop"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"), QStringLiteral("EnableCursorSuppression"), 0u, 1u),
        };
        c.push_back(t);
    }
}

} // namespace vth::tweaks::detail
