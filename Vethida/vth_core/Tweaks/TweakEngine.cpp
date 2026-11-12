#include "TweakEngine.h"

#include "../Logger.h"
#include "../ProtectedServices.h"
#include "../Runtime/ServiceRegistry.h"
#include "../Runtime/Settings.h"
#include "../Runtime/TweakStateTracker.h"
#include "../Runtime/Tier.h"

#include <QCoreApplication>
#include <QtCore/QHash>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QMutex>
#include <QtCore/QProcess>
#include <QtCore/QSet>

#include <windows.h>
#include "../Windows/BcdBackup.h"
#include <QHash>
#include <QtCore/QFileInfo>

namespace vth::tweaks {
    namespace {
        VTH_DEFINE_LOG_CATEGORY(catTweakEngine, "vth.tweakengine")

        using vth::services::RegistryValueKind;

        // Commands run elevated, so a bare name must not resolve via PATH. Pin it
        // to System32.
        QString pinnedProgram(const QString& program) {
            if (program.contains(QLatin1Char('\\')) || program.contains(QLatin1Char('/')))
                return program;

            wchar_t dir[MAX_PATH] = {};
            const UINT n = GetSystemDirectoryW(dir, MAX_PATH);
            const QString system32 = (n > 0 && n < MAX_PATH)
                ? QString::fromWCharArray(dir, static_cast<int>(n))
                : QStringLiteral("C:\\Windows\\System32");

            QString exe = program;
            if (!exe.endsWith(QStringLiteral(".exe"), Qt::CaseInsensitive))
                exe += QStringLiteral(".exe");

            // powershell.exe is not directly under System32.
            if (exe.compare(QStringLiteral("powershell.exe"), Qt::CaseInsensitive) == 0)
                return system32 + QStringLiteral("\\WindowsPowerShell\\v1.0\\powershell.exe");

            return system32 + QLatin1Char('\\') + exe;
        }

        // How the last command on this thread ended, for the failure record.
        struct LastRun { long code = 0; QString output; };
        thread_local LastRun t_lastRun;

        bool runProcess(const QString& program, const QStringList& args) {
            t_lastRun = {};
            QProcess p;
            p.setProcessChannelMode(QProcess::MergedChannels);
            p.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* a) {
                a->flags |= 0x08000000;   // CREATE_NO_WINDOW
            });
            p.start(pinnedProgram(program), args);
            if (!p.waitForStarted(10000)) {
                t_lastRun.code = -1;
                t_lastRun.output = p.errorString();
                return false;
            }
            if (!p.waitForFinished(180000)) {   // appx/installer ops can run for minutes
                p.kill();
                p.waitForFinished(2000);
                t_lastRun.code = -2;
                return false;
            }
            t_lastRun.code = p.exitStatus() == QProcess::NormalExit ? p.exitCode() : -3;
            if (t_lastRun.code != 0)
                t_lastRun.output = QString::fromLocal8Bit(p.readAll()).trimmed().right(600);
            return p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0;
        }

        bool isIntegerKind(RegistryValueKind k) {
            return k == RegistryValueKind::DWord || k == RegistryValueKind::QWord;
        }

        bool valueEquals(const QVariant& a, const QVariant& b, RegistryValueKind kind) {
            if (!a.isValid() || !b.isValid()) return false;
            if (isIntegerKind(kind))
                return a.toULongLong() == b.toULongLong();
            if (kind == RegistryValueKind::Binary)
                return a.toByteArray() == b.toByteArray();
            // toString() turns any list longer than one entry into "", which
            // would read two page files as the empty "no page file" list.
            if (kind == RegistryValueKind::MultiString)
                return a.toStringList() == b.toStringList();
            return a.toString() == b.toString();
        }

        bool isCapturableKind(RegistryValueKind k) {
            return k == RegistryValueKind::DWord || k == RegistryValueKind::String;
        }

        void splitHiveSubkey(const QString& path, QString& hive, QString& subkey) {
            const int slash = path.indexOf(QLatin1Char('\\'));
            hive   = slash < 0 ? path : path.left(slash);
            subkey = slash < 0 ? QString() : path.mid(slash + 1);
        }

        QString capKey(const QString& hive, const QString& subkey, const QString& name) {
            return hive.toUpper() + QLatin1Char('|') + subkey + QLatin1Char('|') + name;
        }

        // Skipped when already applied, so a re-apply keeps the original snapshot.
        void captureRegistryOp(const QString& domain, const RegistryOp& op,
                               const vth::services::IRegistryEditor& reg) {
            if (!isCapturableKind(op.kind)) return;
            const QVariant current = reg.getValue(op.path, op.name);
            if (valueEquals(current, op.appliedValue, op.kind)) return;

            vth::TweakSnapshot snap;
            snap.domain = domain;
            snap.kind   = vth::SnapshotKind::Registry;
            snap.label  = op.name.isEmpty() ? op.path : (op.path + QLatin1Char('\\') + op.name);
            splitHiveSubkey(op.path, snap.hive, snap.subkey);
            snap.name          = op.name;
            snap.existedBefore = reg.valueExists(op.path, op.name);
            snap.isDword       = (op.kind == RegistryValueKind::DWord);
            if (snap.isDword) snap.prevDword  = current.toUInt();
            else              snap.prevString = current.toString();
            vth::recordTweakSnapshot(snap);
        }

        bool restoreFromSnapshot(const RegistryOp& op, const vth::TweakSnapshot& s,
                                 vth::services::IRegistryEditor& reg) {
            if (!s.existedBefore)
                return reg.deleteValue(op.path, op.name);
            if (s.isDword)
                return reg.setValue(op.path, op.name, QVariant::fromValue<uint>(s.prevDword),
                                    RegistryValueKind::DWord);
            return reg.setValue(op.path, op.name, s.prevString, RegistryValueKind::String);
        }

    }


    RegistryOp detail::dwordOp(const QString& path, const QString& name, quint32 applied,
                            std::optional<quint32> revert) {
        RegistryOp op;
        op.path = path;
        op.name = name;
        op.kind = vth::services::RegistryValueKind::DWord;
        op.appliedValue = QVariant::fromValue<uint>(applied);
        if (revert.has_value())
            op.revertValue = QVariant::fromValue<uint>(*revert);
        return op;
    }

    RegistryOp detail::stringOp(const QString& path, const QString& name, const QString& applied,
                                std::optional<QString> revert) {
        RegistryOp op;
        op.path = path;
        op.name = name;
        op.kind = vth::services::RegistryValueKind::String;
        op.appliedValue = applied;
        if (revert.has_value())
            op.revertValue = *revert;
        return op;
    }


    namespace {
        // Tier is scored from what a tweak does, never its category or tab.

        // Does any op write to a path, value name or argument containing `needle`?
        bool writesAnything(const TweakSpec& t, const QString& needle) {
            for (const RegistryOp& op : t.registryOps) {
                if (op.path.contains(needle, Qt::CaseInsensitive)
                    || op.name.contains(needle, Qt::CaseInsensitive))
                    return true;
            }
            for (const CommandOp& op : t.commandOps) {
                for (const QString& a : op.applyArgs)
                    if (a.contains(needle, Qt::CaseInsensitive))
                        return true;
            }
            return false;
        }

        bool writesAnyOf(const TweakSpec& t, const QStringList& needles) {
            for (const QString& needle : needles)
                if (writesAnything(t, needle))
                    return true;
            return false;
        }

        bool weakensSecurityFeature(const TweakSpec& t) {
            static const QStringList kSecurityTargets = {
                QStringLiteral("Windows Defender"),
                QStringLiteral("DisableAntiSpyware"),
                QStringLiteral("DisableRealtimeMonitoring"),
                QStringLiteral("TamperProtection"),
                QStringLiteral("SmartScreen"),
                QStringLiteral("EnableLUA"),
                QStringLiteral("ConsentPromptBehavior"),
                QStringLiteral("PromptOnSecureDesktop"),
                QStringLiteral("LocalAccountTokenFilterPolicy"),
                QStringLiteral("FeatureSettingsOverride"),   // Spectre / Meltdown
                QStringLiteral("MitigationOptions"),         // DEP / CFG / ASLR
                QStringLiteral("MoveImages"),                // ASLR
                QStringLiteral("DisableExceptionChainValidation"),  // SEHOP
                QStringLiteral("DeviceGuard"),
                QStringLiteral("HypervisorEnforcedCodeIntegrity"),
                QStringLiteral("EnableVirtualizationBasedSecurity"),
                QStringLiteral("LsaCfgFlags"),
                QStringLiteral("RunAsPPL"),                  // LSA protection
                QStringLiteral("RestrictAnonymous"),
                QStringLiteral("NoLMHash"),
                QStringLiteral("EnableCertPaddingCheck"),
                QStringLiteral("SharedAccess\\Parameters\\FirewallPolicy"),
                QStringLiteral("EnableFirewall"),
                QStringLiteral("WindowsUpdate"),
            };
            return writesAnyOf(t, kSecurityTargets);
        }

        // Settings read by the OS itself.
        bool touchesKernelSettings(const TweakSpec& t) {
            static const QStringList kKernelTargets = {
                QStringLiteral("bcdedit"),
                QStringLiteral("\\Session Manager"),
                QStringLiteral("\\Control\\PriorityControl"),
                QStringLiteral("Memory Management"),
                QStringLiteral("\\Control\\CrashControl"),
                QStringLiteral("\\Control\\Nsi"),
                QStringLiteral("\\Control\\Power"),
                QStringLiteral("\\Control\\FileSystem"),
                QStringLiteral("\\Control\\GraphicsDrivers"),
                QStringLiteral("\\Control\\DeviceGuard"),
                QStringLiteral("\\Services\\Ndis"),
                QStringLiteral("\\Services\\Tcpip\\Parameters"),
            };
            return writesAnyOf(t, kKernelTargets);
        }

        // Protections Free may never switch off, plus the Policies hive.
        bool disablesWindowsProtection(const TweakSpec& t) {
            static const QStringList kProtections = {
                QStringLiteral("Windows Defender"),
                QStringLiteral("DisableAntiSpyware"),
                QStringLiteral("DisableRealtimeMonitoring"),
                QStringLiteral("MpPreference"),
                QStringLiteral("TamperProtection"),
                QStringLiteral("SmartScreen"),
                QStringLiteral("EnableLUA"),
                QStringLiteral("ConsentPromptBehavior"),
                QStringLiteral("PromptOnSecureDesktop"),
                QStringLiteral("WindowsUpdate"),
                QStringLiteral("WUServer"),
                QStringLiteral("\\Policies\\"),
            };
            return writesAnyOf(t, kProtections);
        }

        // Firmware, NVRAM, drivers and storage controllers.
        bool touchesHardwareSettings(const TweakSpec& t) {
            static const QStringList kHardwareTargets = {
                QStringLiteral("bcdedit"),
                QStringLiteral("SystemStartOptions"),
                QStringLiteral("nvram"),
                QStringLiteral("SCEWIN"),
                QStringLiteral("\\Control\\SafeBoot"),
                QStringLiteral("\\Control\\GraphicsDrivers"),
                QStringLiteral("\\Control\\Class\\{4d36e968"),   // display adapters
                QStringLiteral("\\Control\\Class\\{4d36e97b"),   // storage controllers
                QStringLiteral("\\Services\\storahci"),
                QStringLiteral("\\Services\\stornvme"),
                QStringLiteral("\\Services\\disk"),
                QStringLiteral("\\Services\\iaStorV"),
                QStringLiteral("StorPort"),
                QStringLiteral("DriverStore"),
                QStringLiteral("nvlddmkm"),
            };
            return writesAnyOf(t, kHardwareTargets)
                || t.category == QStringLiteral("bios");
        }

        int entitlementScore(const TweakSpec& t) {
            int score = 0;

            switch (t.impact) {
                case Impact::Critical: score += 3; break;
                case Impact::High:     score += 2; break;
                case Impact::Medium:   score += 1; break;
                case Impact::Low:      break;
            }

            if (!t.reversible)
                score += 2;

            if (t.warnings.size() + t.effects.size() >= 2)
                score += 1;

            // Tweaks with more ops are harder to explain.
            const int opCount = t.registryOps.size() + t.commandOps.size()
                              + t.dynamicOps.size();
            if (opCount > 7)      score += 2;
            else if (opCount >= 3) score += 1;

            if (weakensSecurityFeature(t))
                score += 1;

            if (touchesKernelSettings(t))
                score += 1;

            if (t.requiresReboot)
                score += 1;

            static const QStringList kObscureSubCategoryHints = {
                QStringLiteral("exploit"),
                QStringLiteral("mitigation"),
                QStringLiteral("side-channel"),
                QStringLiteral("virtualization-based"),
                QStringLiteral("dma protection"),
                QStringLiteral("boot security"),
                QStringLiteral("control flow guard"),
                QStringLiteral("attestation"),
            };
            for (const QString& hint : kObscureSubCategoryHints)
                if (t.subCategory.contains(hint, Qt::CaseInsensitive)) {
                    score += 2;
                    break;
                }

            // concern also holds plain usage guidance, so only wording that flags
            // risk or uncertainty counts.
            static const QStringList kObscureConcernHints = {
                QStringLiteral("undocumented"),
                QStringLiteral("placebo"),
                QStringLiteral("unverified"),
                QStringLiteral("no measured"),
                QStringLiteral("no measurable"),
                QStringLiteral("never recommended"),
                QStringLiteral("not verified"),
                QStringLiteral("opt-in only"),
                QStringLiteral("obscure"),
                QStringLiteral("unknown effect"),
                QStringLiteral("not officially"),
                QStringLiteral("no official"),
                QStringLiteral("unconfirmed"),
                QStringLiteral("not part of"),
                QStringLiteral("no published"),
                QStringLiteral("not independently verified"),
                QStringLiteral("needs review"),
                QStringLiteral("needs curator"),
                QStringLiteral("curator review"),
            };
            for (const QString& hint : kObscureConcernHints)
                if (t.concern.contains(hint, Qt::CaseInsensitive)) {
                    score += 2;
                    break;
                }

            return score;
        }

        bool qualifiesForFree(const TweakSpec& t) {
            return t.warnings.isEmpty()
                && t.effects.isEmpty()
                && t.reversible
                && t.impact == Impact::Low
                && !t.requiresReboot
                && !disablesWindowsProtection(t)
                && !touchesHardwareSettings(t)
                && entitlementScore(t) < 5;
        }

        void classifyEntitlement(TweakSpec& t) {
            using vth::models::ProductTier;

            // These override declaredTier and the score.
            if (t.earlyAccess || raisesThermalOrPowerLoad(t)) {
                t.minTier = ProductTier::Ultimate;
                return;
            }

            if (t.declaredTier.has_value()) {
                t.minTier = *t.declaredTier;
                return;
            }

            if (qualifiesForFree(t)) {
                t.minTier = ProductTier::Free;
                return;
            }

            const int score = entitlementScore(t);
            t.minTier = score >= 5 ? ProductTier::Ultimate : ProductTier::Pro;
        }
    }

    const QVector<TweakSpec>& catalog() {
        static const QVector<TweakSpec> c = []() {
            QVector<TweakSpec> v;
            detail::appendActive(v);
            detail::appendOperatorBatch(v);
            detail::appendOperatorGpuPower(v);
            detail::appendOperatorAdvanced(v);
            detail::appendOperatorDebloat(v);

            // the extended set never replaces a spec above with the same id
            QSet<QString> ids;
            for (const TweakSpec& t : v)
                ids.insert(t.id);
            QVector<TweakSpec> extended;
            detail::appendExtended(extended);
            for (TweakSpec& t : extended) {
                if (ids.contains(t.id))
                    continue;
                ids.insert(t.id);
                v.push_back(std::move(t));
            }
            detail::applyRelations(v);

            for (TweakSpec& t : v)
                classifyEntitlement(t);

            const auto translate = [](QString& s) {
                if (!s.isEmpty())
                    s = QCoreApplication::translate("Tweaks", s.toUtf8().constData());
            };
            for (TweakSpec& t : v) {
                translate(t.name);
                translate(t.description);
                translate(t.subCategory);
                for (QString& w : t.warnings)
                    translate(w);
                for (QString& e : t.effects)
                    translate(e);
            }
            return v;
        }();
        return c;
    }

    const TweakSpec* find(const QString& id) {
        static const QHash<QString, const TweakSpec*> index = [] {
            QHash<QString, const TweakSpec*> h;
            const QVector<TweakSpec>& all = catalog();
            h.reserve(all.size());
            for (const TweakSpec& t : all)
                if (!h.contains(t.id))
                    h.insert(t.id, &t);
            return h;
        }();
        return index.value(id, nullptr);
    }

    bool isStateUndetectable(const TweakSpec& spec) {
        if (!spec.registryOps.isEmpty())
            return false;
        for (const DynamicOp& op : spec.dynamicOps)
            if (op.detect)
                return false;
        return true;
    }

    namespace {
        QString protectedServiceIn(const RegistryOp& op);
        bool    targetsAbsentService(const RegistryOp& op,
                                     const vth::services::IRegistryEditor& reg);
    }

    DetectState detect(const TweakSpec& spec, const vth::services::IRegistryEditor& reg) {
        if (isStateUndetectable(spec))
            return DetectState::Unknown;

        int on = 0, unknown = 0, total = 0;
        for (const RegistryOp& op : spec.registryOps) {
            // Apply skips these, so they don't count.
            if (!protectedServiceIn(op).isEmpty() || targetsAbsentService(op, reg))
                continue;
            ++total;
            if (valueEquals(reg.getValue(op.path, op.name), op.appliedValue, op.kind))
                ++on;
        }
        for (const DynamicOp& op : spec.dynamicOps) {
            if (!op.detect) continue;
            ++total;
            switch (op.detect()) {
                case DetectState::On:      ++on;      break;
                case DetectState::Off:     break;
                case DetectState::Unknown: ++unknown; break;
            }
        }

        if (total == 0 || unknown == total)
            return DetectState::Unknown;
        // Partially applied counts as Off.
        return on == total ? DetectState::On : DetectState::Off;
    }

    namespace {
        QString regTarget(const RegistryOp& op) {
            return op.name.isEmpty() ? op.path : (op.path + QLatin1Char('\\') + op.name);
        }

        bool isBootConfigWrite(const CommandOp& op) {
            const QString exe = QFileInfo(op.program).completeBaseName();
            return exe.compare(QStringLiteral("bcdedit"), Qt::CaseInsensitive) == 0;
        }
        const QString& servicesPrefix() {
            static const QString kPrefix =
                QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\");
            return kPrefix;
        }

        // The service whose Start value this op sets, or empty.
        QString startValueService(const RegistryOp& op) {
            if (op.name.compare(QStringLiteral("Start"), Qt::CaseInsensitive) != 0)
                return {};
            if (!op.path.startsWith(servicesPrefix(), Qt::CaseInsensitive))
                return {};
            return op.path.mid(servicesPrefix().size()).section(QLatin1Char('\\'), 0, 0);
        }

        // The protected service this op would disable (Start = 4), or empty.
        // Other Start values are how a revert puts a service back.
        QString protectedServiceIn(const RegistryOp& op) {
            if (op.appliedValue.toUInt() != 4u)
                return {};
            const QString name = startValueService(op);
            return vth::win::services::isProtected(name) ? name : QString();
        }

        // True when the op sets the Start value of a service that isn't installed.
        bool targetsAbsentService(const RegistryOp& op,
                                  const vth::services::IRegistryEditor& reg) {
            const QString name = startValueService(op);
            return !name.isEmpty() && !reg.keyExists(servicesPrefix() + name);
        }

        void record(ApplyOutcome& out, OpFailure::Kind kind, const QString& target) {
            out.ok = false;
            OpFailure f{ kind, target };
            if (kind == OpFailure::Kind::Registry) {
                f.code = vth::services::lastRegistryWriteError();
            } else if (kind == OpFailure::Kind::Command) {
                f.code = t_lastRun.code;
                f.output = t_lastRun.output;
            }
            qCWarning(catTweakEngine).noquote() << "op failed:" << target << "code" << f.code
                                                << f.output.left(300);
            out.failures.push_back(f);
        }
    }

    ApplyOutcome applyVerbose(const TweakSpec& spec, vth::services::IRegistryEditor& reg) {
        ApplyOutcome out;

        // Enforced here as well as in the UI, so no caller can get around it.
        if (!vth::meetsTier(spec.minTier)) {
            qCWarning(catTweakEngine)
                << "refusing tweak above the active tier" << spec.id;
            out.ok = false;
            out.failures.push_back({ OpFailure::Kind::Refused, QStringLiteral("tier") });
            return out;
        }

        // A caller may hold a spec from before it was withdrawn.
        if (isRemotelyDisabled(spec.id)) {
            qCWarning(catTweakEngine)
                << "refusing withdrawn tweak" << spec.id
                << "reason:" << remoteDisableReason(spec.id);
            out.ok = false;
            out.failures.push_back({ OpFailure::Kind::Refused, QStringLiteral("withdrawn") });
            return out;
        }

        int skippedProtected = 0;
        for (const RegistryOp& op : spec.registryOps) {
            if (const QString svc = protectedServiceIn(op); !svc.isEmpty()) {
                qCInfo(catTweakEngine).noquote()
                    << "apply" << spec.id << "- skipped protected service"
                    << svc << ":" << vth::win::services::protectedReason(svc);
                ++skippedProtected;
                continue;
            }
            if (targetsAbsentService(op, reg)) {
                qCInfo(catTweakEngine).noquote()
                    << "apply" << spec.id << "- skipped service not installed:" << op.path;
                ++skippedProtected;
                continue;
            }
            captureRegistryOp(spec.id, op, reg);
            if (!reg.setValue(op.path, op.name, op.appliedValue, op.kind))
                record(out, OpFailure::Kind::Registry, regTarget(op));
        }
        for (const CommandOp& op : spec.commandOps) {
            if (isBootConfigWrite(op) && !vth::win::bcd::ensureBackupBeforeBootWrite()) {
                qCWarning(catTweakEngine).noquote()
                    << "apply" << spec.id
                    << "- refused a boot-configuration write with no store backup";
                record(out, OpFailure::Kind::Command, op.program);
                continue;
            }
            if (!runProcess(op.program, op.applyArgs))
                record(out, OpFailure::Kind::Command, op.program);
        }
        for (const DynamicOp& op : spec.dynamicOps)
            if (op.apply && !op.apply())
                record(out, OpFailure::Kind::Dynamic, op.label);
        // Every op was skipped, so report it as a failure.
        if (skippedProtected > 0 && skippedProtected == spec.registryOps.size()
            && spec.commandOps.isEmpty() && spec.dynamicOps.isEmpty()) {
            out.ok = false;
            out.failures.push_back({ OpFailure::Kind::Refused, QStringLiteral("protected") });
        }
        qCInfo(catTweakEngine).noquote() << "apply" << spec.id
            << (out.ok ? QStringLiteral("ok")
                       : QStringLiteral("failed(%1 op)").arg(out.failures.size()));
        return out;
    }

    ApplyOutcome revertVerbose(const TweakSpec& spec, vth::services::IRegistryEditor& reg) {
        ApplyOutcome out;

        if (!spec.reversible) {
            out.ok = false;
            out.irreversible = true;
            qCWarning(catTweakEngine).noquote()
                << "revert refused (irreversible)" << spec.id;
            return out;
        }

        // Restore captured values. revertValue is the fallback for ops with none.
        const QVector<vth::TweakSnapshot> caps = vth::tweakSnapshots(spec.id);
        QHash<QString, vth::TweakSnapshot> byKey;
        for (const vth::TweakSnapshot& s : caps)
            if (s.kind == vth::SnapshotKind::Registry)
                byKey.insert(capKey(s.hive, s.subkey, s.name), s);

        for (const RegistryOp& op : spec.registryOps) {
            QString hive, subkey;
            splitHiveSubkey(op.path, hive, subkey);
            const auto it = byKey.constFind(capKey(hive, subkey, op.name));

            bool ok;
            if (it != byKey.constEnd())
                ok = restoreFromSnapshot(op, it.value(), reg);
            else if (op.revertValue.has_value())
                ok = reg.setValue(op.path, op.name, *op.revertValue, op.kind);
            else
                ok = reg.deleteValue(op.path, op.name);

            if (!ok)
                record(out, OpFailure::Kind::Registry, regTarget(op));
        }
        for (const CommandOp& op : spec.commandOps)
            if (!runProcess(op.program, op.revertArgs))
                record(out, OpFailure::Kind::Command, op.program);
        for (const DynamicOp& op : spec.dynamicOps)
            if (op.revert && !op.revert())
                record(out, OpFailure::Kind::Dynamic, op.label);

        // Clear them so the next apply captures new values.
        if (!caps.isEmpty() && out.ok)
            vth::clearTweakDomain(spec.id);

        qCInfo(catTweakEngine).noquote() << "revert" << spec.id
            << (out.ok ? QStringLiteral("ok")
                       : QStringLiteral("failed(%1 op)").arg(out.failures.size()));
        return out;
    }

    bool apply(const TweakSpec& spec, vth::services::IRegistryEditor& reg) {
        return applyVerbose(spec, reg).ok;
    }

    bool revert(const TweakSpec& spec, vth::services::IRegistryEditor& reg) {
        return revertVerbose(spec, reg).ok;
    }

    // Remote kill switch. Persisted so a withdrawal survives a restart and holds
    // while offline.
    namespace {
        constexpr char kWithdrawnKey[] = "tweaks/withdrawn";

        QMutex                  g_withdrawnMu;
        QHash<QString, QString> g_withdrawn;
        bool                    g_withdrawnLoaded = false;

        // Caller holds g_withdrawnMu.
        void loadWithdrawnLocked() {
            if (g_withdrawnLoaded)
                return;
            g_withdrawnLoaded = true;   // first, so a failed load isn't retried
            auto* settings = ServiceRegistry::instance().get<Settings>();
            if (!settings)
                return;
            const QString raw =
                settings->value(QString::fromLatin1(kWithdrawnKey)).toString();
            if (raw.isEmpty())
                return;
            const QJsonDocument doc = QJsonDocument::fromJson(raw.toUtf8());
            if (!doc.isObject())
                return;
            const QJsonObject o = doc.object();
            for (auto it = o.constBegin(); it != o.constEnd(); ++it)
                g_withdrawn.insert(it.key(), it.value().toString());
        }
    }

    void setRemotelyDisabled(const QHash<QString, QString>& idToReason) {
        QMutexLocker lock(&g_withdrawnMu);
        g_withdrawnLoaded = true;
        if (g_withdrawn == idToReason)
            return;
        g_withdrawn = idToReason;

        QJsonObject o;
        for (auto it = g_withdrawn.constBegin(); it != g_withdrawn.constEnd(); ++it)
            o.insert(it.key(), it.value());

        if (auto* settings = ServiceRegistry::instance().get<Settings>()) {
            settings->setValue(
                QString::fromLatin1(kWithdrawnKey),
                QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact)));
        }
        qCInfo(catTweakEngine) << "withdrawn tweak set updated:"
                               << g_withdrawn.size() << "entries";
    }

    bool isRemotelyDisabled(const QString& id) {
        QMutexLocker lock(&g_withdrawnMu);
        loadWithdrawnLocked();
        return g_withdrawn.contains(id);
    }

    QString remoteDisableReason(const QString& id) {
        QMutexLocker lock(&g_withdrawnMu);
        loadWithdrawnLocked();
        return g_withdrawn.value(id);
    }

    QString impactLabel(Impact i) {
        switch (i) {
            case Impact::Low:      return QStringLiteral("Low");
            case Impact::Medium:   return QStringLiteral("Medium");
            case Impact::High:     return QStringLiteral("High");
            case Impact::Critical: return QStringLiteral("Critical");
        }
        return {};
    }

    QString hardwareVendorKey(const TweakSpec& t) {
        static const QStringList kNvidia = {
            QStringLiteral("NVIDIA"),
            QStringLiteral("nvlddmkm"),
            QStringLiteral("nvidia-smi"),
        };
        static const QStringList kAmd = {
            QStringLiteral("amdkmdag"),
            QStringLiteral("atikmdag"),
            QStringLiteral("Radeon"),
            QStringLiteral("amdgpu"),
        };

        if (t.subCategory.compare(QStringLiteral("NVIDIA"), Qt::CaseInsensitive) == 0
            || t.id.startsWith(QStringLiteral("gpu.nvidia."), Qt::CaseInsensitive)) {
            return QStringLiteral("nvidia");
        }
        if (t.subCategory.compare(QStringLiteral("AMD"), Qt::CaseInsensitive) == 0
            || t.id.startsWith(QStringLiteral("gpu.amd."), Qt::CaseInsensitive)) {
            return QStringLiteral("amd");
        }
        if (writesAnyOf(t, kNvidia))
            return QStringLiteral("nvidia");
        if (writesAnyOf(t, kAmd))
            return QStringLiteral("amd");
        return {};
    }

    bool needsUserConfirmation(const TweakSpec& spec) {
        return !spec.reversible || spec.askFirst;
    }

    bool raisesThermalOrPowerLoad(const TweakSpec& spec) {
        static const char* kMarkers[] = {
            "overclock", "voltage", "vcore", "undervolt",
            "cstate", "c-state", "c_state", "idle-state", "idlestate",
            "min-processor-state", "processor-state-100", "throttlestop",
            "power-limit", "powerlimit", "tdp", "pl1", "pl2",
            "turbo-unlock", "unlock-turbo", "thermal-limit", "disable-thermal",
            "core-parking-off", "disable-speedstep", "speedshift-off",
        };
        for (const char* m : kMarkers) {
            const QLatin1String needle(m);
            if (spec.id.contains(needle, Qt::CaseInsensitive)
                || spec.subCategory.contains(needle, Qt::CaseInsensitive))
                return true;
        }

        if (spec.category.compare(QStringLiteral("overclock"),
                                  Qt::CaseInsensitive) == 0)
            return true;

        static const char* kLoadPhrases[] = {
            "power draw", "power consumption", "idle power", "more power",
            "fan noise", "thermal throttle", "temperature", "temps",
            "max clocks", "maximum clocks", "never enters idle",
            "never enter idle", "stays at 100", "100% minimum",
            "minimum state", "out of idle c-states", "battery life",
        };
        for (const char* p : kLoadPhrases) {
            const QLatin1String needle(p);
            if (spec.concern.contains(needle, Qt::CaseInsensitive))
                return true;
            for (const QString& w : spec.warnings)
                if (w.contains(needle, Qt::CaseInsensitive))
                    return true;
            for (const QString& e : spec.effects)
                if (e.contains(needle, Qt::CaseInsensitive))
                    return true;
        }
        return false;
    }

}

namespace vth::tweaks
{
namespace
{
        bool touchesKernelSurface(const TweakSpec& t) {
            static const QStringList kKernelSurfaces = {
                QStringLiteral("\\Session Manager"),
                QStringLiteral("\\Control\\PriorityControl"),
                QStringLiteral("Memory Management"),
                QStringLiteral("\\Control\\CrashControl"),
                QStringLiteral("\\Control\\Nsi"),
                QStringLiteral("\\Control\\Power"),
                QStringLiteral("\\Control\\FileSystem"),
                QStringLiteral("\\Control\\GraphicsDrivers"),
                QStringLiteral("\\Control\\DeviceGuard"),
                QStringLiteral("bcdedit"),
                QStringLiteral("\\Services\\Ndis"),
                QStringLiteral("\\Services\\Tcpip\\Parameters"),
            };
            for (const QString& s : kKernelSurfaces)
                if (writesAnything(t, s))
                    return true;
            return false;
        }
        bool touchesHardwareSurface(const TweakSpec& t) {
            static const QStringList kHardwareSurfaces = {
                QStringLiteral("bcdedit"),
                QStringLiteral("nvram"),
                QStringLiteral("SCEWIN"),
                QStringLiteral("SystemStartOptions"),
                QStringLiteral("\\Control\\SafeBoot"),
                QStringLiteral("\\Control\\GraphicsDrivers"),
                QStringLiteral("\\Control\\Class\\{4d36e968"),
                QStringLiteral("\\Control\\Class\\{4d36e97b"),
                QStringLiteral("\\Services\\storahci"),
                QStringLiteral("\\Services\\stornvme"),
                QStringLiteral("\\Services\\disk"),
                QStringLiteral("\\Services\\iaStorV"),
                QStringLiteral("StorPort"),
                QStringLiteral("DriverStore"),
                QStringLiteral("nvlddmkm"),
            };
            for (const QString& s : kHardwareSurfaces)
                if (writesAnything(t, s))
                    return true;
            return t.category == QStringLiteral("bios");
        }
}
}

