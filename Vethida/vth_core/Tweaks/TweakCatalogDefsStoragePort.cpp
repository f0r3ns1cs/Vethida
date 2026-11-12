#include "TweakEngine.h"

namespace vth::tweaks::detail {

void appendDefsStoragePort(QVector<TweakSpec>& c) {
    {
        TweakSpec t;
        t.id = QStringLiteral("storage.nvme.stornvme_control_device_parameters");
        t.category = QStringLiteral("storage");
        t.subCategory = QStringLiteral("NVMe");
        t.name = QStringLiteral("NVMe storage class device parameters");
        t.description = QStringLiteral("Writes a second, larger-valued set of the same-named NVMe parameters under Control\\StorNVMe\\Parameters\\Device.");
        t.iconName = QStringLiteral("database");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("Nearly identical to the stornvme service Parameters tweak but under a different registry subtree (Control instead of Services); it is unclear whether this second path is actually consumed by the storage stack or is a redundant duplicate carried over from the source tool.");
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorNVMe\\Parameters\\Device"), QStringLiteral("StorNVMeAllowZeroLatency"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorNVMe\\Parameters\\Device"), QStringLiteral("QueueDepth"), 0x00000064u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorNVMe\\Parameters\\Device"), QStringLiteral("NvmeMaxReadSplit"), 4u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorNVMe\\Parameters\\Device"), QStringLiteral("NvmeMaxWriteSplit"), 4u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorNVMe\\Parameters\\Device"), QStringLiteral("ForceFlush"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorNVMe\\Parameters\\Device"), QStringLiteral("ImmediateData"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorNVMe\\Parameters\\Device"), QStringLiteral("MaxSegmentsPerCommand"), 0x00000256u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorNVMe\\Parameters\\Device"), QStringLiteral("MaxOutstandingCmds"), 0x00000256u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorNVMe\\Parameters\\Device"), QStringLiteral("ForceEagerWrites"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorNVMe\\Parameters\\Device"), QStringLiteral("MaxQueuedCommands"), 0x00000256u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorNVMe\\Parameters\\Device"), QStringLiteral("MaxOutstandingIORequests"), 0x00000256u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorNVMe\\Parameters\\Device"), QStringLiteral("NumberOfRequests"), 0x00001500u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorNVMe\\Parameters\\Device"), QStringLiteral("IoSubmissionQueueCount"), 3u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorNVMe\\Parameters\\Device"), QStringLiteral("IoQueueDepth"), 0x00000064u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorNVMe\\Parameters\\Device"), QStringLiteral("HostMemoryBufferBytes"), 0x00001500u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorNVMe\\Parameters\\Device"), QStringLiteral("ArbitrationBurst"), 0x00000256u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("storage.nvme.stornvme_service_parameters");
        t.category = QStringLiteral("storage");
        t.subCategory = QStringLiteral("NVMe");
        t.name = QStringLiteral("NVMe driver service parameters (stornvme)");
        t.description = QStringLiteral("Writes an extended set of queue-depth, read/write-split, flush, and outstanding-command limits under the stornvme service's own Parameters key.");
        t.iconName = QStringLiteral("database");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("Aside from IoQueueDepth, these StorNVMe parameter names are not documented by Microsoft; they are sourced from community SSD-tuning guides with no official confirmation the miniport driver reads them from this exact path.");
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\stornvme\\Parameters"), QStringLiteral("StorNVMeAllowZeroLatency"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\stornvme\\Parameters"), QStringLiteral("QueueDepth"), 0x00000040u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\stornvme\\Parameters"), QStringLiteral("NvmeMaxReadSplit"), 4u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\stornvme\\Parameters"), QStringLiteral("NvmeMaxWriteSplit"), 4u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\stornvme\\Parameters"), QStringLiteral("ForceFlush"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\stornvme\\Parameters"), QStringLiteral("ImmediateData"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\stornvme\\Parameters"), QStringLiteral("MaxSegmentsPerCommand"), 0x00000100u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\stornvme\\Parameters"), QStringLiteral("ForceEagerWrites"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\stornvme\\Parameters"), QStringLiteral("MaxOutstandingCmds"), 0x00000100u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\stornvme\\Parameters"), QStringLiteral("MaxQueuedCommands"), 0x00000100u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\stornvme\\Parameters"), QStringLiteral("MaxOutstandingIORequests"), 0x00000100u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\stornvme\\Parameters"), QStringLiteral("NumberOfRequests"), 0x000005dcu, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\stornvme\\Parameters"), QStringLiteral("IoSubmissionQueueCount"), 3u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\stornvme\\Parameters"), QStringLiteral("IoQueueDepth"), 0x00000064u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\stornvme\\Parameters"), QStringLiteral("HostMemoryBufferBytes"), 0x000005dcu, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\stornvme\\Parameters"), QStringLiteral("ArbitrationBurst"), 0x00000100u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("storage.nvme.classpnp_disable_perf_throttling");
        t.category = QStringLiteral("storage");
        t.subCategory = QStringLiteral("NVMe");
        t.name = QStringLiteral("Disable NVMe performance throttling (Classpnp)");
        t.description = QStringLiteral("Sets NVMeDisablePerfThrottling=1 under the storage class driver's Classpnp key, disabling thermal/power-based performance throttling for NVMe drives.");
        t.iconName = QStringLiteral("thermometer");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("Not officially documented by Microsoft, but widely cited in community SSD-tuning references as a real classpnp.sys parameter; disabling throttling can raise sustained drive temperatures.");
        t.warnings = { QStringLiteral("May increase sustained NVMe drive temperature under heavy load since built-in throttling is disabled") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Classpnp"), QStringLiteral("NVMeDisablePerfThrottling"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("storage.svc.backup_bitlocker_disable");
        t.category = QStringLiteral("storage");
        t.subCategory = QStringLiteral("Backup & encryption services");
        t.name = QStringLiteral("Disable storage tiering, BitLocker, backup and File History services");
        t.description = QStringLiteral("Disables Storage Tiers Management, the BitLocker Drive Encryption Service (BDESVC), the Block Level Backup Engine (wbengine, used by Windows Backup), and the File History service.");
        t.iconName = QStringLiteral("archive-off");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("Disabling BDESVC can prevent BitLocker unlock/recovery-key prompts and management UI from working on encrypted drives; disabling wbengine breaks Windows Backup and any in-progress recovery. Only apply if you do not use BitLocker or Windows Backup.");
        t.warnings = {
            QStringLiteral("BitLocker management and recovery-key prompts may stop functioning."),
            QStringLiteral("Windows Backup (wbengine) and File History backups/restores will fail."),
            QStringLiteral("Storage Spaces tiering (moving hot/cold data between tiers) stops running."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\TieringEngineService"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\BDESVC"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\wbengine"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\fhsvc"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("storage.defrag.disable_boot_optimize");
        t.category = QStringLiteral("storage");
        t.subCategory = QStringLiteral("Defrag");
        t.name = QStringLiteral("Disable boot-time defrag optimization");
        t.description = QStringLiteral("Disables the automatic boot-time disk defragmentation/prefetch-layout optimization pass Windows periodically schedules.");
        t.iconName = QStringLiteral("database");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.registryOps = {
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Dfrg\\BootOptimizeFunction"), QStringLiteral("Enable"), QStringLiteral("N"), QStringLiteral("Y")),
        };
        c.push_back(t);
    }
}

}
