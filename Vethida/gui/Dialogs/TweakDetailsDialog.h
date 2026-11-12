#pragma once

#include <QDateTime>
#include <QString>

class QWidget;

namespace vth::tweaks { struct TweakSpec; }

namespace vth::dialogs
{
    // What the caller knows about the machine and the ledger.
    struct TweakDetailsContext
    {
        // The caller's last apply error for this tweak, if any.
        QString lastFailureReason;
        // The rollback ledger holds captured pre-apply values for this tweak.
        bool changedByVethida = false;
        // When the newest was written. Invalid for entries that predate timestamps.
        QDateTime changedAt;
    };

    // how the modal was closed
    enum class TweakDetailsOutcome
    {
        Closed,
        // They asked to put back what Vethida captured. The caller does it.
        RevertRequested,
    };

    TweakDetailsOutcome showTweakDetails(QWidget* parent,
                                         const vth::tweaks::TweakSpec& t,
                                         const TweakDetailsContext& ctx = {});

    // The warning reader behind a row's "N warnings" chip: the warnings in full, with
    // no apply or revert.
    void showTweakWarnings(QWidget* parent, const vth::tweaks::TweakSpec& t);
}
