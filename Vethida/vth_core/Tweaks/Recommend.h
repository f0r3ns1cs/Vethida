#pragma once

#include "TweakEngine.h"
#include "../Models/ProductTier.h"
#include "../Windows/MachineProfile.h"

#include <QHash>
#include <QString>
#include <QVector>

namespace vth::tweaks::recommend
{
    // Why a tweak was left out.
    enum class Skip
    {
        None,
        NotForThisHardware,   // e.g. an NVIDIA tweak on an AMD GPU
        CouldCauseHarm,       // heat, power, security, or a listed side effect
        NotReversible,
        AboveTier,
        EarlyAccess,
        AlreadyApplied,
    };

    struct Candidate
    {
        const TweakSpec* spec = nullptr;
        Skip             skip = Skip::None;

        bool recommended() const { return skip == Skip::None; }
    };

    struct Result
    {
        QVector<Candidate> candidates;         // every spec considered, in catalog order
        QVector<const TweakSpec*> toApply;     // recommended and not already on

        bool anythingToDo() const { return !toApply.isEmpty(); }
    };

    // A key may join several categories with '+', such as "privacy+security".
    bool categoryMatches(const QString& specCategory, const QString& key);

    // states maps tweak id to DetectState.
    Result forCategory(const QString& category,
                       const vth::win::machine::Profile& profile,
                       const QHash<QString, int>& states,
                       vth::models::ProductTier tier,
                       bool earlyAccessOptIn);

    Skip hardwareFit(const TweakSpec& spec, const vth::win::machine::Profile& profile);
}

namespace vth::tweaks::recommend
{
    QString skipLabel(Skip skip);
}

