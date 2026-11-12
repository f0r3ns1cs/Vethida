#include "TweakEngine.h"

namespace vth::tweaks::detail {

void appendDefsSecurityPort(QVector<TweakSpec>& c) {
    {
        TweakSpec t;
        t.id = QStringLiteral("security.mitigations.disable_sehop");
        t.category = QStringLiteral("security");
        t.subCategory = QStringLiteral("Exploit Mitigations");
        t.name = QStringLiteral("Disable SEHOP");
        t.description = QStringLiteral("Disables Structured Exception Handler Overwrite Protection (SEHOP) system-wide, a mitigation that helps block a class of stack-based exploit techniques.");
        t.iconName = QStringLiteral("shield-off");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.reversible = true;
        t.warnings = { QStringLiteral("Removes a legacy exploit-mitigation layer; most modern apps are unaffected day-to-day.") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Kernel"), QStringLiteral("DisableExceptionChainValidation"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("security.mitigations.disable_elam_driver_policy");
        t.category = QStringLiteral("security");
        t.subCategory = QStringLiteral("Boot Security");
        t.name = QStringLiteral("Disable Early-Launch Anti-Malware driver policy");
        t.description = QStringLiteral("Overrides the Early Launch Antimalware (ELAM) boot-start driver classification policy, relaxing the enforcement that normally blocks known-bad or untrusted boot-start drivers from loading.");
        t.iconName = QStringLiteral("shield-off");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.reversible = true;
        t.warnings = { QStringLiteral("Weakens boot-time driver malware screening.") };
        t.concern = QStringLiteral("Lowers a boot-time anti-malware control; the applied value (0) falls outside Microsoft's four documented Group Policy values (1/3/7/8), so its precise semantics should be double-checked before shipping.");
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\EarlyLaunch"), QStringLiteral("DriverLoadPolicy"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("security.mitigations.disable_win32k_syscall_filter");
        t.category = QStringLiteral("security");
        t.subCategory = QStringLiteral("Exploit Mitigations");
        t.name = QStringLiteral("Force system-wide Win32k syscall filtering flag");
        t.description = QStringLiteral("Sets the kernel-wide DisallowWin32kSystemCalls flag under Session Manager\\Kernel. Windows normally applies this exploit mitigation per-process (for example to sandboxed renderer processes); forcing it as a system default is unusual and its effect on ordinary GUI applications is not well documented.");
        t.iconName = QStringLiteral("shield-off");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.reversible = true;
        t.warnings = { QStringLiteral("Effect on ordinary desktop applications is not well documented; verify before relying on it.") };
        t.concern = QStringLiteral("DisallowWin32kSystemCalls is normally applied per-process (e.g. sandboxed renderers), not as a kernel-wide default; forcing it system-wide could have unpredictable effects on GUI apps and needs human verification before shipping.");
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Kernel"), QStringLiteral("DisallowWin32kSystemCalls"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("security.mitigations.disable_bitlocker_dma_lock");
        t.category = QStringLiteral("security");
        t.subCategory = QStringLiteral("DMA Protection");
        t.name = QStringLiteral("Allow new DMA devices while locked");
        t.description = QStringLiteral("Disables the BitLocker policy that blocks new external DMA-capable devices from connecting while the system is locked.");
        t.iconName = QStringLiteral("shield-off");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = false;
        t.reversible = true;
        t.warnings = { QStringLiteral("Weakens protection against DMA attacks performed while the machine is locked.") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\FVE"), QStringLiteral("DisableExternalDMAUnderLock"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("security.mitigations.disable_spectre_meltdown");
        t.category = QStringLiteral("security");
        t.subCategory = QStringLiteral("CPU Mitigations");
        t.name = QStringLiteral("Disable Spectre/Meltdown mitigations");
        t.description = QStringLiteral("Disables the CPU speculative-execution side-channel mitigations (Spectre variant 2 and Meltdown) documented in Microsoft's guidance, trading security for CPU performance.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::Critical;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.reversible = true;
        t.warnings = { QStringLiteral("Re-exposes the system to Spectre/Meltdown-class speculative execution attacks.") };
        t.concern = QStringLiteral("This disables OS-level Spectre/Meltdown mitigations system-wide. Ship only with a very clear, explicit warning - this is a well-known but genuinely security-reducing performance tweak.");
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("FeatureSettingsOverride"), 3u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("FeatureSettingsOverrideMask"), 3u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("security.mitigations.disable_cfg_system_default");
        t.category = QStringLiteral("security");
        t.subCategory = QStringLiteral("Exploit Mitigations");
        t.name = QStringLiteral("Disable control flow guard system default");
        t.description = QStringLiteral("Turns off the system-wide default for Control Flow Guard (CFG), a mitigation that restricts indirect code execution to legitimate call targets.");
        t.iconName = QStringLiteral("shield-off");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.reversible = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("EnableCfg"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("security.mitigations.disable_vbs");
        t.category = QStringLiteral("security");
        t.subCategory = QStringLiteral("Virtualization-Based Security");
        t.name = QStringLiteral("Disable virtualization-based security policy");
        t.description = QStringLiteral("Forces the Virtualization-Based Security (VBS) policy override off. VBS underpins Credential Guard, Memory Integrity (HVCI), and other hypervisor-enforced protections.");
        t.iconName = QStringLiteral("shield-off");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.reversible = true;
        t.warnings = { QStringLiteral("Disables Credential Guard / Memory Integrity if they depend on this policy."), QStringLiteral("Some anti-cheat and virtualization software behaves differently with VBS off.") };
        t.concern = QStringLiteral("VBS is increasingly enabled by default on Secured-core and newer Windows 11 PCs. Confirm on target hardware that VBS state is not managed through a different mechanism before shipping this as a routine tweak.");
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\DeviceGuard"), QStringLiteral("EnableVirtualizationBasedSecurity"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("security.mitigations.disable_hvci_memory_attestation_requirement");
        t.category = QStringLiteral("security");
        t.subCategory = QStringLiteral("Virtualization-Based Security");
        t.name = QStringLiteral("Disable HVCI UEFI memory attestation requirement");
        t.description = QStringLiteral("Turns off the policy requiring UEFI memory attestation before Hypervisor-Protected Code Integrity (HVCI) is allowed to run. Only meaningful on systems that have HVCI configured with strict attestation.");
        t.iconName = QStringLiteral("shield-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.reversible = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DeviceGuard"), QStringLiteral("HVCIMATRequired"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("security.mitigations.disable_kernel_dma_protection_policy");
        t.category = QStringLiteral("security");
        t.subCategory = QStringLiteral("DMA Protection");
        t.name = QStringLiteral("Disable Kernel DMA Protection policy override");
        t.description = QStringLiteral("Turns off the Group Policy-style override that enables Kernel DMA Protection, which blocks unauthorized DMA access from Thunderbolt and other hot-pluggable PCIe devices.");
        t.iconName = QStringLiteral("shield-off");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.reversible = true;
        t.warnings = { QStringLiteral("Reduces protection against DMA (Thunderbolt/PCIe) attack devices.") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\DeviceGuard\\Scenarios\\DmaGuard"), QStringLiteral("Enable"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("kernel.cpu.disable_intel_tsx");
        t.category = QStringLiteral("security");
        t.subCategory = QStringLiteral("CPU Mitigations");
        t.name = QStringLiteral("Disable Intel TSX");
        t.description = QStringLiteral("Disables Intel TSX (Transactional Synchronization Extensions) at the kernel level, mitigating the TSX Asynchronous Abort (TAA) speculative-execution side channel (CVE-2019-11135) at the cost of TSX-dependent software falling back to non-transactional code paths.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = { QStringLiteral("Software that relies on Intel TSX for performance (some database/HPC workloads) will run slower with TSX disabled") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("DisableTsx"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("kernel.security.disable_control_flow_guard");
        t.category = QStringLiteral("security");
        t.subCategory = QStringLiteral("Exploit Mitigations");
        t.name = QStringLiteral("Disable control flow guard enforcement");
        t.description = QStringLiteral("Writes a system-wide override that disables Control Flow Guard (CFG) enforcement and CFG export suppression at the kernel level.");
        t.iconName = QStringLiteral("shield-off");
        t.impact = Impact::Critical;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("This is a genuine, significant security regression: Control Flow Guard is a system-wide exploit mitigation that helps block control-flow-hijacking attacks, and there is no established performance benefit to disabling it. Ship only with a prominent warning, if at all - this is not a routine perf tweak.");
        t.warnings = { QStringLiteral("Reduces exploit-mitigation coverage system-wide; not recommended outside of driver-development or compatibility debugging") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("DisableControlFlowGuardExportSuppression"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("DisableControlFlowGuardCfg"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("security.svc.system_guard_agent_disable");
        t.category = QStringLiteral("security");
        t.subCategory = QStringLiteral("Attestation");
        t.name = QStringLiteral("Disable system guard runtime monitor broker");
        t.description = QStringLiteral("Disables SgrmAgent, the Windows Defender System Guard Runtime Monitor Broker that periodically attests platform/runtime integrity.");
        t.iconName = QStringLiteral("shield-check");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("Part of the Windows Defender System Guard security stack; disabling it turns off runtime integrity attestation.");
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\SgrmAgent"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("security.svc.msa_signin_assistant_disable");
        t.category = QStringLiteral("security");
        t.subCategory = QStringLiteral("Accounts");
        t.name = QStringLiteral("Disable Microsoft Account sign-in assistant");
        t.description = QStringLiteral("Disables the Microsoft Account Sign-in Assistant service (wlidsvc), which handles authentication for apps and Windows features tied to a Microsoft Account.");
        t.iconName = QStringLiteral("user-off");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("If you sign in to Windows with a Microsoft Account, disabling this service can prevent sign-in and lock you out of your session - only apply on machines using a local account.");
        t.warnings = { QStringLiteral("Apps and Windows features that authenticate via a Microsoft Account (Store, OneDrive, Mail, Xbox) may stop working.") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\wlidsvc"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("security.session_manager.disable_object_manager_protection");
        t.category = QStringLiteral("security");
        t.subCategory = QStringLiteral("Kernel");
        t.name = QStringLiteral("Disable object manager namespace protection");
        t.description = QStringLiteral("Turns off the Session Manager's stricter Object Manager namespace protection, which normally restricts non-admin processes from creating objects that squat on names other applications expect to find in the shared \\BaseNamedObjects directory.");
        t.iconName = QStringLiteral("shield-off");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.reversible = true;
        t.concern = QStringLiteral("Weakens a kernel-level security mitigation (object-namespace squatting protection) present by default since Windows Vista SP1; genuinely risky, should not be recommended by default.");
        t.warnings = {
            QStringLiteral("Reduces protection against local object-namespace squatting attacks"),
            QStringLiteral("Requires a restart to take effect"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager"), QStringLiteral("ProtectionMode"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sec.mitigation.disable_speculative_execution");
        t.category = QStringLiteral("security");
        t.subCategory = QStringLiteral("CPU mitigations");
        t.name = QStringLiteral("Disable speculative execution mitigations");
        t.description = QStringLiteral("Disables the Meltdown/Spectre CPU speculative-execution side-channel mitigations via the Memory Management FeatureSettings override, recovering the CPU performance those mitigations cost.");
        t.iconName = QStringLiteral("shield-off");
        t.impact = Impact::Critical;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Leaves the CPU exposed to Meltdown/Spectre-class speculative-execution side-channel attacks"),
            QStringLiteral("A reboot is required in both directions for the mitigation state to actually change"),
        };
        t.concern = QStringLiteral("Turns off OS-level Meltdown/Spectre protection outright; only offer this to users who understand and accept the security tradeoff.");
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("FeatureSettingsOverride"), 3u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("FeatureSettingsOverrideMask"), 3u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("FeatureSettings"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("security.uac.disable");
        t.category = QStringLiteral("security");
        t.subCategory = QStringLiteral("UAC");
        t.name = QStringLiteral("Disable user account control (UAC)");
        t.description = QStringLiteral("Turns off User Account Control entirely (EnableLUA=0) so no application - including malware - is prompted for elevation before making system-wide changes.");
        t.iconName = QStringLiteral("shield-off");
        t.impact = Impact::Critical;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.reversible = true;
        t.warnings = {
            QStringLiteral("All applications run with full administrator rights with no elevation prompt, including malware"),
            QStringLiteral("A system restart is required for the change to take effect"),
        };
        t.concern = QStringLiteral("Disabling UAC entirely removes Windows' primary privilege-elevation safeguard; Microsoft strongly advises against this. Only ship with the user's explicit, informed consent.");
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"), QStringLiteral("EnableLUA"), 0u, 1u),
        };
        c.push_back(t);
    }
}

}
