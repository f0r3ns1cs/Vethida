#include "TweakEngine.h"

// Storage registry tweaks (NVMe / AHCI / NTFS).
// Per-port HIPM/DIPM under Enum\PCI\...\StorPort is per-device.

namespace vth::tweaks::detail {

    namespace {
        const QString kNvme = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\stornvme\\Parameters\\Device");
        const QString kAhci = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\storahci\\Parameters\\Device");
        const QString kFs   = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem");
    }

    void appendStorage(QVector<TweakSpec>& c) {
        {
            TweakSpec t;
            t.id = QStringLiteral("storage.ahci-no-lpm");
            t.category = QStringLiteral("storage");
            t.subCategory = QStringLiteral("Link power management");
            t.name = QStringLiteral("Disable AHCI link power management");
            t.description = QStringLiteral("Disables SATA/AHCI link power management for lower I/O latency.");
            t.iconName = QStringLiteral("database");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.registryOps = { dwordOp(kAhci, QStringLiteral("NoLPM"), 1, std::nullopt) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("storage.ntfs-disable-lastaccess");
            t.category = QStringLiteral("general");
            t.subCategory = QStringLiteral("NTFS");
            t.name = QStringLiteral("Disable NTFS last-access updates");
            t.description = QStringLiteral(
                "Stops NTFS writing a last-access timestamp on every file read, cutting "
                "metadata write traffic.");
            t.iconName = QStringLiteral("file");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            // 0x80000001 is user-managed, disabled. Revert writes 0x80000002
            // (system-managed, enabled), the modern default where NTFS decides per volume.
            //   0x80000000 user-managed enabled   | 0x80000001 user-managed disabled
            //   0x80000002 system-managed enabled | 0x80000003 system-managed disabled
            t.registryOps = {
                dwordOp(kFs, QStringLiteral("NtfsDisableLastAccessUpdate"), 0x80000001u, 0x80000002u),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("storage.ntfs-disable-8dot3");
            t.category = QStringLiteral("storage");
            t.subCategory = QStringLiteral("NTFS");
            t.name = QStringLiteral("Disable 8.3 short filenames");
            t.description = QStringLiteral(
                "Stops NTFS creating legacy 8.3 short names, speeding up directories with "
                "many files.");
            t.iconName = QStringLiteral("file");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.warnings = { QStringLiteral("A few very old 16-bit apps rely on 8.3 names.") };
            t.registryOps = {
                dwordOp(kFs, QStringLiteral("NtfsDisable8dot3NameCreation"), 1, 2),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("storage.ntfs-long-paths");
            t.category = QStringLiteral("storage");
            t.subCategory = QStringLiteral("File paths");
            t.name = QStringLiteral("Enable long paths");
            t.description = QStringLiteral("Removes the 260-character path length limit for applications that opt in (via their app manifest and the Unicode file APIs). Programs that do not opt in are unaffected.");
            t.iconName = QStringLiteral("file");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.registryOps = {
                dwordOp(kFs, QStringLiteral("LongPathsEnabled"), 1, 0),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("storage.disable-autorun");
            t.category = QStringLiteral("general");
            t.subCategory = QStringLiteral("AutoPlay");
            t.name = QStringLiteral("Disable AutoRun / AutoPlay");
            t.description = QStringLiteral(
                "Disables AutoRun/AutoPlay on all drive types, including removable media.");
            t.iconName = QStringLiteral("shield");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"),
                        QStringLiteral("NoDriveTypeAutoRun"), 0xFFu, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("storage.nvme-power-mgmt-off");
            t.category = QStringLiteral("storage");
            t.name = QStringLiteral("Disable NVMe power management");
            t.description = QStringLiteral(
                "Disables idle/runtime power management and interrupt coalescing on NVMe "
                "drives for lowest storage latency.");
            t.iconName = QStringLiteral("database");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = { QStringLiteral("Higher idle drive power and temperature.") };
            t.registryOps = {
                dwordOp(kNvme, QStringLiteral("EnablePowerManagement"), 0, std::nullopt),
                dwordOp(kNvme, QStringLiteral("DisableIdlePowerManagement"), 1, std::nullopt),
                dwordOp(kNvme, QStringLiteral("DisableRuntimePowerManagement"), 1, std::nullopt),
                dwordOp(kNvme, QStringLiteral("NvmeDisableInterruptCoalescing"), 1, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("storage.ntfs-memory-usage");
            t.category = QStringLiteral("storage");
            t.name = QStringLiteral("Increase NTFS memory usage");
            t.description = QStringLiteral(
                "Tells NTFS to use more paged-pool memory for its caches.");
            t.iconName = QStringLiteral("file");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "placebo on most systems: helps only with very heavy file-server "
                "metadata workloads; no measured gaming benefit.");
            t.registryOps = {
                dwordOp(kFs, QStringLiteral("NtfsMemoryUsage"), 2, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("storage.nvme-queue-tuning");
            t.category = QStringLiteral("storage");
            t.name = QStringLiteral("NVMe queue and command depth");
            t.description = QStringLiteral(
                "Widens NVMe command/queue depths and outstanding-request limits on both the "
                "stornvme service and the StorNVMe class-driver device key, and disables the "
                "in-box driver's I/O performance throttling.");
            t.iconName = QStringLiteral("database");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "raises SSD idle power on laptops; the two key paths (Services\\stornvme\\"
                "Parameters and Control\\StorNVMe\\Parameters\\Device) overlap in purpose and "
                "are both written for compatibility across driver versions.");
            const QString svc = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\stornvme\\Parameters");
            const QString dev = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorNVMe\\Parameters\\Device");
            QVector<RegistryOp> o;
            o.push_back(dwordOp(svc, QStringLiteral("StorNVMeAllowZeroLatency"), 1, std::nullopt));
            o.push_back(dwordOp(svc, QStringLiteral("NvmeMaxReadSplit"), 4, std::nullopt));
            o.push_back(dwordOp(svc, QStringLiteral("NvmeMaxWriteSplit"), 4, std::nullopt));
            o.push_back(dwordOp(svc, QStringLiteral("ForceFlush"), 1, std::nullopt));
            o.push_back(dwordOp(svc, QStringLiteral("ImmediateData"), 1, std::nullopt));
            o.push_back(dwordOp(svc, QStringLiteral("MaxSegmentsPerCommand"), 0x00000100u, std::nullopt));
            o.push_back(dwordOp(svc, QStringLiteral("ForceEagerWrites"), 1, std::nullopt));
            o.push_back(dwordOp(svc, QStringLiteral("MaxOutstandingCmds"), 0x00000100u, std::nullopt));
            o.push_back(dwordOp(svc, QStringLiteral("MaxQueuedCommands"), 0x00000100u, std::nullopt));
            o.push_back(dwordOp(svc, QStringLiteral("MaxOutstandingIORequests"), 0x00000100u, std::nullopt));
            o.push_back(dwordOp(svc, QStringLiteral("NumberOfRequests"), 0x000005dcu, std::nullopt));
            o.push_back(dwordOp(svc, QStringLiteral("IoSubmissionQueueCount"), 3, std::nullopt));
            o.push_back(dwordOp(svc, QStringLiteral("IoQueueDepth"), 0x00000064u, std::nullopt));
            o.push_back(dwordOp(svc, QStringLiteral("HostMemoryBufferBytes"), 0x000005dcu, std::nullopt));
            o.push_back(dwordOp(svc, QStringLiteral("ArbitrationBurst"), 0x00000100u, std::nullopt));
            o.push_back(dwordOp(dev, QStringLiteral("StorNVMeAllowZeroLatency"), 1, std::nullopt));
            o.push_back(dwordOp(dev, QStringLiteral("NvmeMaxReadSplit"), 4, std::nullopt));
            o.push_back(dwordOp(dev, QStringLiteral("NvmeMaxWriteSplit"), 4, std::nullopt));
            o.push_back(dwordOp(dev, QStringLiteral("ForceFlush"), 1, std::nullopt));
            o.push_back(dwordOp(dev, QStringLiteral("ImmediateData"), 1, std::nullopt));
            o.push_back(dwordOp(dev, QStringLiteral("MaxSegmentsPerCommand"), 0x00000256u, std::nullopt));
            o.push_back(dwordOp(dev, QStringLiteral("MaxOutstandingCmds"), 0x00000256u, std::nullopt));
            o.push_back(dwordOp(dev, QStringLiteral("ForceEagerWrites"), 1, std::nullopt));
            o.push_back(dwordOp(dev, QStringLiteral("MaxQueuedCommands"), 0x00000256u, std::nullopt));
            o.push_back(dwordOp(dev, QStringLiteral("MaxOutstandingIORequests"), 0x00000256u, std::nullopt));
            o.push_back(dwordOp(dev, QStringLiteral("NumberOfRequests"), 0x00001500u, std::nullopt));
            o.push_back(dwordOp(dev, QStringLiteral("IoSubmissionQueueCount"), 3, std::nullopt));
            o.push_back(dwordOp(dev, QStringLiteral("IoQueueDepth"), 0x00000064u, std::nullopt));
            o.push_back(dwordOp(dev, QStringLiteral("HostMemoryBufferBytes"), 0x00001500u, std::nullopt));
            o.push_back(dwordOp(dev, QStringLiteral("ArbitrationBurst"), 0x00000256u, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Classpnp"),
                                QStringLiteral("NVMeDisablePerfThrottling"), 1, std::nullopt));
            t.registryOps = o;
            c.push_back(t);
        }
    }

}
