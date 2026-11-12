#pragma once

#include "../Services/IRegistryEditor.h"
#include "../Models/ProductTier.h"

#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtCore/QVector>

#include <functional>
#include <optional>

// A catalog of tweaks, each a list of typed operations that can be applied,
// reverted and detected.
namespace vth::tweaks {

    enum class Impact { Low, Medium, High, Critical };

    enum class DetectState {
        On,         // every op matches its applied value
        Off,        // not fully applied: reverted, absent, or a mix
        Unknown,    // could not read
    };

    // On revert the value is set to revertValue, or deleted when it is nullopt.
    struct RegistryOp {
        QString                        path;        // HIVE\Subkey form, e.g. HKLM\SYSTEM\...
        QString                        name;        // value name
        vth::services::RegistryValueKind kind = vth::services::RegistryValueKind::DWord;
        QVariant                       appliedValue;
        std::optional<QVariant>        revertValue;
    };

    // An external command such as schtasks. Command-only tweaks detect as Unknown.
    struct CommandOp {
        QString     program;
        QStringList applyArgs;
        QStringList revertArgs;
    };

    // An operation resolved at runtime, such as one per network adapter. detect
    // is optional; apply and revert return success.
    struct DynamicOp {
        QString                         label;
        std::function<DetectState()>    detect;
        std::function<bool()>           apply;
        std::function<bool()>           revert;
    };

    struct TweakSpec {
        QString     id;
        QString     category;      // GUI category key: general, debloating, networking, advanced, bios, benchmarking, ...
        QString     subCategory;   // optional finer grouping within a category (e.g. "NVIDIA", "C-States"). When empty the GUI derives one from the id prefix.
        // Overrides the tier classifyEntitlement would score for this tweak.
        std::optional<vth::models::ProductTier> declaredTier;

        // Resolved by classifyEntitlement, so builders set declaredTier instead.
        vth::models::ProductTier minTier = vth::models::ProductTier::Pro;

        // Preview tweak, only for Ultimate users who have opted in.
        bool        earlyAccess = false;

        QString     name;
        QString     description;
        QString     iconName;
        Impact      impact         = Impact::Low;
        bool        requiresAdmin  = false;
        bool        requiresReboot = false;
        bool        reversible     = true;
        QStringList warnings;   // shown on the row and in the details
        QStringList effects;    // side effects, shown only in the details
        bool        askFirst = false;   // confirm before applying

        QString     concern;    // maintainer note, hidden from users

        // Ids of tweaks that write the same value differently. With both applied,
        // revert would restore the wrong state. Set on both specs.
        QStringList excludes;

        QVector<RegistryOp> registryOps;
        QVector<CommandOp>  commandOps;
        QVector<DynamicOp>  dynamicOps;
        QString     source;
    };

    struct OpFailure {
        enum class Kind { Registry, Command, Dynamic, Refused };
        Kind    kind = Kind::Registry;
        QString target;   // "HKLM\...\Name", the command program, or the dynamic-op label
        // Registry: the Win32 status of the write. Command: the exit code, or
        // -1 when the program would not start and -2 when it timed out.
        long    code = 0;
        QString output;   // end of the command's output, for the log
    };

    struct ApplyOutcome {
        bool               ok = true;
        QVector<OpFailure> failures;
        // Revert was refused and nothing was attempted, so don't offer a retry.
        bool               irreversible = false;
    };

    const QVector<TweakSpec>& catalog();
    const TweakSpec*          find(const QString& id);

    DetectState detect(const TweakSpec& spec, const vth::services::IRegistryEditor& reg);

    // No registry op and no dynamic op with detect(), so detect() is always Unknown.
    bool isStateUndetectable(const TweakSpec& spec);

    // The *Verbose forms also report which operations failed.
    bool        apply (const TweakSpec& spec, vth::services::IRegistryEditor& reg);
    bool        revert(const TweakSpec& spec, vth::services::IRegistryEditor& reg);
    ApplyOutcome applyVerbose (const TweakSpec& spec, vth::services::IRegistryEditor& reg);
    ApplyOutcome revertVerbose(const TweakSpec& spec, vth::services::IRegistryEditor& reg);

    QString impactLabel(Impact i);

    // Overclocking, pinned processor states, lifted power limits and the like.
    bool raisesThermalOrPowerLoad(const TweakSpec& spec);

    // "nvidia", "amd", or empty for a tweak that applies to every machine.
    QString hardwareVendorKey(const TweakSpec& spec);

    bool needsUserConfirmation(const TweakSpec& spec);

    inline bool isOneWay(const TweakSpec& spec) { return !spec.reversible; }

    // Remote kill switch: the update feed can name tweaks that must not be applied.
    void    setRemotelyDisabled(const QHash<QString, QString>& idToReason);
    bool    isRemotelyDisabled(const QString& id);
    QString remoteDisableReason(const QString& id);

    enum class TweakLock {
        Available,
        LockedTier,         // needs a higher tier (see spec.minTier)
        LockedEarlyAccess,  // needs Ultimate and the opt-in
        DisabledRemotely,   // see remoteDisableReason()
    };

    inline TweakLock lockStateFor(const TweakSpec& spec,
                                  vth::models::ProductTier currentTier,
                                  bool earlyAccessOptIn)
    {
        // Checked before tier.
        if (isRemotelyDisabled(spec.id))
            return TweakLock::DisabledRemotely;

        if (spec.earlyAccess) {
            const bool ok = currentTier == vth::models::ProductTier::Ultimate
                         && earlyAccessOptIn;
            return ok ? TweakLock::Available : TweakLock::LockedEarlyAccess;
        }
        if (vth::models::tierRank(spec.minTier) > vth::models::tierRank(currentTier))
            return TweakLock::LockedTier;
        return TweakLock::Available;
    }

    namespace detail {

        RegistryOp dwordOp(const QString& path, const QString& name, quint32 applied,
                        std::optional<quint32> revert);
        RegistryOp stringOp(const QString& path, const QString& name, const QString& applied,
                            std::optional<QString> revert);

        void appendActive(QVector<TweakSpec>& c);
        void appendOperatorBatch(QVector<TweakSpec>& c);
        void appendOperatorGpuPower(QVector<TweakSpec>& c);
        void appendOperatorAdvanced(QVector<TweakSpec>& c);
        void appendOperatorDebloat(QVector<TweakSpec>& c);
    }
}

