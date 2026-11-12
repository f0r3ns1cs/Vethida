#pragma once

#include "../../vth_core/Models/ProductTier.h"
#include "../../vth_core/Runtime/Fetch.h"
#include "../../vth_core/Tweaks/Recommend.h"
#include "../../vth_core/Tweaks/TweakEngine.h"
#include "../../vth_core/Windows/MachineProfile.h"

#include <QDateTime>
#include <QHash>
#include <QMetaType>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>

#include <functional>
#include <memory>

namespace vth::services { class IRegistryEditor; }

namespace vth::ui
{
    class TweakCategoryPage;

    // What a pane can ask for over a whole list.
    enum class BulkAction
    {
        Recommended, // Recommend.h picks for this machine, nothing risky
        // everything their tier allows that isn't on, except what needsUserConfirmation lists
        Everything,
        // Reverse exactly what the last run on this category turned on.
        Undo,
    };

    // How a bulk run ended. One signal carries all three, because a pane has a spinner
    // to stop whichever happened.
    enum class BulkEnd
    {
        Ran,         // the batch went through; succeeded/failed say how it went
        Blocked,     // refused before anything was written; reasons says why
        // didn't run, nothing to report (canceled, no detection yet, or another run going)
        Abandoned,
    };

    struct BulkReport
    {
        QString     category;
        BulkAction  action = BulkAction::Recommended;
        BulkEnd     end    = BulkEnd::Ran;
        int         succeeded = 0; // counts operations, not tweaks
        int         failed    = 0;
        QStringList reasons;   // Blocked only
    };

    // The one place that knows how to run a tweak.
    class TweakController final : public QObject
    {
        Q_OBJECT

    public:
        Q_INVOKABLE void requestToggle(const QString& id, bool on);
        // Built per operation on the worker thread. A fake here tests apply and revert
        // without a registry.
        using RegistryFactory = std::function<std::unique_ptr<vth::services::IRegistryEditor>()>;

        // Returns false to abandon the operation. Runs on the UI thread, before
        // any work is dispatched.
        using ConfirmHook = std::function<bool(const vth::tweaks::TweakSpec&)>;

        // Called instead of applying when entitlement refuses the tweak, so the
        // surface can raise its own upsell. The controller never opens UI itself.
        using LockedHook = std::function<void(const vth::tweaks::TweakSpec&,
                                              vth::tweaks::TweakLock)>;

        // called when an excluded tweak is on (both on would break revert)
        using ConflictHook = std::function<bool(const vth::tweaks::TweakSpec& requested,
                                                const vth::tweaks::TweakSpec& applied)>;

        // restore point failed, return true to continue anyway
        using RestorePointFailedHook = std::function<bool(const QString& reason)>;

        explicit TweakController(RegistryFactory makeRegistry = {},
                                 QObject* parent = nullptr);
        ~TweakController() override;

        void setConfirmHook(ConfirmHook hook);
        void setLockedHook(LockedHook hook);
        void setConflictHook(ConflictHook hook);
        void setRestorePointFailedHook(RestorePointFailedHook hook);

        void setEntitlement(vth::models::ProductTier tier, bool earlyAccessOptIn);

        // Unelevated sessions cannot write HKLM. Surfaces reflect this by
        // disabling toggles; the controller refuses the write regardless.
        void setElevated(bool elevated);

        // unbinds itself on destruction
        void bind(TweakCategoryPage* page);

        // One sweep of the catalog, shared by every bound page. A completed sweep is
        // cached until force is passed.
        void refreshDetection(bool force = false);

        vth::models::ProductTier tier() const { return m_tier; }
        bool earlyAccessOptIn() const { return m_earlyAccessOptIn; }
        bool elevated() const { return m_elevated; }

        vth::tweaks::DetectState detected(const QString& id) const;

        // Why the last operation on this tweak failed, empty if it succeeded.
        QString failureReason(const QString& id) const;

        // Whether the rollback ledger holds captured pre-apply values for this
        // tweak - which is to say, whether Vethida changed it.
        bool hasRecordedChange(const QString& id) const;

        // invalid if none or too old to have a timestamp, use hasRecordedChange for "whether"
        QDateTime lastRecordedChange(const QString& id) const;

        // Put back what Vethida captured before it changed this tweak.
        Q_INVOKABLE void revertTweak(const QString& id);

        // This machine, as the recommendation engine sees it. Every field reads Unknown
        // until machineProfileReady.
        const vth::win::machine::Profile& machineProfile() const
        {
            return m_profile;
        }

        // Runs one action over a whole list, through the same apply path and entitlement
        // gate a single toggle takes.
        void applyBulk(const QString& category, BulkAction action, const QStringList& ids);

        // True when the last bulk run on this category turned something on that's still
        // undoable.
        bool canUndo(const QString& category) const;

    Q_SIGNALS:
        void operationFinished(const QString& id, const vth::tweaks::ApplyOutcome& outcome);

        // The machine profile has landed; surfaces that grayed out their
        // recommendation affordance can enable it.
        void machineProfileReady();

        // What the run is writing right now, by name. The one in flight, not
        // the one just finished.
        void bulkProgress(const QString& label);

        // always emitted when a bulk run ends
        void bulkFinished(const vth::ui::BulkReport& report);

        // Whether canUndo() would now answer differently. Panes offering an Undo
        // watch this rather than polling.
        void undoAvailabilityChanged();

    private:
        void onTweakToggled(const QString& id, bool on);
        void publishDetection();
        // Push what m_detected holds for one id to every bound page.
        void reflect(const QString& id);
        void detectMachine();
        // filtered by tier and current state, empty = nothing to do
        QVector<const vth::tweaks::TweakSpec*> bulkTargets(const QString& category,
                                                           BulkAction action,
                                                           const QStringList& ids) const;
        // Everything from the preflight onward, shared by all three actions.
        void runBulk(const QString& category, BulkAction action,
                     const QVector<const vth::tweaks::TweakSpec*>& specs);
        // The worker pass and the state publishing that follows it.
        void dispatchBulk(const QString& category, BulkAction action,
                          const QVector<const vth::tweaks::TweakSpec*>& specs);
        void finishBulk(const BulkReport& report);
        void setUndoable(const QString& category, const QStringList& ids);
        // Load/store m_appliedUndetectable. Kept out of the sweep so a failure to
        // reach Settings degrades to "shows off" rather than losing the toggle.
        void loadAppliedUndetectable();
        void saveAppliedUndetectable();
        // Folds m_appliedUndetectable over a detection map, replacing the
        // meaningless Unknown for command-only tweaks with what we recorded.
        void applyUndetectableOverlay();

        vth::win::machine::Profile   m_profile;
        FetchToken                   m_profileToken;

        RegistryFactory              m_makeRegistry;
        ConfirmHook                  m_confirm;
        LockedHook                   m_onLocked;
        ConflictHook                 m_onConflict;
        RestorePointFailedHook       m_onRestorePointFailed;
        QVector<TweakCategoryPage*>  m_bound;
        QHash<QString, int>          m_detected;
        QHash<QString, QString>      m_reason;
        // Ids whose state was written by an apply/revert while a full sweep was
        // in flight; that sweep's (older) snapshot must not overwrite them.
        QSet<QString>                m_writtenDuringSweep;
        // Command-only tweaks the user has applied through Vethida.
        QSet<QString>                m_appliedUndetectable;
        vth::models::ProductTier     m_tier = vth::models::ProductTier::None;
        bool                         m_earlyAccessOptIn = false;
        bool                         m_elevated = false;
        // Detection has landed, whatever it found. The only thing the
        // recommended row's existence may depend on.
        bool                         m_profileReady = false;
        bool                         m_detectDone = false;
        bool                         m_sweepInFlight = false;
        // A forced refresh that arrived while a sweep was running.
        bool                         m_resweepPending = false;
        // One bulk run at a time, across every bound pane.
        bool                         m_bulkInFlight = false;

        // What the last bulk run turned on, and where. Session-only, because an Undo
        // against a stale list would reverse changes made on purpose.
        QString                      m_undoCategory;
        QStringList                  m_undoIds;
    };
}

// Queued connections need this even though every emit here is already on the UI
// thread, because nothing stops a caller connecting one.
Q_DECLARE_METATYPE(vth::ui::BulkReport)

