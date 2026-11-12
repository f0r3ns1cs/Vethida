#include "TweakEngine.h"

namespace vth::tweaks::detail {

void appendDefsBios(QVector<TweakSpec>& c) {
    {
        TweakSpec t;
        t.id = QStringLiteral("bios.cpu.core_parking_disable");
        t.category = QStringLiteral("power");
        t.subCategory = QStringLiteral("Core parking");
        t.declaredTier = vth::models::ProductTier::Ultimate;
        t.name = QStringLiteral("Disable CPU core parking");
        t.description = QStringLiteral("Prevents Windows from parking (idling) CPU cores, keeping all cores active and ready to handle workloads.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("CPU will draw more power and run warmer at idle because no cores are ever parked"),
            QStringLiteral("Laptops will see reduced battery life"),
            QStringLiteral("A reboot is required for the change to fully take effect or to revert cleanly"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\54533251-82be-4824-96c1-47b60b740d00\\0cc5b647-c1df-4637-891a-dec35c318583"), QStringLiteral("ACSettingIndex"), 100u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\54533251-82be-4824-96c1-47b60b740d00\\0cc5b647-c1df-4637-891a-dec35c318583"), QStringLiteral("DCSettingIndex"), 100u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\54533251-82be-4824-96c1-47b60b740d00\\ea062031-0e34-4ff1-9b6d-eb1059334028"), QStringLiteral("ACSettingIndex"), 100u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\54533251-82be-4824-96c1-47b60b740d00\\ea062031-0e34-4ff1-9b6d-eb1059334028"), QStringLiteral("DCSettingIndex"), 100u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bios.cpu.cstates_disable");
        t.category = QStringLiteral("power");
        t.subCategory = QStringLiteral("C-States");
        t.declaredTier = vth::models::ProductTier::Ultimate;
        t.name = QStringLiteral("Disable deep CPU C-states (via registry)");
        t.description = QStringLiteral("Forces the CPU to never enter idle (C-state) power states (the kernel busy-loops instead), which trims wake latency but sharply raises idle power and heat. Also disables Modern Standby (CsEnabled), so a machine that only supports Modern Standby may lose working sleep. Does not merely limit idle to C1; it disables idle entirely. Full C-state control still requires disabling C-States in BIOS.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Significantly increases idle power consumption (10-30W more at idle)"),
            QStringLiteral("May increase CPU temperature at idle"),
            QStringLiteral("For maximum effect, also disable C-States in BIOS"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\54533251-82be-4824-96c1-47b60b740d00\\5d76a2ca-e8c0-402f-a133-2158492d58ad"), QStringLiteral("ACSettingIndex"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\54533251-82be-4824-96c1-47b60b740d00\\5d76a2ca-e8c0-402f-a133-2158492d58ad"), QStringLiteral("DCSettingIndex"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("CsEnabled"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bios.pcie.aspm_disable");
        t.category = QStringLiteral("power");
        t.subCategory = QStringLiteral("Latency");
        t.declaredTier = vth::models::ProductTier::Ultimate;
        t.name = QStringLiteral("Disable PCIe ASPM (active state power management)");
        t.description = QStringLiteral("Disables PCIe Active State Power Management to prevent GPU and NVMe latency spikes from link state transitions.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("GPU, SSD, and network card will no longer enter low-power link states, raising idle power draw"),
            QStringLiteral("Laptop battery life will be noticeably shorter"),
            QStringLiteral("A reboot is required for the change to fully take effect or to revert cleanly"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PnP\\Pci"), QStringLiteral("ASPMOptOut"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\501a4d13-42af-4f44-93d8-1278f74aa2d2\\ee12f906-d277-404b-b6da-e5fa1a576df5"), QStringLiteral("ACSettingIndex"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\501a4d13-42af-4f44-93d8-1278f74aa2d2\\ee12f906-d277-404b-b6da-e5fa1a576df5"), QStringLiteral("DCSettingIndex"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bios.fast_boot");
        t.category = QStringLiteral("power");
        t.subCategory = QStringLiteral("Fast startup");
        t.declaredTier = vth::models::ProductTier::Ultimate;
        t.name = QStringLiteral("Disable UEFI fast boot (Hiberboot)");
        t.description = QStringLiteral("Disables Windows Hiberboot (fast startup) so every boot performs a full POST and clean driver initialization.");
        t.iconName = QStringLiteral("bolt-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Boot time may increase by a few seconds"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Power"), QStringLiteral("HiberbootEnabled"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bios.resize_bar");
        t.category = QStringLiteral("gpu");
        t.declaredTier = vth::models::ProductTier::Ultimate;
        t.name = QStringLiteral("Enable Resizable BAR (registry policy)");
        t.description = QStringLiteral("Enables the Windows registry policy for Resizable BAR so the GPU can map its full VRAM through PCI Express.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("No effect unless UEFI firmware also has Above 4G Decoding and ReBAR enabled"),
            QStringLiteral("Requires a compatible GPU (NVIDIA RTX 30-series+, AMD RX 6000+, Intel Arc)"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PnP\\Pci"), QStringLiteral("HackFlags"), 0u, std::nullopt),
        };
        c.push_back(t);
    }
}

}
