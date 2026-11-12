#include "pch.h"
#include "TweakController.h"

#include "TweakCategoryPage.h"

#include "../../vth_core/Runtime/Dispatcher.h"
#include "../../vth_core/Runtime/Preflight.h"
#include "../../vth_core/Runtime/CrashRecoveryService.h"
#include "../../vth_core/Runtime/ServiceRegistry.h"
#include "../../vth_core/Runtime/Settings.h"
#include "../../vth_core/Runtime/TweakStateTracker.h"
#include "../../vth_core/Services/IRegistryEditor.h"
#include "../../vth_core/Services/RegistryEditor.h"
#include "../../vth_core/SystemRestore.h"

#include <QPointer>

namespace vth::ui
{
    namespace
    {
        using vth::tweaks::DetectState;

        // One registry failure, as the cause it actually is. The path and the
        // code are what decide it; neither is ever shown (they are in the log).
        QString explainRegistry(const vth::tweaks::OpFailure& f)
        {
            const QString path = f.target;
            switch (f.code) {
            case ERROR_ELEVATION_REQUIRED:
                return QObject::tr("This change needs administrator rights, and Vethida is not "
                                   "running with them. Close Vethida and open it again, and allow "
                                   "it when Windows asks.");
            case ERROR_ACCESS_DENIED:
                if (path.contains(QLatin1String("Windows Defender"), Qt::CaseInsensitive))
                    return QObject::tr("Windows Security's Tamper Protection blocks changes to "
                                       "Microsoft Defender. Turn Tamper Protection off in Windows "
                                       "Security, under Virus & threat protection settings, apply "
                                       "this, then switch Tamper Protection on again if you want it.");
                if (path.contains(QLatin1String("\\Policies\\"), Qt::CaseInsensitive))
                    return QObject::tr("This setting is managed by an organization policy on this PC "
                                       "(Group Policy or device management), which overrides changes "
                                       "made anywhere else.");
                if (path.contains(QLatin1String("\\Services\\"), Qt::CaseInsensitive))
                    return QObject::tr("Windows protects this service's settings so that only it can "
                                       "change them.");
                return QObject::tr("Windows has locked this setting so that only it can change it. "
                                   "Security or management software can do the same.");
            case ERROR_REGISTRY_IO_FAILED:
            case ERROR_REGISTRY_CORRUPT:
            case ERROR_CANTWRITE:
            case ERROR_CANTOPEN:
                return QObject::tr("Windows could not save the change because its settings store "
                                   "reported an error. Use \"Run system file check\" in System > Fixes, "
                                   "then restart.");
            case ERROR_NO_SYSTEM_RESOURCES:
            case ERROR_NOT_ENOUGH_MEMORY:
            case ERROR_OUTOFMEMORY:
                return QObject::tr("Windows ran out of resources while saving the change. Restart "
                                   "the PC and try again.");
            default:
                return f.code != 0
                    ? QObject::tr("Windows refused the change (error %1).").arg(f.code)
                    : QObject::tr("Windows refused the change.");
            }
        }

        QString explainCommand(const vth::tweaks::OpFailure& f)
        {
            const QString said = f.output.toLower();
            if (f.code == -1)
                return QObject::tr("A Windows tool this tweak uses would not start. It may have been "
                                   "removed, or security software is blocking it.");
            if (f.code == -2)
                return QObject::tr("A step took longer than three minutes and was stopped. Try "
                                   "again once the PC is less busy.");
            if (said.contains(QLatin1String("access is denied"))
                || said.contains(QLatin1String("access denied")))
                return QObject::tr("Windows refused one of the steps. Security software or a policy "
                                   "on this PC can do this.");
            if (said.contains(QLatin1String("not recognized"))
                || said.contains(QLatin1String("cannot find"))
                || said.contains(QLatin1String("could not find"))
                || said.contains(QLatin1String("does not exist")))
                return QObject::tr("Part of Windows this tweak works on is not present on this PC, "
                                   "so there was nothing to change.");
            return QObject::tr("One of the steps reported an error (code %1).").arg(f.code);
        }

        QString renderFailure(const vth::tweaks::ApplyOutcome& outcome)
        {
            // Refused, not failed: nothing was attempted and nothing is
            // half-applied, so no retry is offered.
            if (outcome.irreversible) {
                return QObject::tr(
                    "This tweak is marked Permanent and cannot be undone. Return Windows "
                    "to a restore point from before you applied it, or change the setting "
                    "back in Windows.");
            }

            using Kind = vth::tweaks::OpFailure::Kind;
            QStringList parts;
            const auto add = [&parts](const QString& s) {
                if (!s.isEmpty() && !parts.contains(s))
                    parts << s;
            };
            for (const auto& f : outcome.failures) {
                switch (f.kind) {
                case Kind::Registry: add(explainRegistry(f)); break;
                case Kind::Command:  add(explainCommand(f));  break;
                case Kind::Dynamic:
                    add(QObject::tr("A hardware setting (%1) could not be applied. The device may "
                                    "not support it, or its driver needs an update.").arg(f.target));
                    break;
                case Kind::Refused:
                    if (f.target == QLatin1String("protected"))
                        add(QObject::tr("Everything this tweak changes is a service Windows needs, "
                                        "so Vethida left it alone. Nothing was changed."));
                    else if (f.target == QLatin1String("withdrawn"))
                        add(QObject::tr("This tweak was turned off in a Vethida update."));
                    else
                        add(QObject::tr("Your plan does not include this tweak."));
                    break;
                }
            }
            if (parts.isEmpty())
                parts << QObject::tr("The tweak could not be applied.");
            if (outcome.failures.size() > 1 && parts.size() == 1)
                parts.first() += QLatin1Char(' ')
                    + QObject::tr("This affected %1 of its changes.").arg(outcome.failures.size());
            return parts.join(QLatin1Char(' '));
        }
    }

    TweakController::TweakController(RegistryFactory makeRegistry, QObject* parent)
        : QObject(parent)
        , m_makeRegistry(std::move(makeRegistry))
    {
        if (!m_makeRegistry) {
            m_makeRegistry = []() -> std::unique_ptr<vth::services::IRegistryEditor> {
                return std::make_unique<vth::services::RegistryEditor>();
            };
        }
        loadAppliedUndetectable();
        detectMachine();

        // An undo made from the Restore page changes what every tweak row
        // shows; re-read so no switch is left showing a change that is gone.
        if (auto* tracker = vth::ServiceRegistry::instance().get<vth::TweakStateTracker>())
            connect(tracker, &vth::TweakStateTracker::reverted, this,
                    [this]() { refreshDetection(); }, Qt::QueuedConnection);
    }

    // The persisted set of command-only tweaks the user has applied.
    namespace
    {
        constexpr char kAppliedUndetectableKey[] = "tweaks/appliedCommandOnly";
    }

    void TweakController::loadAppliedUndetectable()
    {
        auto* settings = vth::ServiceRegistry::instance().get<vth::Settings>();
        if (!settings)
            return;
        const QStringList ids =
            settings->value(QString::fromLatin1(kAppliedUndetectableKey)).toStringList();
        for (const QString& id : ids) {
            // Drop ids the catalog no longer has, so a renamed or retired tweak
            // does not keep a row alive in this file forever.
            if (vth::tweaks::find(id))
                m_appliedUndetectable.insert(id);
        }
    }

    void TweakController::saveAppliedUndetectable()
    {
        auto* settings = vth::ServiceRegistry::instance().get<vth::Settings>();
        if (!settings)
            return;
        QStringList ids(m_appliedUndetectable.cbegin(), m_appliedUndetectable.cend());
        ids.sort();   // stable on disk, so the file does not churn between runs
        settings->setValue(QString::fromLatin1(kAppliedUndetectableKey), ids);
    }

    void TweakController::applyUndetectableOverlay()
    {
        // Only touches ids detection can't speak about.
        for (const vth::tweaks::TweakSpec& spec : vth::tweaks::catalog()) {
            if (!vth::tweaks::isStateUndetectable(spec))
                continue;
            m_detected.insert(spec.id,
                              static_cast<int>(m_appliedUndetectable.contains(spec.id)
                                                   ? DetectState::On
                                                   : DetectState::Off));
        }
    }

    // Once per session, because none of it changes while the app is open.
    void TweakController::detectMachine()
    {
        m_profileToken = vth::fetch<vth::win::machine::Profile>(
            this,
            []() { return vth::win::machine::detect(); },
            [this](vth::win::machine::Profile p) {
                m_profile = std::move(p);
                m_profileReady = true;
                Q_EMIT machineProfileReady();
            });
    }

    QVector<const vth::tweaks::TweakSpec*>
    TweakController::bulkTargets(const QString& category, BulkAction action,
                                 const QStringList& ids) const
    {
        QVector<const vth::tweaks::TweakSpec*> out;

        if (action == BulkAction::Undo) {
            // newest first, skip anything already turned off by hand
            for (auto it = m_undoIds.crbegin(); it != m_undoIds.crend(); ++it) {
                const vth::tweaks::TweakSpec* spec = vth::tweaks::find(*it);
                if (spec && spec->reversible && detected(*it) == DetectState::On)
                    out.push_back(spec);
            }
            return out;
        }

        if (ids.isEmpty())
            return out;

        QVector<const vth::tweaks::TweakSpec*> candidates;
        if (action == BulkAction::Recommended) {
            const auto result = vth::tweaks::recommend::forCategory(
                category, m_profile, m_detected, m_tier, m_earlyAccessOptIn);
            const QSet<QString> shown(ids.cbegin(), ids.cend());
            for (const vth::tweaks::TweakSpec* spec : result.toApply)
                if (shown.contains(spec->id))
                    candidates.push_back(spec);
        } else {
            for (const QString& id : ids) {
                const vth::tweaks::TweakSpec* spec = vth::tweaks::find(id);
                if (!spec)
                    continue;
                // The one entitlement gate, the same one the single toggle asks.
                if (vth::tweaks::lockStateFor(*spec, m_tier, m_earlyAccessOptIn)
                    != vth::tweaks::TweakLock::Available)
                    continue;
                // Needs its own decision, so it stays on its row with its own prompt.
                if (vth::tweaks::needsUserConfirmation(*spec))
                    continue;
                // Hardware first, because a tweak for a device this PC doesn't have reads
                // back as neither on nor off.
                if (vth::tweaks::recommend::hardwareFit(*spec, m_profile)
                    == vth::tweaks::recommend::Skip::NotForThisHardware)
                    continue;
                if (detected(id) == DetectState::On)
                    continue;
                candidates.push_back(spec);
            }
        }

        // Mutually exclusive tweaks, settled here so both apply actions get it.
        QSet<QString> taken;
        for (const vth::tweaks::TweakSpec* spec : candidates) {
            bool clashes = false;
            for (const QString& other : spec->excludes) {
                if (taken.contains(other) || detected(other) == DetectState::On) {
                    clashes = true;
                    break;
                }
            }
            if (clashes)
                continue;
            taken.insert(spec->id);
            out.push_back(spec);
        }
        return out;
    }

    void TweakController::applyBulk(const QString& category, BulkAction action,
                                    const QStringList& ids)
    {
        // Every early return reports, because the caller's button is already loading.
        BulkReport report;
        report.category = category;
        report.action   = action;

        if (m_bulkInFlight || !m_profileReady || !m_detectDone) {
            report.end = BulkEnd::Abandoned;
            finishBulk(report);
            return;
        }

        const QVector<const vth::tweaks::TweakSpec*> specs =
            bulkTargets(category, action, ids);
        if (specs.isEmpty()) {
            finishBulk(report);
            return;
        }

        runBulk(category, action, specs);
    }

    bool TweakController::canUndo(const QString& category) const
    {
        return !m_bulkInFlight && !m_undoIds.isEmpty() && m_undoCategory == category;
    }

    void TweakController::runBulk(const QString& category, BulkAction action,
                                  const QVector<const vth::tweaks::TweakSpec*>& specs)
    {
        // Checked once for the batch, because these are facts about the session.
        const vth::preflight::Report pre = vth::preflight::check();
        if (pre.hasBlockers()) {
            BulkReport report;
            report.category = category;
            report.action   = action;
            report.end      = BulkEnd::Blocked;
            for (const auto& b : pre.blockers()) {
                report.reasons << (b.remediation.isEmpty()
                    ? b.message
                    : QStringLiteral("%1 %2").arg(b.message, b.remediation));
            }
            finishBulk(report);
            return;
        }

        m_bulkInFlight = true;
        Q_EMIT undoAvailabilityChanged();

        // Not offered for an Undo, which is the way back.
        auto* settings = vth::ServiceRegistry::instance().get<vth::Settings>();
        const bool wantRestorePoint =
            action != BulkAction::Undo
            && (!settings
                || settings->value(QStringLiteral("app/restorePointBeforeBulk"), true).toBool());
        if (!wantRestorePoint) {
            dispatchBulk(category, action, specs);
            return;
        }

        QPointer<TweakController> guard(this);
        vth::Dispatcher::instance().dispatch([guard, category, action, specs]() {
            namespace sr = vth::win::sysrestore;
            const sr::CreateOutcome outcome = sr::create(
                QStringLiteral("Vethida: before applying changes"), false);
            vth::Dispatcher::instance().dispatchUi([guard, category, action, specs, outcome]() {
                if (!guard)
                    return;
                // Throttled counts as made, because Windows refuses a second point within
                // 24 hours.
                const bool have = outcome.result == sr::CreateResult::Ok
                               || outcome.result == sr::CreateResult::Throttled;
                if (have) {
                    guard->dispatchBulk(category, action, specs);
                    return;
                }
                // Asked, because carrying on quietly leaves someone believing they have a
                // way back.
                QString reason = outcome.error.isEmpty()
                    ? sr::createResultLabel(outcome.result) : outcome.error;
                if (outcome.result == sr::CreateResult::ProtectionDisabled
                    || outcome.result == sr::CreateResult::ServicesDisabled)
                    reason += QLatin1Char(' ') + tr("Creating a restore point on the Restore "
                                                    "page offers to switch it on.");
                if (guard->m_onRestorePointFailed && guard->m_onRestorePointFailed(reason)) {
                    guard->dispatchBulk(category, action, specs);
                    return;
                }
                BulkReport report;
                report.category = category;
                report.action   = action;
                report.end      = BulkEnd::Abandoned;
                guard->m_bulkInFlight = false;
                guard->finishBulk(report);
            });
        });
    }

    void TweakController::dispatchBulk(const QString& category, BulkAction action,
                                       const QVector<const vth::tweaks::TweakSpec*>& specs)
    {
        QPointer<TweakController> guard(this);
        auto make = m_makeRegistry;
        const bool applying = action != BulkAction::Undo;

        // Write-ahead crash marker, bracketing the batch.
        QStringList opIds;
        opIds.reserve(specs.size());
        for (const vth::tweaks::TweakSpec* spec : specs)
            opIds << spec->id;

        // One id for the run, shared by the crash marker and every ledger snapshot.
        const QString kind = action == BulkAction::Recommended ? QStringLiteral("recommended")
                           : action == BulkAction::Everything  ? QStringLiteral("all")
                                                               : QStringLiteral("undo");
        const QString batchId =
            vth::newTweakBatchId(QStringLiteral("%1:%2").arg(kind, category));

        if (auto* recovery =
                vth::ServiceRegistry::instance().get<vth::CrashRecoveryService>()) {
            recovery->beginBatch(batchId, opIds);
        }

        vth::Dispatcher::instance().dispatch(
            [guard, make, specs, category, action, applying, batchId]() {
            // one batch id for everything in this run
            const vth::TweakBatch batch(batchId);

            auto reg = make();
            struct Row {
                QString id;
                bool ok = false;
                int state = 0;
                vth::tweaks::ApplyOutcome outcome;
            };
            QVector<Row> rows;
            rows.reserve(specs.size());

            for (const vth::tweaks::TweakSpec* spec : specs) {
                // Before the write, so the band names what's being written.
                const QString label = spec->name;
                vth::Dispatcher::instance().dispatchUi([guard, label]() {
                    if (guard)
                        Q_EMIT guard->bulkProgress(label);
                });

                const vth::tweaks::ApplyOutcome outcome = applying
                    ? vth::tweaks::applyVerbose(*spec, *reg)
                    : vth::tweaks::revertVerbose(*spec, *reg);
                // Command-only tweaks always detect Unknown, so their outcome is the only
                // evidence.
                const bool nowOn = applying ? outcome.ok : !outcome.ok;
                const int state = vth::tweaks::isStateUndetectable(*spec)
                    ? static_cast<int>(nowOn ? vth::tweaks::DetectState::On
                                             : vth::tweaks::DetectState::Off)
                    : static_cast<int>(vth::tweaks::detect(*spec, *reg));
                rows.push_back({ spec->id, outcome.ok, state, outcome });
            }

            vth::Dispatcher::instance().dispatchUi([guard, rows, category, action, applying]() {
                if (!guard)
                    return;
                BulkReport report;
                report.category = category;
                report.action   = action;
                QStringList turnedOn;
                for (const Row& r : rows) {
                    guard->m_detected.insert(r.id, r.state);
                    // Same rule as the single-toggle path: a sweep in flight
                    // carries an older snapshot and must not overwrite these.
                    if (guard->m_sweepInFlight)
                        guard->m_writtenDuringSweep.insert(r.id);
                    const bool on = static_cast<vth::tweaks::DetectState>(r.state)
                                    == vth::tweaks::DetectState::On;
                    // remember the ones we can't detect so they show as on next time
                    if (const vth::tweaks::TweakSpec* spec = vth::tweaks::find(r.id);
                        spec && vth::tweaks::isStateUndetectable(*spec)) {
                        if (on) guard->m_appliedUndetectable.insert(r.id);
                        else    guard->m_appliedUndetectable.remove(r.id);
                    }
                    // same row note as a single toggle, set before reflect draws it
                    if (r.ok)
                        guard->m_reason.remove(r.id);
                    else
                        guard->m_reason.insert(r.id, renderFailure(r.outcome));
                    guard->reflect(r.id);
                    // Success is the row reaching the state the run was for, because a write
                    // Tamper Protection dropped returns ok and detects Off.
                    const bool unreadable = static_cast<vth::tweaks::DetectState>(r.state)
                                            == vth::tweaks::DetectState::Unknown;
                    const bool done = r.ok && (on == applying || unreadable);
                    if (done)
                        ++report.succeeded;
                    else
                        ++report.failed;
                    if (applying && r.ok && (on || unreadable))
                        turnedOn << r.id;
                }
                // Cleared here because endBatch asserts the UI thread.
                if (auto* recovery = vth::ServiceRegistry::instance()
                                         .get<vth::CrashRecoveryService>()) {
                    recovery->endBatch();
                }
                // Command-only tweaks have no readable state, so they're recorded.
                guard->saveAppliedUndetectable();
                guard->m_bulkInFlight = false;
                // Only what this run turned on. An Undo clears the record.
                guard->setUndoable(category, applying ? turnedOn : QStringList());
                guard->finishBulk(report);
            });
        });
    }

    void TweakController::setUndoable(const QString& category, const QStringList& ids)
    {
        m_undoCategory = ids.isEmpty() ? QString() : category;
        m_undoIds      = ids;
        Q_EMIT undoAvailabilityChanged();
    }

    void TweakController::finishBulk(const BulkReport& report)
    {
        Q_EMIT bulkFinished(report);
        Q_EMIT undoAvailabilityChanged();
    }

    TweakController::~TweakController() = default;

    void TweakController::setConfirmHook(ConfirmHook hook)   { m_confirm = std::move(hook); }
    void TweakController::setLockedHook(LockedHook hook)     { m_onLocked = std::move(hook); }
    void TweakController::setConflictHook(ConflictHook hook) { m_onConflict = std::move(hook); }
    void TweakController::setRestorePointFailedHook(RestorePointFailedHook hook)
    {
        m_onRestorePointFailed = std::move(hook);
    }

    void TweakController::setEntitlement(vth::models::ProductTier tier, bool earlyAccessOptIn)
    {
        m_tier = tier;
        m_earlyAccessOptIn = earlyAccessOptIn;
        for (TweakCategoryPage* page : m_bound) {
            if (!page)
                continue;
            page->setCurrentTierRank(vth::models::tierRank(tier));
            page->setEarlyAccessOptIn(earlyAccessOptIn);
        }
    }

    void TweakController::setElevated(bool elevated)
    {
        m_elevated = elevated;
        for (TweakCategoryPage* page : m_bound)
            if (page && m_detectDone)
                page->setToggleInteractive(elevated);
    }

    void TweakController::bind(TweakCategoryPage* page)
    {
        if (!page || m_bound.contains(page))
            return;
        m_bound.push_back(page);
        connect(page, &TweakCategoryPage::tweakToggled, this, &TweakController::onTweakToggled);
        connect(page, &QObject::destroyed, this, [this, page]() { m_bound.removeAll(page); });

        // The bulk path, wired here rather than at each call site so every pane
        // that binds gets all three actions on the same terms.
        connect(page, &TweakCategoryPage::bulkRequested,
                this, [this, page](const QString& category, BulkAction action,
                                   const QStringList& ids) {
                    page->setBulkBusy(true);
                    applyBulk(category, action, ids);
                });
        connect(this, &TweakController::bulkFinished, page,
                [page](const BulkReport&) { page->setBulkBusy(false); });

        // The pane decides whether it's the one that pressed.
        connect(this, &TweakController::bulkProgress, page,
                [page](const QString& label) { page->setBulkProgress(label); });

        // every pane, so no second run can start
        const auto pushAvailability = [this, page]() {
            page->setBulkAvailability(!m_bulkInFlight, canUndo(page->bulkCategory()));
        };
        connect(this, &TweakController::undoAvailabilityChanged, page, pushAvailability);
        connect(page, &TweakCategoryPage::bulkCategoryChanged, this, pushAvailability);
        pushAvailability();

        connect(this, &TweakController::machineProfileReady, page,
                [this, page]() { page->setBulkReady(m_profileReady); });
        page->setBulkReady(m_profileReady);

        // A page bound after detection has already run would otherwise show every
        // tweak as off until the next sweep.
        page->setCurrentTierRank(vth::models::tierRank(m_tier));
        page->setEarlyAccessOptIn(m_earlyAccessOptIn);
        // spinner and disabled switches until the first detection finishes
        page->setStatesLoading(!m_detectDone);
        if (m_detectDone) {
            page->setDetected(m_detected);
            page->setToggleInteractive(m_elevated);
        }
    }

    vth::tweaks::DetectState TweakController::detected(const QString& id) const
    {
        return static_cast<DetectState>(m_detected.value(id, static_cast<int>(DetectState::Unknown)));
    }

    QString TweakController::failureReason(const QString& id) const
    {
        return m_reason.value(id);
    }

    // Both of these read the ledger, which files a tweak's snapshots under its
    // own id as the domain - so they answer about that tweak and nothing else.
    bool TweakController::hasRecordedChange(const QString& id) const
    {
        return !vth::tweakSnapshots(id).isEmpty();
    }

    QDateTime TweakController::lastRecordedChange(const QString& id) const
    {
        QDateTime latest;
        for (const vth::TweakSnapshot& s : vth::tweakSnapshots(id)) {
            // Entries written before the ledger carried a timestamp have none,
            // and they are still real changes. This says when, never whether.
            if (s.appliedAt.isValid() && (!latest.isValid() || s.appliedAt > latest))
                latest = s.appliedAt;
        }
        return latest;
    }

    void TweakController::revertTweak(const QString& id)
    {
        // The off direction of the one apply path.
        onTweakToggled(id, false);
    }

    void TweakController::refreshDetection(bool force)
    {
        // queue a forced refresh that comes in mid-sweep, the running one is stale
        if (m_sweepInFlight) {
            m_resweepPending = m_resweepPending || force;
            return;
        }
        if (m_detectDone && !force)
            return;
        m_sweepInFlight = true;
        m_writtenDuringSweep.clear();

        QPointer<TweakController> guard(this);
        auto make = m_makeRegistry;
        vth::Dispatcher::instance().dispatch([guard, make]() {
            auto reg = make();
            QHash<QString, int> states;
            for (const vth::tweaks::TweakSpec& spec : vth::tweaks::catalog())
                states.insert(spec.id, static_cast<int>(vth::tweaks::detect(spec, *reg)));
            vth::Dispatcher::instance().dispatchUi([guard, states]() {
                if (!guard)
                    return;
                // skip ids toggled while the sweep was running
                for (auto it = states.constBegin(); it != states.constEnd(); ++it)
                    if (!guard->m_writtenDuringSweep.contains(it.key()))
                        guard->m_detected.insert(it.key(), it.value());
                // Over the ids detection can't answer for.
                guard->applyUndetectableOverlay();
                guard->m_writtenDuringSweep.clear();
                guard->m_detectDone = true;
                guard->m_sweepInFlight = false;
                guard->publishDetection();
                if (guard->m_resweepPending) {
                    guard->m_resweepPending = false;
                    guard->refreshDetection(true);
                }
            });
        });
    }

    void TweakController::publishDetection()
    {
        for (TweakCategoryPage* page : m_bound) {
            if (!page)
                continue;
            page->setDetected(m_detected);
            page->setToggleInteractive(m_elevated);
            page->setStatesLoading(false);
        }
    }

    void TweakController::reflect(const QString& id)
    {
        const int state = m_detected.value(id, static_cast<int>(DetectState::Unknown));
        for (TweakCategoryPage* page : m_bound)
            if (page)
                page->setTweakState(id, state);
    }

    void TweakController::onTweakToggled(const QString& id, bool on)
    {
        const vth::tweaks::TweakSpec* spec = vth::tweaks::find(id);
        if (!spec)
            return;

        // The one entitlement gate.
        if (on) {
            const vth::tweaks::TweakLock lock =
                vth::tweaks::lockStateFor(*spec, m_tier, m_earlyAccessOptIn);
            if (lock != vth::tweaks::TweakLock::Available) {
                reflect(id);
                if (m_onLocked)
                    m_onLocked(*spec, lock);
                return;
            }
        }

        // Refuse to stack mutually-exclusive tweaks, because whichever reverts second
        // restores a value captured while the other was active.
        const vth::tweaks::TweakSpec* clearFirst = nullptr;
        if (on) {
            for (const QString& otherId : spec->excludes) {
                if (detected(otherId) != DetectState::On)
                    continue;
                const vth::tweaks::TweakSpec* other = vth::tweaks::find(otherId);
                // No hook, or an unknown counterpart: refuse, as before. There
                // is nothing to offer and applying both is not an option.
                if (!other || !m_onConflict || !m_onConflict(*spec, *other)) {
                    reflect(id);
                    return;
                }
                clearFirst = other;
                break;
            }
        }

        if (on && vth::tweaks::needsUserConfirmation(*spec)) {
            if (m_confirm && !m_confirm(*spec)) {
                reflect(id);
                return;
            }
        }

        QPointer<TweakController> guard(this);
        auto make = m_makeRegistry;
        vth::Dispatcher::instance().dispatch([guard, make, id, spec, on, clearFirst]() {
            // One act is one batch. No crash marker, because this returns to the event
            // loop between changes.
            const vth::TweakBatch batch(vth::newTweakBatchId(id));

            auto reg = make();

            // turn the conflicting tweak off first, same pass
            QString clearedId;
            int  clearedState = 0;
            bool clearedOk    = false;
            if (clearFirst) {
                clearedOk    = vth::tweaks::revertVerbose(*clearFirst, *reg).ok;
                clearedId    = clearFirst->id;
                clearedState = static_cast<int>(vth::tweaks::detect(*clearFirst, *reg));
            }

            const vth::tweaks::ApplyOutcome outcome = on
                ? vth::tweaks::applyVerbose(*spec, *reg)
                : vth::tweaks::revertVerbose(*spec, *reg);
            const int detectedState = static_cast<int>(vth::tweaks::detect(*spec, *reg));
            // spec points into the static catalog, so copying the pointer is safe.
            vth::Dispatcher::instance().dispatchUi([guard, id, on, spec, outcome, detectedState,
                                                    clearFirst, clearedId, clearedState,
                                                    clearedOk]() {
                if (!guard)
                    return;

                // Published before the requested one, so the pane never shows both on.
                if (clearFirst) {
                    bool clearedShown =
                        static_cast<DetectState>(clearedState) == DetectState::On;
                    if (vth::tweaks::isStateUndetectable(*clearFirst)) {
                        if (clearedOk) {
                            guard->m_appliedUndetectable.remove(clearedId);
                            guard->saveAppliedUndetectable();
                            clearedShown = false;
                        } else {
                            clearedShown = guard->m_appliedUndetectable.contains(clearedId);
                        }
                    }
                    guard->m_detected.insert(
                        clearedId,
                        static_cast<int>(clearedShown ? DetectState::On : DetectState::Off));
                    if (guard->m_sweepInFlight)
                        guard->m_writtenDuringSweep.insert(clearedId);
                    guard->reflect(clearedId);
                }
                // Reflects what the registry shows. A partial apply detects as Off, and the
                // reason string explains why.
                guard->m_detected.insert(id, detectedState);
                // A write that lands while a full sweep is in flight must not be
                // clobbered by that sweep's older snapshot when it returns.
                if (guard->m_sweepInFlight)
                    guard->m_writtenDuringSweep.insert(id);
                if (outcome.ok)
                    guard->m_reason.remove(id);
                else
                    guard->m_reason.insert(id, renderFailure(outcome));

                // Detection answers Unknown for command-only tweaks, so their outcome decides.
                bool shown = static_cast<DetectState>(detectedState) == DetectState::On;
                if (vth::tweaks::isStateUndetectable(*spec)) {
                    if (outcome.ok) {
                        if (on) guard->m_appliedUndetectable.insert(id);
                        else    guard->m_appliedUndetectable.remove(id);
                        guard->saveAppliedUndetectable();
                        shown = on;
                    } else {
                        shown = guard->m_appliedUndetectable.contains(id);
                    }
                    guard->m_detected.insert(
                        id, static_cast<int>(shown ? DetectState::On : DetectState::Off));
                }
                guard->reflect(id);
                Q_EMIT guard->operationFinished(id, outcome);
            });
        });
    }
}

namespace vth::ui
{
    void TweakController::requestToggle(const QString& id, bool on)
    {
        onTweakToggled(id, on);
    }
}

