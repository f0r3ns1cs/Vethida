#include "TweakEngine.h"

// Session Manager\Memory Management.
//
// Pool values that are set-to-default no-ops (NonPagedPoolSize=0, SystemPages=0,
// PagedPoolQuota=0, NonPagedPoolQuota=0, LargePageMinimum=0,
// SecondLevelDataCache=0, MapAllocationGranularity=65536, AllocationPreference=0,
// SystemCacheDirtyPageThreshold=0, ClearPageFileAtShutdown=0) are intentionally
// not shipped: writing a value equal to its default changes nothing. PoolUsageMaximum
// is omitted for the same reason. Pagefile sizing and RAM-compression toggles are
// WMI/PowerShell, not registry - they need the command-op engine extension.

namespace vth::tweaks::detail {

    namespace {
        const QString kMM = QStringLiteral(
            "HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management");
        const QString kPF = QStringLiteral(
            "HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"
            "\\PrefetchParameters");
    }

    void appendMemory(QVector<TweakSpec>& c) {
        {
            TweakSpec t;
            t.id = QStringLiteral("mem.disable-paging-executive");
            t.category = QStringLiteral("advanced");
            t.subCategory = QStringLiteral("Paging");
            t.name = QStringLiteral("Keep kernel in RAM");
            t.description = QStringLiteral("Forces the Windows kernel and drivers to stay resident in RAM instead of being paged to disk. The change is real, but the practical benefit is negligible on modern systems with an SSD and ample RAM.");
            t.iconName = QStringLiteral("cpu");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = { QStringLiteral("Holds ~20-50 MB more RAM resident.") };
            t.registryOps = { dwordOp(kMM, QStringLiteral("DisablePagingExecutive"), 1, 0) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("mem.disable-page-combining");
            t.category = QStringLiteral("advanced");
            t.subCategory = QStringLiteral("Page combining");
            t.name = QStringLiteral("Disable memory page combining");
            t.description = QStringLiteral(
                "Stops the kernel thread that periodically scans for identical memory "
                "pages to merge, saving CPU on high-RAM systems.");
            t.iconName = QStringLiteral("layers");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = { QStringLiteral("Minor RAM increase; worth it on 16 GB+ systems.") };
            t.registryOps = { dwordOp(kMM, QStringLiteral("DisablePageCombining"), 1, std::nullopt) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("mem.disable-superfetch");
            t.category = QStringLiteral("advanced");
            t.subCategory = QStringLiteral("Prefetch & SysMain");
            t.name = QStringLiteral("Disable Superfetch");
            t.description = QStringLiteral(
                "Stops Windows loading the programs you open most into spare memory "
                "ahead of time. Those programs open a little slower; the memory and "
                "the background disk activity come back to you.");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "SSD: little measurable benefit. Pairs with disabling the SysMain "
                "service; on HDDs it can hurt launch times.");
            t.registryOps = { dwordOp(kPF, QStringLiteral("EnableSuperfetch"), 0, 3) };
            c.push_back(t);
        }


        {
            TweakSpec t;
            t.id = QStringLiteral("mem.large-system-cache");
            t.category = QStringLiteral("advanced");
            t.name = QStringLiteral("Large System Cache");
            t.description = QStringLiteral(
                "Switches the file-system cache to the server-style policy that favors "
                "cached file data over working-set memory.");
            t.iconName = QStringLiteral("database");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "placebo/workload: the gaming default is 0; 1 is a server policy that can "
                "hurt desktop responsiveness. No measured gaming benefit.");
            t.registryOps = { dwordOp(kMM, QStringLiteral("LargeSystemCache"), 1, 0) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("mem.io-page-lock-limit");
            t.category = QStringLiteral("advanced");
            t.name = QStringLiteral("Raise I/O page lock limit");
            t.description = QStringLiteral(
                "Sets the maximum bytes that may be locked for a single I/O operation "
                "to 960 KB instead of the automatic default.");
            t.iconName = QStringLiteral("database");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "placebo: an undocumented value with no published measurement; the auto "
                "default is fine on modern systems.");
            t.registryOps = { dwordOp(kMM, QStringLiteral("IoPageLockLimit"), 983040, std::nullopt) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("mem.disable-prefetcher");
            t.category = QStringLiteral("general");
            t.name = QStringLiteral("Disable Prefetcher");
            t.description = QStringLiteral(
                "Turns off the application and boot prefetcher.");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "outdated on SSDs: prefetch helps HDDs, not SSDs, and modern Windows "
                "largely no-ops it on SSDs already. Do not apply on HDD systems.");
            t.registryOps = { dwordOp(kPF, QStringLiteral("EnablePrefetcher"), 0, 3) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("mem.disable-cfg");
            t.category = QStringLiteral("security");
            t.name = QStringLiteral("Disable kernel control flow guard");
            t.description = QStringLiteral(
                "Disables kernel Control Flow Guard, removing its per-call overhead.");
            t.iconName = QStringLiteral("shield-off");
            t.impact = Impact::High;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = { QStringLiteral("Security trade-off: removes a control-flow exploit mitigation.") };
            t.concern = QStringLiteral(
                "risky: real security trade-off (defeats some ROP-style exploits) for a "
                "small, workload-dependent gain.");
            t.registryOps = { dwordOp(kMM, QStringLiteral("EnableCfg"), 0, 1) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("mem.disable-aslr-images");
            t.category = QStringLiteral("security");
            t.name = QStringLiteral("Disable image ASLR");
            t.description = QStringLiteral(
                "Disables per-load address-space randomization for images.");
            t.iconName = QStringLiteral("shield-off");
            t.impact = Impact::High;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = { QStringLiteral("Security trade-off: weakens memory-corruption exploit defense.") };
            t.concern = QStringLiteral(
                "risky: significant security trade-off for a marginal, unmeasured gain.");
            t.registryOps = { dwordOp(kMM, QStringLiteral("MoveImages"), 0, std::nullopt) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("mem.disable-cpu-mitigations");
            t.category = QStringLiteral("security");
            t.name = QStringLiteral("Disable CPU speculation mitigations");
            t.description = QStringLiteral(
                "Disables the Spectre/Meltdown speculative-execution mitigations, "
                "which cost the most in syscall-heavy work.");
            t.iconName = QStringLiteral("shield-off");
            t.impact = Impact::Critical;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = { QStringLiteral("Major security trade-off: exposes speculative-execution side channels.") };
            t.concern = QStringLiteral(
                "risky: only reasonable on a dedicated machine that runs no untrusted "
                "code; do not apply on a daily driver.");
            t.registryOps = {
                dwordOp(kMM, QStringLiteral("FeatureSettings"), 1, std::nullopt),
                dwordOp(kMM, QStringLiteral("FeatureSettingsOverride"), 3, std::nullopt),
                dwordOp(kMM, QStringLiteral("FeatureSettingsOverrideMask"), 3, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("mem.manager-extra-tuning");
            t.category = QStringLiteral("advanced");
            t.name = QStringLiteral("Extra memory manager values");
            t.description = QStringLiteral(
                "Disables page combining and clear-page-at-shutdown, zeroes several pool-size "
                "quota overrides, enables PAE, and turns off boot-time prefetch tracing.");
            t.iconName = QStringLiteral("layers");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "several values here (SecondLevelDataCache, SessionPoolSize, SessionViewSize) "
                "are undocumented tuning knobs with no published measurement; PhysicalAddressExtension "
                "is a no-op on any 64-bit install.");
            t.registryOps = {
                dwordOp(kMM, QStringLiteral("DisablePagingCombining"), 1, std::nullopt),
                dwordOp(kMM, QStringLiteral("ClearPageFileAtShutdown"), 0, std::nullopt),
                dwordOp(kMM, QStringLiteral("NonPagedPoolQuota"), 0, std::nullopt),
                dwordOp(kMM, QStringLiteral("NonPagedPoolSize"), 0, std::nullopt),
                dwordOp(kMM, QStringLiteral("PagedPoolQuota"), 0, std::nullopt),
                dwordOp(kMM, QStringLiteral("PagedPoolSize"), 0, std::nullopt),
                dwordOp(kMM, QStringLiteral("PhysicalAddressExtension"), 1, std::nullopt),
                dwordOp(kMM, QStringLiteral("SecondLevelDataCache"), 0x00000100u, std::nullopt),
                dwordOp(kMM, QStringLiteral("SessionPoolSize"), 0x00000020u, std::nullopt),
                dwordOp(kMM, QStringLiteral("SessionViewSize"), 0x00000020u, std::nullopt),
                dwordOp(kMM, QStringLiteral("SystemPages"), 0, std::nullopt),
                dwordOp(kPF, QStringLiteral("EnableBoottrace"), 0, std::nullopt),
                dwordOp(kPF, QStringLiteral("SfTracingState"), 1, std::nullopt),
                stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Dfrg\\BootOptimizeFunction"),
                         QStringLiteral("Enable"), QStringLiteral("N"), std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\OptimalLayout"),
                        QStringLiteral("EnableAutoLayout"), 0, std::nullopt),
            };
            c.push_back(t);
        }
    }

} // namespace vth::tweaks::detail
