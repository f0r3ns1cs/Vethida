#include "SystemRestore.h"

#include "Windows/Privileges.h"

#include <QProcess>
#include <QStringLiteral>
#include <QThread>
#include <QTimeZone>

#include <algorithm>
#include <string>

#include <windows.h>
#include <reason.h>
#include <comdef.h>
#include <wbemidl.h>
#include <QRegularExpression>

#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

namespace vth::win::sysrestore
{
    namespace
    {
        // From srrestoreptapi.h, which isn't in every SDK. Windows expects these
        // structs packed.
        constexpr DWORD kBeginSystemChange = 100;
        constexpr DWORD kEndSystemChange = 101;
        constexpr DWORD kModifySettings = 12;

#pragma pack(push, 1)
        struct VTH_RESTOREPOINTINFOW {
            DWORD   dwEventType;
            DWORD   dwRestorePtType;
            INT64   llSequenceNumber;
            WCHAR   szDescription[256];
        };

        struct VTH_STATEMGRSTATUS {
            DWORD nStatus;
            INT64 llSequenceNumber;
        };
#pragma pack(pop)

        using SRSetRestorePointWFn = BOOL(WINAPI*)(
            VTH_RESTOREPOINTINFOW*, VTH_STATEMGRSTATUS*);

        // Absolute, because this runs elevated and PATH could be hijacked.
        QString powershellPath() {
            wchar_t dir[MAX_PATH] = {};
            const UINT n = GetSystemDirectoryW(dir, MAX_PATH);
            if (n == 0 || n >= MAX_PATH) return QStringLiteral("powershell.exe");
            return QString::fromWCharArray(dir)
                 + QStringLiteral("\\WindowsPowerShell\\v1.0\\powershell.exe");
        }

        bool runPowerShell(const QString& script, QString* out, QString* err,
                           int timeoutMs = 20000) {
            QProcess p;
            p.setProgram(powershellPath());
            p.setArguments({
                QStringLiteral("-NoProfile"),
                QStringLiteral("-NonInteractive"),
                QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"),
                QStringLiteral("-Command"), script,
            });
            p.start();
            if (!p.waitForStarted(5000)) {
                if (err) *err = QStringLiteral("Windows System Restore did not respond.");
                return false;
            }
            if (!p.waitForFinished(timeoutMs)) {
                p.kill();
                p.waitForFinished(2000);
                if (err) *err = QStringLiteral("Windows System Restore did not respond.");
                return false;
            }
            if (out) *out = QString::fromLocal8Bit(p.readAllStandardOutput());
            if (err) *err = QString::fromLocal8Bit(p.readAllStandardError());
            return p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0;
        }

        // WMI datetime, yyyymmddHHMMSS.ffffff+UUU. CreationTime is in UTC.
        QDateTime parseWmiDate(const QString& raw) {
            if (raw.size() < 14) return {};
            bool ok = false;
            const int y = raw.left(4).toInt(&ok);          if (!ok) return {};
            const int m = raw.mid(4, 2).toInt(&ok);        if (!ok) return {};
            const int d = raw.mid(6, 2).toInt(&ok);        if (!ok) return {};
            const int H = raw.mid(8, 2).toInt(&ok);        if (!ok) return {};
            const int M = raw.mid(10, 2).toInt(&ok);       if (!ok) return {};
            const int S = raw.mid(12, 2).toInt(&ok);       if (!ok) return {};
            return QDateTime(QDate(y, m, d), QTime(H, M, S), QTimeZone::UTC);
        }

        HRESULT connectToRestoreNamespace(IWbemLocator*& locator, IWbemServices*& services) {
            CoInitializeSecurity(nullptr, -1, nullptr, nullptr, RPC_C_AUTHN_LEVEL_DEFAULT,
                                 RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE, nullptr);

            HRESULT hr = CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER,
                                          IID_IWbemLocator, reinterpret_cast<void**>(&locator));
            if (FAILED(hr))
                return hr;

            BSTR ns = SysAllocString(L"ROOT\\DEFAULT");
            hr = locator->ConnectServer(ns, nullptr, nullptr, nullptr, 0, nullptr, nullptr, &services);
            SysFreeString(ns);
            if (FAILED(hr))
                return hr;

            CoSetProxyBlanket(services, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr,
                              RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);
            return hr;
        }

        EnumerateResult enumerateWmi() {
            EnumerateResult r;
            const bool ownsCom = SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED));

            IWbemLocator* locator = nullptr;
            IWbemServices* services = nullptr;
            IEnumWbemClassObject* rows = nullptr;
            HRESULT hr = connectToRestoreNamespace(locator, services);
            if (SUCCEEDED(hr)) {
                BSTR lang = SysAllocString(L"WQL");
                BSTR query = SysAllocString(L"SELECT SequenceNumber, Description, CreationTime FROM SystemRestore");
                hr = services->ExecQuery(lang, query, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                                         nullptr, &rows);
                SysFreeString(lang);
                SysFreeString(query);
            }
            if (SUCCEEDED(hr) && rows) {
                r.ok = true;
                for (;;) {
                    IWbemClassObject* obj = nullptr;
                    ULONG got = 0;
                    if (FAILED(rows->Next(15000, 1, &obj, &got)) || got == 0)
                        break;
                    RestorePoint rp;
                    VARIANT v;
                    VariantInit(&v);
                    if (SUCCEEDED(obj->Get(L"SequenceNumber", 0, &v, nullptr, nullptr)))
                        rp.sequenceNumber = (v.vt == VT_I4 || v.vt == VT_UI4) ? static_cast<int>(v.lVal) : 0;
                    VariantClear(&v);
                    if (SUCCEEDED(obj->Get(L"Description", 0, &v, nullptr, nullptr)) && v.vt == VT_BSTR)
                        rp.description = QString::fromWCharArray(v.bstrVal);
                    VariantClear(&v);
                    if (SUCCEEDED(obj->Get(L"CreationTime", 0, &v, nullptr, nullptr)) && v.vt == VT_BSTR)
                        rp.createdUtc = parseWmiDate(QString::fromWCharArray(v.bstrVal));
                    VariantClear(&v);
                    rp.isVethida = rp.description.contains(QStringLiteral("Vethida"), Qt::CaseInsensitive);
                    obj->Release();
                    if (rp.sequenceNumber > 0)
                        r.points.append(rp);
                }
            } else {
                r.error = QStringLiteral("Windows would not list the restore points (0x%1).")
                              .arg(static_cast<quint32>(hr), 8, 16, QLatin1Char('0'));
            }
            if (rows)     rows->Release();
            if (services) services->Release();
            if (locator)  locator->Release();
            if (ownsCom)
                CoUninitialize();
            return r;
        }

        int newestSequence(const QVector<RestorePoint>& points) {
            int best = 0;
            for (const RestorePoint& p : points)
                best = qMax(best, p.sequenceNumber);
            return best;
        }

        int serviceStartType(const wchar_t* name) {
            const std::wstring key = std::wstring(L"SYSTEM\\CurrentControlSet\\Services\\") + name;
            DWORD v = 0, size = sizeof(v);
            if (RegGetValueW(HKEY_LOCAL_MACHINE, key.c_str(), L"Start", RRF_RT_REG_DWORD,
                             nullptr, &v, &size) != ERROR_SUCCESS)
                return -1;
            return static_cast<int>(v);
        }

        bool policyDisablesRestore() {
            DWORD v = 0, size = sizeof(v);
            return RegGetValueW(HKEY_LOCAL_MACHINE,
                                L"SOFTWARE\\Policies\\Microsoft\\Windows NT\\SystemRestore",
                                L"DisableSR", RRF_RT_REG_DWORD, nullptr, &v, &size) == ERROR_SUCCESS
                && v == 1;
        }

        quint64 systemDriveFreeBytes() {
            wchar_t dir[MAX_PATH] = {};
            if (GetWindowsDirectoryW(dir, MAX_PATH) < 3)
                return 0;
            const wchar_t root[] = { dir[0], L':', L'\\', 0 };
            ULARGE_INTEGER freeBytes{};
            if (!GetDiskFreeSpaceExW(root, &freeBytes, nullptr, nullptr))
                return 0;
            return freeBytes.QuadPart;
        }

        CreateOutcome createFailed(CreateResult result, const QString& error) {
            CreateOutcome out;
            out.result = result;
            out.error = error;
            return out;
        }

        // Returns the method's ReturnValue (0 is success), or -1 if WMI can't be reached.
        long wmiRestore(int sequenceNumber, QString* why) {
            const bool ownsCom = SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED));

            long result = -1;
            IWbemLocator* locator = nullptr;
            IWbemServices* services = nullptr;
            IWbemClassObject* cls = nullptr;
            IWbemClassObject* inDef = nullptr;
            IWbemClassObject* in = nullptr;
            IWbemClassObject* outParams = nullptr;

            HRESULT hr = connectToRestoreNamespace(locator, services);
            if (SUCCEEDED(hr)) {
                BSTR clsName = SysAllocString(L"SystemRestore");
                hr = services->GetObject(clsName, 0, nullptr, &cls, nullptr);
                SysFreeString(clsName);
            }
            if (SUCCEEDED(hr))
                hr = cls->GetMethod(L"Restore", 0, &inDef, nullptr);
            if (SUCCEEDED(hr))
                hr = inDef->SpawnInstance(0, &in);
            if (SUCCEEDED(hr)) {
                VARIANT v;
                VariantInit(&v);
                v.vt = VT_I4;
                v.lVal = sequenceNumber;
                hr = in->Put(L"SequenceNumber", 0, &v, 0);
                VariantClear(&v);
            }
            if (SUCCEEDED(hr)) {
                BSTR clsName = SysAllocString(L"SystemRestore");
                BSTR method  = SysAllocString(L"Restore");
                hr = services->ExecMethod(clsName, method, 0, nullptr, in, &outParams, nullptr);
                SysFreeString(clsName);
                SysFreeString(method);
            }
            if (SUCCEEDED(hr) && outParams) {
                VARIANT rv;
                VariantInit(&rv);
                if (SUCCEEDED(outParams->Get(L"ReturnValue", 0, &rv, nullptr, nullptr))
                    && (rv.vt == VT_I4 || rv.vt == VT_UI4))
                    result = static_cast<long>(rv.lVal);
                VariantClear(&rv);
            } else if (why) {
                *why = QStringLiteral("WMI 0x%1").arg(static_cast<quint32>(hr), 8, 16, QLatin1Char('0'));
            }

            if (outParams) outParams->Release();
            if (in)        in->Release();
            if (inDef)     inDef->Release();
            if (cls)       cls->Release();
            if (services)  services->Release();
            if (locator)   locator->Release();
            if (ownsCom)
                CoUninitialize();
            return result;
        }

        bool restartForRestore() {
            HANDLE token = nullptr;
            if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
                TOKEN_PRIVILEGES tp{};
                tp.PrivilegeCount = 1;
                tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
                if (LookupPrivilegeValueW(nullptr, SE_SHUTDOWN_NAME, &tp.Privileges[0].Luid))
                    AdjustTokenPrivileges(token, FALSE, &tp, 0, nullptr, nullptr);
                CloseHandle(token);
            }
            wchar_t message[] = L"Vethida is restarting Windows to finish the system restore.";
            return InitiateShutdownW(nullptr, message, 10,
                                     SHUTDOWN_RESTART | SHUTDOWN_FORCE_OTHERS,
                                     SHTDN_REASON_MAJOR_OPERATINGSYSTEM
                                         | SHTDN_REASON_MINOR_RECONFIG
                                         | SHTDN_REASON_FLAG_PLANNED) == ERROR_SUCCESS;
        }

        DeleteOutcome classifyDeleteOutput(bool ran, const QString& stdoutText,
                                           const QString& stderrText) {
            DeleteOutcome out;
            const QString s = stdoutText.trimmed();
            if (ran && s == QStringLiteral("OK")) {
                out.result = DeleteResult::Ok;
                return out;
            }

            const QString msg = s.startsWith(QStringLiteral("ERR:"))
                ? s.mid(4).trimmed()
                : stderrText.trimmed();
            const QString low = msg.toLower();
            if (msg == QStringLiteral("NOTFOUND") || msg == QStringLiteral("NOMATCH")
                || (low.contains(QStringLiteral("not")) && low.contains(QStringLiteral("found")))) {
                out.result = DeleteResult::NotFound;
                out.error = QStringLiteral(
                    "This restore point could no longer be matched to a shadow copy.");
            } else if (low.contains(QStringLiteral("denied"))
                       || low.contains(QStringLiteral("administrator"))) {
                out.result = DeleteResult::AccessDenied;
                out.error = QStringLiteral("Windows refused to delete the restore point. "
                                           "Security software that protects backups is the "
                                           "usual cause.");
            } else {
                out.result = DeleteResult::OtherError;
                out.error = msg;
            }
            return out;
        }

        DeleteOutcome deleteFailed(DeleteResult result, const QString& error) {
            DeleteOutcome out;
            out.result = result;
            out.error = error;
            return out;
        }
    }

    EnumerateResult enumerateChecked() {
        EnumerateResult r = enumerateWmi();
        std::sort(r.points.begin(), r.points.end(),
                  [](const RestorePoint& a, const RestorePoint& b) {
                      return a.sequenceNumber > b.sequenceNumber;
                  });
        return r;
    }

    QString createResultLabel(CreateResult r) {
        switch (r) {
        case CreateResult::Ok:                 return QStringLiteral("Restore point created.");
        case CreateResult::AccessDenied:       return QStringLiteral("Creating a restore point needs administrator rights.");
        case CreateResult::Throttled:          return QStringLiteral("Windows already made a restore point in the last 24 hours.");
        case CreateResult::ProtectionDisabled: return QStringLiteral("System Protection is turned off for the system drive.");
        case CreateResult::ServicesDisabled:   return QStringLiteral("The Volume Shadow Copy service is disabled.");
        case CreateResult::DiskFull:           return QStringLiteral("There is not enough free space on the system drive.");
        case CreateResult::OtherError:         return QStringLiteral("Windows could not create the restore point.");
        }
        return {};
    }

    CreateOutcome create(const QString& description, bool bypassThrottle) {
        if (description.isEmpty())
            return createFailed(CreateResult::OtherError,
                                QStringLiteral("A restore point needs a description."));

        if (policyDisablesRestore())
            return createFailed(CreateResult::ProtectionDisabled,
                                QStringLiteral("A Group Policy on this PC turns System Restore off."));
        if (serviceStartType(L"VSS") == SERVICE_DISABLED
            || serviceStartType(L"swprv") == SERVICE_DISABLED)
            return createFailed(CreateResult::ServicesDisabled,
                                QStringLiteral("The Volume Shadow Copy service, which every restore "
                                               "point is made with, is disabled."));
        constexpr quint64 kMinFree = 1024ull * 1024 * 1024;
        const quint64 freeBytes = systemDriveFreeBytes();
        if (freeBytes > 0 && freeBytes < kMinFree)
            return createFailed(CreateResult::DiskFull,
                                QStringLiteral("The system drive has less than 1 GB free. Free up some "
                                               "space (Clean up on the Dashboard helps), then try again."));

        const EnumerateResult before = enumerateWmi();
        const int newestBefore = before.ok ? newestSequence(before.points) : 0;

        HMODULE h = LoadLibraryW(L"srclient.dll");
        if (!h)
            return createFailed(CreateResult::OtherError,
                                QStringLiteral("Windows' System Restore component is missing. Use "
                                               "\"Run system file check\" in System > Fixes to restore it."));
        auto fn = reinterpret_cast<SRSetRestorePointWFn>(
            GetProcAddress(h, "SRSetRestorePointW"));
        if (!fn) {
            FreeLibrary(h);
            return createFailed(CreateResult::OtherError,
                                QStringLiteral("Windows' System Restore component is damaged. Use "
                                               "\"Run system file check\" in System > Fixes to repair it."));
        }

        LONG savedFreq = -1;
        HKEY srKey = nullptr;
        if (bypassThrottle
            && RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                             L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\SystemRestore",
                             0, KEY_READ | KEY_WRITE, &srKey) == ERROR_SUCCESS) {
            DWORD value = 0;
            DWORD sz = sizeof(value);
            if (RegQueryValueExW(srKey, L"SystemRestorePointCreationFrequency", nullptr, nullptr,
                                 reinterpret_cast<LPBYTE>(&value), &sz) == ERROR_SUCCESS)
                savedFreq = static_cast<LONG>(value);
            const DWORD zero = 0;
            RegSetValueExW(srKey, L"SystemRestorePointCreationFrequency", 0, REG_DWORD,
                           reinterpret_cast<const BYTE*>(&zero), sizeof(zero));
        }

        VTH_RESTOREPOINTINFOW info{};
        info.dwEventType = kBeginSystemChange;
        info.dwRestorePtType = kModifySettings;
        const std::wstring desc = description.toStdWString();
        wcsncpy_s(info.szDescription, desc.c_str(), _TRUNCATE);

        VTH_STATEMGRSTATUS st{};
        const BOOL ok = fn(&info, &st);
        const DWORD status = st.nStatus;
        const INT64 sequence = st.llSequenceNumber;

        // The restore point only exists once the change is closed.
        VTH_STATEMGRSTATUS endSt{};
        BOOL ended = FALSE;
        if (ok && status == 0) {
            info.dwEventType = kEndSystemChange;
            info.llSequenceNumber = sequence;
            ended = fn(&info, &endSt);
        }
        const DWORD endStatus = endSt.nStatus;

        // Only now: the throttle has to stay lifted until the end call returns.
        if (srKey) {
            if (savedFreq >= 0) {
                const DWORD restore = static_cast<DWORD>(savedFreq);
                RegSetValueExW(srKey, L"SystemRestorePointCreationFrequency", 0, REG_DWORD,
                               reinterpret_cast<const BYTE*>(&restore), sizeof(restore));
            } else {
                RegDeleteValueW(srKey, L"SystemRestorePointCreationFrequency");
            }
            RegCloseKey(srKey);
        }

        FreeLibrary(h);

        if (ok && status == 0) {
            if (!ended || endStatus != 0)
                return createFailed(CreateResult::OtherError,
                                    QStringLiteral("Windows started the restore point but did not finish "
                                                   "it (error %1). Try again in a minute.").arg(endStatus));

            // When throttled, Windows reports success but keeps nothing new, so
            // only a newer sequence number counts.
            for (int attempt = 0; attempt < 10; ++attempt) {
                const EnumerateResult after = enumerateWmi();
                if (!after.ok)
                    break;
                for (const RestorePoint& p : after.points) {
                    if (p.sequenceNumber > newestBefore) {
                        CreateOutcome out;
                        out.result = CreateResult::Ok;
                        out.sequenceNumber = p.sequenceNumber;
                        return out;
                    }
                }
                QThread::msleep(500);
            }
            if (!before.ok) {
                // Nothing to compare against, so take Windows' word for it.
                CreateOutcome out;
                out.result = CreateResult::Ok;
                out.sequenceNumber = static_cast<int>(sequence);
                return out;
            }
            return bypassThrottle
                ? createFailed(CreateResult::OtherError,
                               QStringLiteral("Windows accepted the request but did not keep a new "
                                              "restore point. Try again in a few minutes."))
                : createFailed(CreateResult::Throttled,
                               QStringLiteral("Windows makes one restore point a day, and this PC "
                                              "already has one from the last 24 hours."));
        }

        switch (status) {
        case ERROR_SERVICE_DISABLED:
            return createFailed(CreateResult::ProtectionDisabled,
                                QStringLiteral("System Protection is off for the system drive, so "
                                               "Windows cannot make restore points."));
        case ERROR_ACCESS_DENIED:
            return createFailed(CreateResult::AccessDenied,
                                QStringLiteral("Windows refused the request. Security software or a "
                                               "Group Policy is blocking restore points."));
        case ERROR_INVALID_PARAMETER:
            return createFailed(CreateResult::Throttled,
                                QStringLiteral("Windows makes one restore point a day, and this PC "
                                               "already has one from the last 24 hours."));
        case ERROR_DISK_FULL:
            return createFailed(CreateResult::DiskFull,
                                QStringLiteral("The system drive is out of space for restore points. "
                                               "Free up some space, then try again."));
        case ERROR_TIMEOUT:
            return createFailed(CreateResult::OtherError,
                                QStringLiteral("Windows took too long to make the restore point, usually "
                                               "because another backup or update is running. Try again "
                                               "once it finishes."));
        default:
            return createFailed(CreateResult::OtherError,
                                QStringLiteral("Windows could not make the restore point (error %1).")
                                    .arg(status));
        }
    }

    QString enableSystemProtection() {
        // Clears the disable flags, re-enables VSS and swprv, turns protection on
        // and tries to allocate shadow storage. Failures come back as "ERR:" on stdout.
        const QString script = QStringLiteral(
            "$ErrorActionPreference='Stop';"
            "try{"
            "$root=$env:SystemDrive + '\\';"
            "foreach($k in @("
            "'HKLM:\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\SystemRestore',"
            "'HKLM:\\SOFTWARE\\Policies\\Microsoft\\Windows NT\\SystemRestore')){"
            "if(Test-Path $k){foreach($v in 'DisableSR','DisableConfig'){"
            "Set-ItemProperty -Path $k -Name $v -Value 0 -Type DWord -Force "
            "-ErrorAction SilentlyContinue}}}"
            "foreach($svc in 'VSS','swprv'){"
            "try{Set-Service -Name $svc -StartupType Manual -ErrorAction Stop}catch{}}"
            "try{Start-Service -Name VSS -ErrorAction Stop}catch{}"
            "Enable-ComputerRestore -Drive $root;"
            "try{vssadmin Resize ShadowStorage /For=$root /On=$root /MaxSize=10GB | Out-Null}catch{}"
            "}catch{Write-Output ('ERR:' + $_.Exception.Message); exit 1}");

        QString out, err;
        const bool ok = runPowerShell(script, &out, &err);
        const QString stdoutText = out.trimmed();

        if (ok && !stdoutText.startsWith(QStringLiteral("ERR:")))
            return {};

        const QString detail = stdoutText.startsWith(QStringLiteral("ERR:"))
            ? stdoutText.mid(4).trimmed()
            : err.trimmed();
        return detail.isEmpty()
            ? QStringLiteral("Windows could not turn on System Protection.")
            : detail;
    }

    QString restoreResultLabel(RestoreResult r) {
        switch (r) {
        case RestoreResult::Restarting:   return QStringLiteral("Restoring (restarting)");
        case RestoreResult::AccessDenied: return QStringLiteral("Windows refused the restore");
        case RestoreResult::InProgress:   return QStringLiteral("A restore is already in progress");
        case RestoreResult::NotFound:     return QStringLiteral("Restore point not found");
        case RestoreResult::OtherError:   return QStringLiteral("Restore failed");
        }
        return {};
    }

    RestoreOutcome restoreToPoint(int sequenceNumber) {
        RestoreOutcome out;
        if (sequenceNumber <= 0) {
            out.result = RestoreResult::NotFound;
            out.error = QStringLiteral("Invalid restore point.");
            return out;
        }

        const EnumerateResult listed = enumerateWmi();
        if (listed.ok) {
            const bool exists = std::any_of(listed.points.cbegin(), listed.points.cend(),
                [sequenceNumber](const RestorePoint& p) { return p.sequenceNumber == sequenceNumber; });
            if (!exists) {
                out.result = RestoreResult::NotFound;
                out.error = QStringLiteral("Windows no longer has this restore point. It removes the "
                                           "oldest ones when their space runs out.");
                return out;
            }
        }

        QString why;
        const long rv = wmiRestore(sequenceNumber, &why);
        if (rv == 0) {
            if (restartForRestore()) {
                out.result = RestoreResult::Restarting;
                return out;
            }
            out.result = RestoreResult::OtherError;
            out.error = QStringLiteral("The restore is set up, but Windows would not restart. "
                                       "Restart the PC yourself to finish it.");
            return out;
        }
        if (rv > 0) {
            out.result = RestoreResult::OtherError;
            out.error = QStringLiteral("Windows refused to start the restore (error %1).").arg(rv);
            if (rv == ERROR_ACCESS_DENIED) {
                out.result = RestoreResult::AccessDenied;
            } else if (rv == ERROR_SERVICE_DISABLED) {
                out.error = QStringLiteral("System Protection is off, so Windows cannot restore. "
                                           "Turn it on above, then try again.");
            }
            return out;
        }

        // WMI couldn't be reached, so try the PowerShell cmdlet once.
        const QString script = QStringLiteral(
            "$ErrorActionPreference='Stop';"
            "try{Restore-ComputerRestorePoint -RestorePoint %1 -Confirm:$false; 'OK'}"
            "catch{Write-Output ('ERR:' + $_.Exception.Message); exit 1}")
            .arg(sequenceNumber);

        QString o, e;
        const bool ran = runPowerShell(script, &o, &e, 90000);
        const QString s = o.trimmed();

        if (s.startsWith(QStringLiteral("ERR:"))) {
            const QString msg = s.mid(4).trimmed();
            out.error = msg;
            const QString low = msg.toLower();
            if (low.contains(QStringLiteral("denied")))
                out.result = RestoreResult::AccessDenied;
            else if (low.contains(QStringLiteral("progress")))
                out.result = RestoreResult::InProgress;
            else if (low.contains(QStringLiteral("not")) && low.contains(QStringLiteral("found")))
                out.result = RestoreResult::NotFound;
            else
                out.result = RestoreResult::OtherError;
            return out;
        }
        if (ran && s.endsWith(QStringLiteral("OK"))) {
            out.result = RestoreResult::Restarting;
            return out;
        }
        out.result = RestoreResult::OtherError;
        out.error = QStringLiteral("Windows' restore service did not answer (%1). Open System "
                                   "Restore from the Start menu (rstrui) and pick this point there.")
                        .arg(why.isEmpty() ? QStringLiteral("no response") : why);
        return out;
    }

    QString deleteResultLabel(DeleteResult r) {
        switch (r) {
        case DeleteResult::Ok:           return QStringLiteral("Deleted");
        case DeleteResult::AccessDenied: return QStringLiteral("Windows refused to delete it");
        case DeleteResult::NotFound:     return QStringLiteral("Restore point not found");
        case DeleteResult::OtherError:   return QStringLiteral("Delete failed");
        }
        return {};
    }

    DeleteOutcome deleteRestorePoint(int sequenceNumber) {
        if (sequenceNumber <= 0)
            return deleteFailed(DeleteResult::NotFound, QStringLiteral("Invalid restore point."));
        if (!isElevated())
            return deleteFailed(DeleteResult::AccessDenied,
                                QStringLiteral("Administrator rights are required to delete."));

        // A restore point and its shadow copy are created together. Refuse if the
        // closest one is over 90 seconds off, so the wrong one is never deleted.
        const QString script = QStringLiteral(
            "$ErrorActionPreference='Stop';"
            "try{"
            "$rp=Get-ComputerRestorePoint | Where-Object {$_.SequenceNumber -eq %1};"
            "if(-not $rp){Write-Output 'ERR:NOTFOUND'; exit 1}"
            "$t=$rp.ConvertToDateTime($rp.CreationTime);"
            "$best=$null; $bestDiff=[double]::MaxValue;"
            "foreach($s in Get-CimInstance Win32_ShadowCopy){"
            "$d=[math]::Abs(($s.InstallDate - $t).TotalSeconds);"
            "if($d -lt $bestDiff){$bestDiff=$d; $best=$s}}"
            "if($null -eq $best -or $bestDiff -gt 90){Write-Output 'ERR:NOMATCH'; exit 1}"
            "vssadmin delete shadows /shadow=$($best.ID) /quiet | Out-Null;"
            "if($LASTEXITCODE -ne 0){Write-Output ('ERR:vssadmin exit ' + $LASTEXITCODE); exit 1}"
            "Write-Output 'OK'"
            "}catch{Write-Output ('ERR:' + $_.Exception.Message); exit 1}")
            .arg(sequenceNumber);

        QString o, e;
        const bool ran = runPowerShell(script, &o, &e, 60000);
        return classifyDeleteOutput(ran, o, e);
    }

    DeleteOutcome deleteAllRestorePoints() {
        if (!isElevated())
            return deleteFailed(DeleteResult::AccessDenied,
                                QStringLiteral("Administrator rights are required to delete."));

        // vssadmin exits 1 when there is nothing to delete, which is fine here. No
        // 2>&1, because PowerShell 5.1 turns that into a NativeCommandError.
        const QString script = QStringLiteral(
            "$ErrorActionPreference='Stop';"
            "try{"
            "$root=$env:SystemDrive;"
            "vssadmin delete shadows /for=$root /all /quiet | Out-Null;"
            "if($LASTEXITCODE -gt 1){Write-Output ('ERR:vssadmin exit ' + $LASTEXITCODE); exit 1}"
            "Write-Output 'OK'"
            "}catch{Write-Output ('ERR:' + $_.Exception.Message); exit 1}");

        QString o, e;
        const bool ran = runPowerShell(script, &o, &e, 60000);
        return classifyDeleteOutput(ran, o, e);
    }
}

namespace vth::win::sysrestore
{
namespace
{
        bool serviceState(const wchar_t* name, bool& installedOut, bool& runningOut) {
            installedOut = false;
            runningOut = false;
            SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
            if (!scm) return false;
            SC_HANDLE svc = OpenServiceW(scm, name, SERVICE_QUERY_STATUS);
            if (!svc) {
                CloseServiceHandle(scm);
                return true;
            }
            installedOut = true;
            SERVICE_STATUS_PROCESS st{};
            DWORD needed = 0;
            if (QueryServiceStatusEx(svc, SC_STATUS_PROCESS_INFO,
                reinterpret_cast<LPBYTE>(&st), sizeof(st),
                &needed)) {
                runningOut = (st.dwCurrentState == SERVICE_RUNNING);
            }
            CloseServiceHandle(svc);
            CloseServiceHandle(scm);
            return true;
        }
        bool readSystemProtectionEnabled() {
            QString out, err;
            runPowerShell(
                QStringLiteral("Get-ComputerRestorePoint | "
                    "Measure-Object | "
                    "Select-Object -ExpandProperty Count"),
                &out, &err);
            bool ok = false;
            const int n = out.trimmed().toInt(&ok);
            return ok && n > 0;
        }
        bool wmiReachable() {
            QString out, err;
            return runPowerShell(
                QStringLiteral("Get-WmiObject -Namespace root\\default "
                    "-Class SystemRestore -ErrorAction Stop | "
                    "Select-Object -First 1 | Out-Null"),
                &out, &err);
        }
}
}

namespace vth::win::sysrestore
{
    Readiness checkReadiness() {
        Readiness r;
        r.isElevated = isElevated();
        bool installed = false, running = false;
        if (serviceState(L"VSS", installed, running)) {
            r.vssServiceInstalled = installed;
            r.vssServiceRunning = running;
        }
        r.systemProtectionEnabled = readSystemProtectionEnabled();
        r.wmiReachable = wmiReachable();
        return r;
    }
    QVector<RestorePoint> enumeratePowerShell();
    QVector<RestorePoint> enumerate() {
        const EnumerateResult checked = enumerateChecked();
        if (checked.ok)
            return checked.points;
        return enumeratePowerShell();
    }
    QVector<RestorePoint> enumeratePowerShell() {
        QVector<RestorePoint> out;
        const QString script = QStringLiteral(
            "Get-ComputerRestorePoint | "
            "Sort-Object SequenceNumber -Descending | "
            "Select-Object -First 100 | "
            "ForEach-Object { "
            "'{0}|{1}|{2}' -f "
            "$_.SequenceNumber,"
            "($_.Description -replace '\\|',' '),"
            "$_.CreationTime "
            "}");
        QString stdout_, stderr_;
        if (!runPowerShell(script, &stdout_, &stderr_)) {
            return out;
        }
        QChar nlChar(L'\n');
        QChar pipeChar(L'|');
        const auto lines = stdout_.split(nlChar, Qt::SkipEmptyParts);
        for (const auto& raw : lines) {
            const QString line = raw.trimmed();
            if (line.isEmpty()) continue;
            const auto parts = line.split(pipeChar);
            if (parts.size() < 3) continue;
            RestorePoint rp;
            rp.sequenceNumber = parts[0].toInt();
            rp.description = parts[1];
            rp.createdUtc = parseWmiDate(parts[2]);
            if (!rp.createdUtc.isValid()) {
                rp.createdUtc = QDateTime::fromString(parts[2], Qt::ISODate);
            }
            rp.isVethida = rp.description.contains(QStringLiteral("Vethida"),
                Qt::CaseInsensitive);
            out.append(rp);
        }
        return out;
    }
}
