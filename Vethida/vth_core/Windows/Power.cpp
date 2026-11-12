#include "Power.h"

#include "../Logger.h"
#include "../Runtime/Paths.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QProcess>
#include <QRegularExpression>
#include <QSaveFile>
#include <QScopeGuard>
#include <QStringLiteral>
#include <QUuid>

#include <string>

#include <windows.h>
#include <objbase.h>
#include <powrprof.h>
#include "../Runtime/Tier.h"

#pragma comment(lib, "powrprof.lib")
#pragma comment(lib, "ole32.lib")

namespace vth::win::power
{
    namespace
    {
        VTH_DEFINE_LOG_CATEGORY(catPower, "vth.power")

        const GUID kSubgroupProcessor = { 0x54533251, 0x82be, 0x4824,
            { 0x96, 0xc1, 0x47, 0xb6, 0x0b, 0x74, 0x0d, 0x00 } };
        const GUID kSettingThrottleMin = { 0x893dee8e, 0x2bef, 0x41e0,
            { 0x89, 0xc6, 0xb5, 0x5d, 0x09, 0x29, 0x96, 0x4c } };
        const GUID kSettingThrottleMax = { 0xbc5038f7, 0x23e0, 0x4960,
            { 0x96, 0xda, 0x33, 0xab, 0xaf, 0x59, 0x35, 0xec } };
        const GUID kSettingMinCores = { 0x0cc5b647, 0xc1df, 0x4637,
            { 0x89, 0x1a, 0xde, 0xc3, 0x5c, 0x31, 0x85, 0x83 } };

        // Absolute, because this runs elevated and PATH could be hijacked.
        QString powercfgPath() {
            wchar_t dir[MAX_PATH] = {};
            const UINT n = GetSystemDirectoryW(dir, MAX_PATH);
            if (n == 0 || n >= MAX_PATH)
                return QStringLiteral("C:\\Windows\\System32\\powercfg.exe");
            return QString::fromWCharArray(dir, static_cast<int>(n))
                 + QStringLiteral("\\powercfg.exe");
        }

        struct Run {
            bool    started  = false;
            bool    finished = false;
            int     exitCode = -1;
            QString output;     // stdout and stderr, trimmed
        };

        // powercfg writes in the console code page, not the ANSI one.
        QString decodeConsole(const QByteArray& bytes) {
            const UINT cp = GetOEMCP();
            const int need = MultiByteToWideChar(cp, 0, bytes.constData(),
                                                 static_cast<int>(bytes.size()), nullptr, 0);
            if (need <= 0)
                return QString::fromLocal8Bit(bytes);
            std::wstring wide(static_cast<size_t>(need), L' ');
            MultiByteToWideChar(cp, 0, bytes.constData(), static_cast<int>(bytes.size()),
                                wide.data(), need);
            return QString::fromStdWString(wide);
        }

        Run run(const QStringList& args, int timeoutMs = 20000) {
            Run r;
            QProcess proc;
            proc.setProcessChannelMode(QProcess::MergedChannels);
            proc.start(powercfgPath(), args);
            if (!proc.waitForStarted(5000)) {
                qCWarning(catPower).noquote() << "powercfg could not start:" << proc.errorString();
                return r;
            }
            r.started = true;
            if (!proc.waitForFinished(timeoutMs)) {
                proc.kill();
                proc.waitForFinished(2000);
                qCWarning(catPower) << "powercfg" << args << "timed out";
                return r;
            }
            r.finished = proc.exitStatus() == QProcess::NormalExit;
            r.exitCode = proc.exitCode();
            r.output   = decodeConsole(proc.readAll()).trimmed();
            if (!r.finished || r.exitCode != 0)
                qCWarning(catPower).noquote()
                    << "powercfg" << args.join(QChar::fromLatin1(' ')) << "exit" << r.exitCode
                    << "output:" << r.output;
            return r;
        }

        bool succeeded(const Run& r) { return r.started && r.finished && r.exitCode == 0; }

        QString normalizeGuid(const QString& s) {
            QString g = s.trimmed().toLower();
            if (g.startsWith(QLatin1Char('{')) && g.endsWith(QLatin1Char('}')))
                g = g.mid(1, g.size() - 2);
            return g;
        }

        bool toGuid(const QString& text, GUID& out) {
            const QString braced = text.startsWith(QLatin1Char('{'))
                                 ? text : QLatin1Char('{') + text + QLatin1Char('}');
            return SUCCEEDED(IIDFromString(reinterpret_cast<LPCOLESTR>(braced.utf16()), &out));
        }

        QString fromGuid(const GUID& g) {
            OLECHAR buf[64]{};
            StringFromGUID2(g, buf, 64);
            return normalizeGuid(QString::fromWCharArray(buf));
        }

        QString extractGuidFromOutput(const QString& output) {
            static const QRegularExpression re(
                QStringLiteral("([0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-"
                               "[0-9a-fA-F]{4}-[0-9a-fA-F]{12})"));
            const auto m = re.match(output);
            if (!m.hasMatch()) return {};
            return normalizeGuid(m.captured(1));
        }

        enum class ServiceState { Running, Stopped, Disabled, Unknown };

        ServiceState powerServiceState() {
            SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
            if (!scm) return ServiceState::Unknown;
            SC_HANDLE svc = OpenServiceW(scm, L"Power", SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG);
            ServiceState state = ServiceState::Unknown;
            if (svc) {
                SERVICE_STATUS st{};
                if (QueryServiceStatus(svc, &st))
                    state = st.dwCurrentState == SERVICE_RUNNING ? ServiceState::Running
                                                                 : ServiceState::Stopped;
                DWORD need = 0;
                QueryServiceConfigW(svc, nullptr, 0, &need);
                if (need > 0) {
                    QByteArray buf(static_cast<int>(need), '\0');
                    auto* cfg = reinterpret_cast<QUERY_SERVICE_CONFIGW*>(buf.data());
                    if (QueryServiceConfigW(svc, cfg, need, &need)
                        && cfg->dwStartType == SERVICE_DISABLED
                        && state != ServiceState::Running)
                        state = ServiceState::Disabled;
                }
                CloseServiceHandle(svc);
            }
            CloseServiceHandle(scm);
            return state;
        }

        // A policy that sets the active plan undoes any change on the next refresh.
        bool planPinnedByPolicy() {
            HKEY key = nullptr;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                              L"SOFTWARE\\Policies\\Microsoft\\Power\\PowerSettings",
                              0, KEY_READ, &key) != ERROR_SUCCESS)
                return false;
            const bool pinned = RegQueryValueExW(key, L"ActivePowerScheme", nullptr,
                                                 nullptr, nullptr, nullptr) == ERROR_SUCCESS;
            RegCloseKey(key);
            return pinned;
        }

        // Most specific cause first.
        QString explainFailure(const Run& r, const QString& displayName) {
            if (!r.started) {
                return QFileInfo::exists(powercfgPath())
                    ? QObject::tr("Windows would not start its power settings tool. Security "
                                  "software may be blocking it; allow it and try %1 again.")
                          .arg(displayName)
                    : QObject::tr("Windows' power settings tool is missing from this PC. "
                                  "Use \"Run system file check\" in System > Fixes to restore it.");
            }
            switch (powerServiceState()) {
            case ServiceState::Disabled:
                return QObject::tr("The Windows Power service is disabled, so no power plan "
                                   "can be changed. Switch it on with \"Restore the Power "
                                   "service\" in System > Fixes, then restart.");
            case ServiceState::Stopped:
                return QObject::tr("The Windows Power service is not running, so no power "
                                   "plan can be changed. Restart the PC, or run \"Restore the "
                                   "Power service\" in System > Fixes.");
            default:
                break;
            }
            if (!r.finished)
                return QObject::tr("Windows did not answer within 20 seconds while applying "
                                   "%1. Try again in a moment.").arg(displayName);
            if (planPinnedByPolicy())
                return QObject::tr("A Group Policy on this PC chooses the power plan, so "
                                   "Windows will not switch to %1. Remove the policy, or ask "
                                   "whoever manages this PC.").arg(displayName);
            const QString low = r.output.toLower();
            if (low.contains(QLatin1String("access")) && low.contains(QLatin1String("denied")))
                return QObject::tr("Windows refused the change to %1. Security software "
                                   "or the PC maker's power app is locking the power "
                                   "settings; close it and try again.").arg(displayName);
            const QString first = r.output.section(QLatin1Char('\n'), 0, 0).trimmed();
            return first.isEmpty()
                ? QObject::tr("Windows could not apply %1.").arg(displayName)
                : QObject::tr("Windows could not apply %1: %2").arg(displayName, first);
        }

        // Windows won't delete the active scheme, so switch to another one first.
        QString standInFor(const QString& target) {
            const QString balanced = WellKnownPlans::balanced();
            if (balanced.compare(target, Qt::CaseInsensitive) != 0
                && !schemeName(balanced).isEmpty())
                return balanced;
            for (const Plan& p : enumeratePlans())
                if (p.guid.compare(target, Qt::CaseInsensitive) != 0)
                    return p.guid;
            return {};
        }

        PlanOutcome verifyActive(const QString& guid, const QString& displayName) {
            PlanOutcome out;
            out.guid = guid;
            const ActiveScheme now = readActiveScheme();
            if (now.guid.compare(guid, Qt::CaseInsensitive) == 0) {
                out.ok = true;
                out.message = QObject::tr("%1 is now your power plan.").arg(displayName);
                return out;
            }
            out.message = planPinnedByPolicy()
                ? QObject::tr("A Group Policy on this PC chooses the power plan, so Windows "
                              "switched away from %1 straight after it was set.").arg(displayName)
                : QObject::tr("Windows switched away from %1 straight after it was set. A "
                              "laptop maker's power app or another system tool is probably "
                              "managing the plan; close it and try again.").arg(displayName);
            return out;
        }
    }

    QString WellKnownPlans::balanced() {
        return QStringLiteral("381b4222-f694-41f0-9685-ff5bb260df2e");
    }

    QString WellKnownPlans::highPerformance() {
        return QStringLiteral("8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c");
    }

    QVector<Plan> enumeratePlans() {
        QVector<Plan> out;

        const Run listed = run({QStringLiteral("/list")});
        if (!succeeded(listed))
            return out;

        // Each plan line: "Power Scheme GUID: <guid>  (<name>) [*]"
        static const QRegularExpression re(
            QStringLiteral("GUID:\\s*([0-9a-fA-F-]+)\\s*\\((.+)\\)(\\s*\\*)?"));

        auto it = re.globalMatch(listed.output);
        while (it.hasNext()) {
            const auto m = it.next();
            Plan p;
            p.guid   = normalizeGuid(m.captured(1));
            p.name   = m.captured(2).trimmed();
            p.active = !m.captured(3).isEmpty();
            out.append(std::move(p));
        }
        return out;
    }

    bool setActivePlan(const QString& guid) {
        if (guid.isEmpty()) return false;
        return succeeded(run({QStringLiteral("/setactive"), guid}));
    }

    ProcessorPolicy readProcessorPolicy(const QString& guid) {
        ProcessorPolicy p;

        GUID scheme{};
        if (!toGuid(guid, scheme))
            return p;

        DWORD value = 0;
        if (PowerReadACValueIndex(nullptr, &scheme, &kSubgroupProcessor,
                                  &kSettingThrottleMin, &value) == ERROR_SUCCESS) {
            p.minStatePercent = static_cast<int>(value);
            p.valid = true;
        }
        if (PowerReadACValueIndex(nullptr, &scheme, &kSubgroupProcessor,
                                  &kSettingThrottleMax, &value) == ERROR_SUCCESS) {
            p.maxStatePercent = static_cast<int>(value);
            p.valid = true;
        }
        // Missing if the processor driver doesn't expose core parking.
        if (PowerReadACValueIndex(nullptr, &scheme, &kSubgroupProcessor,
                                  &kSettingMinCores, &value) == ERROR_SUCCESS) {
            p.minCoresPercent = static_cast<int>(value);
        }
        return p;
    }

    ActiveScheme readActiveScheme() {
        ActiveScheme a;
        GUID* active = nullptr;
        if (PowerGetActiveScheme(nullptr, &active) != ERROR_SUCCESS || !active)
            return a;
        a.guid = fromGuid(*active);
        LocalFree(active);
        a.name = schemeName(a.guid);
        return a;
    }

    QString schemeName(const QString& guid) {
        GUID g{};
        if (guid.isEmpty() || !toGuid(guid, g))
            return {};
        DWORD size = 0;
        if (PowerReadFriendlyName(nullptr, &g, nullptr, nullptr, nullptr, &size) != ERROR_SUCCESS
            || size == 0)
            return {};
        QByteArray buf(static_cast<int>(size), '\0');
        if (PowerReadFriendlyName(nullptr, &g, nullptr, nullptr,
                                  reinterpret_cast<UCHAR*>(buf.data()), &size) != ERROR_SUCCESS)
            return {};
        return QString::fromWCharArray(reinterpret_cast<const wchar_t*>(buf.constData())).trimmed();
    }

    PlanOutcome activate(const QString& guid, const QString& displayName) {
        PlanOutcome out;
        out.guid = guid;
        if (schemeName(guid).isEmpty()) {
            out.message = QObject::tr("%1 is no longer installed on this PC.").arg(displayName);
            return out;
        }
        const Run set = run({QStringLiteral("/setactive"), guid});
        if (!succeeded(set)) {
            out.message = explainFailure(set, displayName);
            return out;
        }
        return verifyActive(guid, displayName);
    }

    PlanOutcome installAndActivate(const QByteArray& planFile, const QString& schemeGuid,
                                   const QString& wantedName, const QString& displayName) {
        PlanOutcome out;
        out.guid = schemeGuid;

        if (planFile.isEmpty()) {
            out.message = QObject::tr("The %1 plan file is missing from this copy of Vethida. "
                                      "Reinstall Vethida to restore it.").arg(displayName);
            return out;
        }

        // Not %TEMP%: another program could swap the file before the elevated import.
        QString stageDir = vth::paths::protectedData() + QStringLiteral("/tmp");
        if (!QDir(stageDir).exists() && !QDir().mkpath(stageDir))
            stageDir = vth::paths::userCache();
        const QString staged = QDir(stageDir).filePath(
            QStringLiteral("vethida-plan-%1.pow").arg(QUuid::createUuid().toString(QUuid::Id128)));
        {
            QSaveFile f(staged);
            if (!f.open(QIODevice::WriteOnly) || f.write(planFile) != planFile.size()
                || !f.commit()) {
                out.message = QObject::tr("Vethida could not write a temporary file to install "
                                          "%1. Check that your drive is not full.").arg(displayName);
                return out;
            }
        }
        const auto cleanup = qScopeGuard([&staged] { QFile::remove(staged); });

        // Older copies with this name, removed once the new one is active.
        QStringList older;
        for (const Plan& p : enumeratePlans())
            if (p.name.compare(wantedName, Qt::CaseInsensitive) == 0
                && p.guid.compare(schemeGuid, Qt::CaseInsensitive) != 0)
                older << p.guid;

        if (!schemeName(schemeGuid).isEmpty()) {
            if (readActiveScheme().guid.compare(schemeGuid, Qt::CaseInsensitive) == 0) {
                const QString stand = standInFor(schemeGuid);
                if (!stand.isEmpty())
                    run({QStringLiteral("/setactive"), stand});
            }
            run({QStringLiteral("/delete"), schemeGuid});
        }

        QString installed = schemeGuid;
        Run imp = run({QStringLiteral("/import"), QDir::toNativeSeparators(staged), schemeGuid});
        if (!succeeded(imp) && !schemeName(schemeGuid).isEmpty()) {
            // The old copy couldn't be deleted, so import beside it.
            imp = run({QStringLiteral("/import"), QDir::toNativeSeparators(staged)});
            installed = extractGuidFromOutput(imp.output);
        }
        if (!succeeded(imp) || installed.isEmpty() || schemeName(installed).isEmpty()) {
            out.message = explainFailure(imp, displayName);
            return out;
        }

        const Run set = run({QStringLiteral("/setactive"), installed});
        if (!succeeded(set)) {
            out.message = explainFailure(set, displayName);
            return out;
        }

        out = verifyActive(installed, displayName);
        if (out.ok)
            for (const QString& g : older)
                run({QStringLiteral("/delete"), g});
        return out;
    }
}

namespace vth::win::power
{
namespace
{
        const GUID kSubgroupVideo  = { 0x7516b95f, 0xf776, 0x4464,
            { 0x8c, 0x53, 0x06, 0x16, 0x7f, 0x40, 0xcc, 0x99 } };
        const GUID kSettingMonitor = { 0x3c0bc021, 0xc8a8, 0x4e07,
            { 0xa9, 0x73, 0x6b, 0x14, 0xcb, 0xcb, 0x2b, 0x7e } };
        const GUID kSubgroupSleep  = { 0x238c9fa8, 0x0aad, 0x41ed,
            { 0x83, 0xf4, 0x97, 0xbe, 0x24, 0x2c, 0x8f, 0x20 } };
        const GUID kSettingStandby = { 0x29f6c1db, 0x86da, 0x48c5,
            { 0x9f, 0xdb, 0xf2, 0xb6, 0x7b, 0x1f, 0x44, 0xda } };
        int secondsToMinutesClamped(DWORD seconds) noexcept {
            if (seconds == 0) return 0;
            const DWORD minutes = seconds / 60;
            return static_cast<int>(minutes);
        }
        Result mapPowercfgExit(int code) noexcept {
            if (code == 0) return Result::Ok;
            return Result::OtherError;
        }
        Result runPowercfg(const QStringList& args) {
            const Run r = run(args);
            if (!r.started || !r.finished)
                return Result::OtherError;
            return mapPowercfgExit(r.exitCode);
        }
}
}

namespace vth::win::power
{
    QString resultLabel(Result r) {
        switch (r) {
            case Result::Ok:           return QStringLiteral("Change applied.");
            case Result::AccessDenied: return QStringLiteral(
                "Windows refused the change. Security software or the PC maker's "
                "power app is locking the power settings.");
            case Result::NotFound:     return QStringLiteral("Power plan not found.");
            case Result::OtherError:   return QStringLiteral("Windows could not change the power settings.");
        }
        return {};
    }
    QString activePlanGuid() {
        GUID* guid = nullptr;
        if (PowerGetActiveScheme(nullptr, &guid) != ERROR_SUCCESS || !guid) {
            return {};
        }
        OLECHAR buf[64]{};
        StringFromGUID2(*guid, buf, 64);
        QString s = QString::fromWCharArray(buf);
        LocalFree(guid);
        return normalizeGuid(s);
    }
    Timeouts readTimeouts() {
        Timeouts t;
        GUID* active = nullptr;
        if (PowerGetActiveScheme(nullptr, &active) != ERROR_SUCCESS || !active) {
            return t;
        }
        DWORD value = 0;
        if (PowerReadACValueIndex(nullptr, active, &kSubgroupVideo,
                                  &kSettingMonitor, &value) == ERROR_SUCCESS) {
            t.monitorAc = secondsToMinutesClamped(value);
        }
        if (PowerReadDCValueIndex(nullptr, active, &kSubgroupVideo,
                                  &kSettingMonitor, &value) == ERROR_SUCCESS) {
            t.monitorDc = secondsToMinutesClamped(value);
        }
        if (PowerReadACValueIndex(nullptr, active, &kSubgroupSleep,
                                  &kSettingStandby, &value) == ERROR_SUCCESS) {
            t.sleepAc = secondsToMinutesClamped(value);
        }
        if (PowerReadDCValueIndex(nullptr, active, &kSubgroupSleep,
                                  &kSettingStandby, &value) == ERROR_SUCCESS) {
            t.sleepDc = secondsToMinutesClamped(value);
        }
        LocalFree(active);
        return t;
    }
    Result writeTimeouts(const Timeouts& t) {
        Result worst = Result::Ok;
        auto upgrade = [&](Result r) {
            if (r == Result::Ok) return;
            if (worst == Result::Ok) worst = r;
            if (r == Result::AccessDenied) worst = Result::AccessDenied;
        };
        upgrade(runPowercfg({QStringLiteral("/change"),
                             QStringLiteral("monitor-timeout-ac"),
                             QString::number(t.monitorAc)}));
        upgrade(runPowercfg({QStringLiteral("/change"),
                             QStringLiteral("monitor-timeout-dc"),
                             QString::number(t.monitorDc)}));
        upgrade(runPowercfg({QStringLiteral("/change"),
                             QStringLiteral("standby-timeout-ac"),
                             QString::number(t.sleepAc)}));
        upgrade(runPowercfg({QStringLiteral("/change"),
                             QStringLiteral("standby-timeout-dc"),
                             QString::number(t.sleepDc)}));
        return worst;
    }
    bool hasBattery() {
        SYSTEM_POWER_STATUS s{};
        if (!GetSystemPowerStatus(&s)) return false;
        return s.BatteryFlag != 128;
    }
    QString WellKnownPlans::powerSaver() {
        return QStringLiteral("a1841308-3541-4fab-bc81-f71556f20b4a");
    }
    QString WellKnownPlans::ultimatePerformance() {
        return QStringLiteral("e9a42b02-d5df-448d-aa00-03f14749eb61");
    }
    std::optional<QString> duplicatePlan(const QString& sourceGuid,
                                         const QString& newName) {
        if (sourceGuid.isEmpty()) return std::nullopt;
        const Run dup = run({QStringLiteral("/duplicatescheme"), sourceGuid});
        if (!succeeded(dup))
            return std::nullopt;
        const QString& out = dup.output;
        const QString newGuid = extractGuidFromOutput(out);
        if (newGuid.isEmpty()) {
            qCWarning(catPower).noquote()
                << "powercfg /duplicatescheme returned no GUID; output:" << out.trimmed();
            return std::nullopt;
        }
        if (!newName.isEmpty()) {
            const Result rn = runPowercfg({QStringLiteral("/changename"),
                                           newGuid, newName});
            if (rn != Result::Ok) {
                qCWarning(catPower) << "powercfg /changename failed for" << newGuid;
            }
        }
        return newGuid;
    }
    std::optional<QString> importPlan(const QString& filePath) {
        if (!vth::meetsTier(vth::models::ProductTier::Pro)) return std::nullopt;
        if (filePath.isEmpty()) return std::nullopt;
        const Run imp = run({QStringLiteral("/import"), QDir::toNativeSeparators(filePath)});
        if (!succeeded(imp))
            return std::nullopt;
        const QString& out = imp.output;
        const QString newGuid = extractGuidFromOutput(out);
        if (newGuid.isEmpty()) {
            qCWarning(catPower).noquote()
                << "powercfg /import returned no GUID; output:" << out.trimmed();
            return std::nullopt;
        }
        return newGuid;
    }
    Result exportPlan(const QString& guid, const QString& filePath) {
        if (guid.isEmpty())     return Result::NotFound;
        if (filePath.isEmpty()) return Result::OtherError;
        return runPowercfg({QStringLiteral("/export"), QDir::toNativeSeparators(filePath), guid});
    }
}
