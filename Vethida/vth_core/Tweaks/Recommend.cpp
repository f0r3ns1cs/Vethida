#include "Recommend.h"

#include <QStringLiteral>
#include <QObject>

namespace vth::tweaks::recommend
{
    namespace
    {
        namespace machine = vth::win::machine;

        bool mentions(const TweakSpec& spec, const char* needle)
        {
            const QLatin1String n(needle);
            return spec.id.contains(n, Qt::CaseInsensitive)
                || spec.subCategory.contains(n, Qt::CaseInsensitive);
        }

        bool writesToSecurityTarget(const TweakSpec& spec)
        {
            static const char* kTargets[] = {
                "WinDefend", "WdNisSvc", "WdFilter", "WdBoot", "MpsSvc",
                "DisableAntiSpyware", "DisableRealtimeMonitoring",
                "TamperProtection", "SmartScreen", "EnableLUA",
                "ConsentPromptBehavior", "EnableFirewall", "BitLocker",
                "HvciOptions", "LsaCfgFlags", "SecureBoot",
                "wuauserv", "UsoSvc", "WaaSMedic",
            };
            const auto hit = [](const QString& hay) {
                for (const char* t : kTargets)
                    if (hay.contains(QLatin1String(t), Qt::CaseInsensitive))
                        return true;
                return false;
            };

            for (const RegistryOp& op : spec.registryOps)
                if (hit(op.path) || hit(op.name))
                    return true;
            for (const CommandOp& op : spec.commandOps) {
                for (const QString& a : op.applyArgs)
                    if (hit(a))
                        return true;
                for (const QString& a : op.revertArgs)
                    if (hit(a))
                        return true;
            }
            return false;
        }

        bool touchesSecurity(const TweakSpec& spec)
        {
            static const char* kMarkers[] = {
                "defender", "smartscreen", "uac", "firewall", "bitlocker",
                "tamper", "mitigation", "aslr", "dep", "cfg", "hvci",
                "memory_integrity", "memoryintegrity", "vbs", "credential_guard",
                "credentialguard", "lsa", "secureboot", "secure_boot", "antivirus",
                "spectre", "meltdown", "windowsupdate", "windows_update", "update",
            };
            for (const char* m : kMarkers)
                if (mentions(spec, m))
                    return true;
            if (writesToSecurityTarget(spec))
                return true;
            return spec.category.compare(QStringLiteral("security"),
                                         Qt::CaseInsensitive) == 0;
        }
    }

    Skip hardwareFit(const TweakSpec& spec, const machine::Profile& profile)
    {
        if (mentions(spec, "nvidia") && !profile.hasNvidia())
            return Skip::NotForThisHardware;
        if (mentions(spec, "radeon") && !profile.hasAmdGpu())
            return Skip::NotForThisHardware;

        // Both make CPUs and GPUs, so these need a subsystem marker too.
        const bool gpuScoped = mentions(spec, "gpu") || mentions(spec, "graphics")
                            || mentions(spec, "display") || mentions(spec, "dxgi");
        const bool cpuScoped = mentions(spec, "cpu") || mentions(spec, "processor")
                            || mentions(spec, "core") || mentions(spec, "sched");

        if (mentions(spec, "amd")) {
            if (gpuScoped && !profile.hasAmdGpu())
                return Skip::NotForThisHardware;
            if (cpuScoped && !profile.hasAmdCpu())
                return Skip::NotForThisHardware;
        }
        if (mentions(spec, "intel")) {
            if (gpuScoped && !profile.hasIntelGpu())
                return Skip::NotForThisHardware;
            if (cpuScoped && !profile.hasIntelCpu())
                return Skip::NotForThisHardware;
        }

        // NVMe tweaks need NVMe, the Prefetch/SysMain/TRIM ones need an SSD
        if (mentions(spec, "nvme") && !profile.hasNvme())
            return Skip::NotForThisHardware;
        if ((mentions(spec, "defrag") || mentions(spec, "prefetch")
             || mentions(spec, "superfetch") || mentions(spec, "sysmain"))
            && !profile.hasFlashStorage())
            return Skip::NotForThisHardware;
        if (mentions(spec, "trim") && !profile.hasFlashStorage())
            return Skip::NotForThisHardware;

        if ((mentions(spec, "hybrid") || mentions(spec, "ecore")
             || mentions(spec, "e_core") || mentions(spec, "pcore"))
            && !profile.cpuHybrid)
            return Skip::NotForThisHardware;

        // Only recommended on confirmed desktops, because of battery life.
        if (!profile.isDesktop()
            && (mentions(spec, "ultimate-performance") || mentions(spec, "max-performance")
                || mentions(spec, "max_performance") || mentions(spec, "power-throttling")
                || mentions(spec, "connected-standby")))
            return Skip::CouldCauseHarm;

        // These values depend on the amount of RAM, and aren't computed per machine.
        if (mentions(spec, "splitthreshold") || mentions(spec, "split_threshold")
            || mentions(spec, "pagingfile") || mentions(spec, "paging_file")
            || mentions(spec, "largesystemcache"))
            return Skip::CouldCauseHarm;

        // Skip memory-for-speed tweaks below 16 GB, or if the amount is unknown.
        if ((profile.ramTotalBytes == 0 || profile.ramGigabytes() < 16)
            && (mentions(spec, "cache") || mentions(spec, "standby")
                || mentions(spec, "ram-compression") || mentions(spec, "compression")))
            return Skip::CouldCauseHarm;

        return Skip::None;
    }

    bool categoryMatches(const QString& specCategory, const QString& key)
    {
        for (const QStringView part : QStringView(key).split(QLatin1Char('+')))
            if (specCategory.compare(part, Qt::CaseInsensitive) == 0)
                return true;
        return false;
    }

    Result forCategory(const QString& category,
                       const machine::Profile& profile,
                       const QHash<QString, int>& states,
                       vth::models::ProductTier tier,
                       bool earlyAccessOptIn)
    {
        Result result;

        const int userRank = vth::models::tierRank(tier);

        for (const TweakSpec& spec : catalog()) {
            if (!categoryMatches(spec.category, category))
                continue;

            Candidate c;
            c.spec = &spec;

            // Hardware first, so a tweak for other hardware isn't labeled risky.
            c.skip = hardwareFit(spec, profile);

            if (c.skip == Skip::None && spec.earlyAccess
                && !(earlyAccessOptIn && userRank >= vth::models::tierRank(
                         vth::models::ProductTier::Ultimate)))
                c.skip = Skip::EarlyAccess;

            if (c.skip == Skip::None && vth::models::tierRank(spec.minTier) > userRank)
                c.skip = Skip::AboveTier;

            if (c.skip == Skip::None && !spec.reversible)
                c.skip = Skip::NotReversible;

            // Anything that could cause harm has to be chosen by hand.
            if (c.skip == Skip::None
                && (!spec.warnings.isEmpty() || !spec.effects.isEmpty()
                    || spec.impact == Impact::High
                    || spec.impact == Impact::Critical
                    || touchesSecurity(spec)
                    || raisesThermalOrPowerLoad(spec)))
                c.skip = Skip::CouldCauseHarm;

            if (c.skip == Skip::None
                && states.value(spec.id, static_cast<int>(DetectState::Unknown))
                       == static_cast<int>(DetectState::On))
                c.skip = Skip::AlreadyApplied;

            if (c.recommended())
                result.toApply.push_back(&spec);
            result.candidates.push_back(c);
        }

        return result;
    }
}

namespace vth::tweaks::recommend
{
    QString skipLabel(Skip skip)
    {
        switch (skip) {
        case Skip::None:               return QObject::tr("Recommended");
        case Skip::NotForThisHardware: return QObject::tr("Not for this hardware");
        case Skip::CouldCauseHarm:     return QObject::tr("Has trade-offs");
        case Skip::NotReversible:      return QObject::tr("Cannot be undone");
        case Skip::AboveTier:          return QObject::tr("Not on your plan");
        case Skip::EarlyAccess:        return QObject::tr("Early access");
        case Skip::AlreadyApplied:     return QObject::tr("Already on");
        }
        return {};
    }
}

