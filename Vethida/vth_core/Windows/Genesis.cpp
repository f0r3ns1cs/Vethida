#include "Genesis.h"

#include "../Logger.h"
#include "../ProtectedProcesses.h"
#include "../ProtectedServices.h"
#include "../Runtime/Tier.h"
#include "../Runtime/Paths.h"
#include "../Runtime/ServiceRegistry.h"
#include "../Runtime/Settings.h"
#include "../Runtime/TweakStateTracker.h"
#include "../Devices.h"
#include "NvApi.h"
#include "Power.h"
#include "Privileges.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>
#include <QStringLiteral>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <windows.h>
#include <dwmapi.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <winsvc.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "advapi32.lib")

namespace vth::win::genesis
{
    namespace
    {
        VTH_DEFINE_LOG_CATEGORY(catGenesis, "vth.genesis")

        using NtSetTimerResolution_t = LONG (NTAPI*)(ULONG, BOOLEAN, PULONG);

        NtSetTimerResolution_t resolveNtSetTimerResolution() {
            static NtSetTimerResolution_t fn = nullptr;
            static bool resolved = false;
            if (!resolved) {
                if (HMODULE m = GetModuleHandleW(L"ntdll.dll")) {
                    fn = reinterpret_cast<NtSetTimerResolution_t>(
                        GetProcAddress(m, "NtSetTimerResolution"));
                }
                resolved = true;
            }
            return fn;
        }

        // Undocumented, but stable since Vista.
        constexpr int kSystemMemoryListInformation = 80;

        enum SYSTEM_MEMORY_LIST_COMMAND {
            MemoryCaptureAccessedBits,
            MemoryCaptureAndResetAccessedBits,
            MemoryEmptyWorkingSets,
            MemoryFlushModifiedList,
            MemoryPurgeStandbyList,
            MemoryPurgeLowPriorityStandbyList,
            MemoryCommandMax,
        };

        using NtSetSystemInformation_t = LONG (NTAPI*)(int, PVOID, ULONG);

        NtSetSystemInformation_t resolveNtSetSystemInformation() {
            static NtSetSystemInformation_t fn = nullptr;
            static bool resolved = false;
            if (!resolved) {
                if (HMODULE m = GetModuleHandleW(L"ntdll.dll")) {
                    fn = reinterpret_cast<NtSetSystemInformation_t>(
                        GetProcAddress(m, "NtSetSystemInformation"));
                }
                resolved = true;
            }
            return fn;
        }

        QString stateFilePath() {
            return paths::protectedData() + QStringLiteral("/genesis-state.json");
        }

        // Absolute path, because this runs elevated.
        QString system32(const wchar_t* relative) {
            wchar_t dir[MAX_PATH] = {};
            const UINT n = GetSystemDirectoryW(dir, MAX_PATH);
            if (n == 0 || n >= MAX_PATH) return QString::fromWCharArray(relative);
            return QString::fromWCharArray(dir) + QLatin1Char('\\')
                 + QString::fromWCharArray(relative);
        }

        // Without enumerateProcesses()'s handle per process, because the monitor runs
        // this on a timer.
        QStringList enumerateProcessNames() {
            QStringList names;
            HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (snap == INVALID_HANDLE_VALUE) return names;
            PROCESSENTRY32W pe{};
            pe.dwSize = sizeof(pe);
            if (Process32FirstW(snap, &pe)) {
                do {
                    names << QString::fromWCharArray(pe.szExeFile);
                } while (Process32NextW(snap, &pe));
            }
            CloseHandle(snap);
            return names;
        }

        // QueryFullProcessImageNameW fails rather than truncates, and paths can pass 260.
        constexpr DWORD kImagePathChars = 1024;

        bool killProcessByName(const QString& name) {
            HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (snap == INVALID_HANDLE_VALUE) return false;
            PROCESSENTRY32W pe{};
            pe.dwSize = sizeof(pe);
            bool any = false;
            if (Process32FirstW(snap, &pe)) {
                do {
                    const QString exe = QString::fromWCharArray(pe.szExeFile);
                    if (exe.compare(name, Qt::CaseInsensitive) != 0) continue;
                    HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                    if (h) {
                        if (TerminateProcess(h, 1)) any = true;
                        CloseHandle(h);
                    }
                } while (Process32NextW(snap, &pe));
            }
            CloseHandle(snap);
            return any;
        }

        using NtSuspendResume_t = LONG (NTAPI*)(HANDLE);

        NtSuspendResume_t resolveNtProc(const char* name) {
            if (HMODULE m = GetModuleHandleW(L"ntdll.dll"))
                return reinterpret_cast<NtSuspendResume_t>(GetProcAddress(m, name));
            return nullptr;
        }

        // Returns whether at least one process was acted on.
        bool suspendResumeByName(const QString& name, bool suspend) {
            static NtSuspendResume_t pSuspend = resolveNtProc("NtSuspendProcess");
            static NtSuspendResume_t pResume  = resolveNtProc("NtResumeProcess");
            const NtSuspendResume_t fn = suspend ? pSuspend : pResume;
            if (!fn) return false;

            HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (snap == INVALID_HANDLE_VALUE) return false;
            PROCESSENTRY32W pe{};
            pe.dwSize = sizeof(pe);
            bool any = false;
            if (Process32FirstW(snap, &pe)) {
                do {
                    const QString exe = QString::fromWCharArray(pe.szExeFile);
                    if (exe.compare(name, Qt::CaseInsensitive) != 0) continue;
                    HANDLE h = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, pe.th32ProcessID);
                    if (h) {
                        if (fn(h) >= 0) any = true;
                        CloseHandle(h);
                    }
                } while (Process32NextW(snap, &pe));
            }
            CloseHandle(snap);
            return any;
        }

        HKEY hiveFor(const QString& hive) {
            if (hive.compare(QStringLiteral("HKLM"), Qt::CaseInsensitive) == 0)
                return HKEY_LOCAL_MACHINE;
            if (hive.compare(QStringLiteral("HKCU"), Qt::CaseInsensitive) == 0)
                return HKEY_CURRENT_USER;
            return nullptr;
        }

        bool snapshotRegValue(const RegTweak& t, RegSnapshot& snap) {
            snap.hive    = t.hive;
            snap.subkey  = t.subkey;
            snap.name    = t.name;
            snap.isDword = t.isDword;
            HKEY h = hiveFor(t.hive);
            if (!h) return false;
            HKEY key = nullptr;
            if (RegOpenKeyExW(h, t.subkey.toStdWString().c_str(),
                              0, KEY_READ, &key) != ERROR_SUCCESS) {
                snap.existedBefore = false;
                return true;
            }
            DWORD type = 0, size = 0;
            const LONG q = RegQueryValueExW(key, t.name.toStdWString().c_str(),
                                            nullptr, &type, nullptr, &size);
            if (q != ERROR_SUCCESS) {
                snap.existedBefore = false;
                RegCloseKey(key);
                return true;
            }
            snap.existedBefore = true;
            if (type == REG_DWORD && size == sizeof(DWORD)) {
                DWORD v = 0;
                DWORD sz = sizeof(v);
                RegQueryValueExW(key, t.name.toStdWString().c_str(),
                                 nullptr, &type, reinterpret_cast<LPBYTE>(&v), &sz);
                snap.isDword   = true;
                snap.prevDword = v;
            } else if (type == REG_SZ || type == REG_EXPAND_SZ) {
                std::vector<wchar_t> buf(size / sizeof(wchar_t) + 2, L'\0');
                DWORD sz = size;
                RegQueryValueExW(key, t.name.toStdWString().c_str(),
                                 nullptr, &type, reinterpret_cast<LPBYTE>(buf.data()), &sz);
                snap.isDword    = false;
                snap.prevString = QString::fromWCharArray(buf.data());
            } else {
                snap.isDword   = true;
                snap.prevDword = 0;
            }
            RegCloseKey(key);
            return true;
        }

        bool writeRegValue(const RegTweak& t) {
            HKEY h = hiveFor(t.hive);
            if (!h) return false;
            if (h == HKEY_LOCAL_MACHINE && !vth::win::isElevated()) {
                qCWarning(catGenesis).noquote()
                    << "writeRegValue refused (not elevated):"
                    << t.hive << "\\" << t.subkey << "\\" << t.name;
                return false;
            }
            HKEY key = nullptr;
            DWORD disp = 0;
            if (RegCreateKeyExW(h, t.subkey.toStdWString().c_str(), 0, nullptr,
                                REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_READ,
                                nullptr, &key, &disp) != ERROR_SUCCESS) {
                return false;
            }
            LONG r;
            if (t.isDword) {
                const DWORD v = t.dwordValue;
                r = RegSetValueExW(key, t.name.toStdWString().c_str(),
                                   0, REG_DWORD, reinterpret_cast<const BYTE*>(&v), sizeof(v));
            } else {
                const std::wstring w = t.stringValue.toStdWString();
                r = RegSetValueExW(key, t.name.toStdWString().c_str(),
                                   0, REG_SZ, reinterpret_cast<const BYTE*>(w.c_str()),
                                   static_cast<DWORD>((w.size() + 1) * sizeof(wchar_t)));
            }
            RegCloseKey(key);
            return r == ERROR_SUCCESS;
        }

        bool restoreRegValue(const RegSnapshot& snap) {
            HKEY h = hiveFor(snap.hive);
            if (!h) return false;
            HKEY key = nullptr;
            const LONG opened = RegOpenKeyExW(h, snap.subkey.toStdWString().c_str(),
                                              0, KEY_WRITE, &key);
            if (opened != ERROR_SUCCESS) {
                // Nothing to remove if the key has gone, so don't keep retrying.
                return opened == ERROR_FILE_NOT_FOUND && !snap.existedBefore;
            }
            LONG r = ERROR_SUCCESS;
            if (!snap.existedBefore) {
                r = RegDeleteValueW(key, snap.name.toStdWString().c_str());
                if (r == ERROR_FILE_NOT_FOUND) r = ERROR_SUCCESS;
            } else if (snap.isDword) {
                const DWORD v = snap.prevDword;
                r = RegSetValueExW(key, snap.name.toStdWString().c_str(),
                                   0, REG_DWORD,
                                   reinterpret_cast<const BYTE*>(&v), sizeof(v));
            } else {
                const std::wstring w = snap.prevString.toStdWString();
                r = RegSetValueExW(key, snap.name.toStdWString().c_str(),
                                   0, REG_SZ,
                                   reinterpret_cast<const BYTE*>(w.c_str()),
                                   static_cast<DWORD>((w.size() + 1) * sizeof(wchar_t)));
            }
            RegCloseKey(key);
            return r == ERROR_SUCCESS;
        }

        void recordInTracker(const RegSnapshot& snap, const QString& label) {
            TweakSnapshot ts;
            ts.domain        = QStringLiteral("genesis");
            ts.kind          = SnapshotKind::Registry;
            ts.label         = label;
            ts.hive          = snap.hive;
            ts.subkey        = snap.subkey;
            ts.name          = snap.name;
            ts.existedBefore = snap.existedBefore;
            ts.isDword       = snap.isDword;
            ts.prevDword     = snap.prevDword;
            ts.prevString    = snap.prevString;
            recordTweakSnapshot(std::move(ts));
        }

        class ScmHandle {
        public:
            explicit ScmHandle(SC_HANDLE h) : h_(h) {}
            ~ScmHandle() { if (h_) CloseServiceHandle(h_); }
            SC_HANDLE get() const { return h_; }
            explicit operator bool() const { return h_ != nullptr; }
            ScmHandle(const ScmHandle&) = delete;
            ScmHandle& operator=(const ScmHandle&) = delete;
        private:
            SC_HANDLE h_ = nullptr;
        };

        bool stopOneService(SC_HANDLE scm, const QString& name, ServiceSnapshot& snap) {
            snap.name = name;

            ScmHandle svc(OpenServiceW(scm, name.toStdWString().c_str(),
                                       SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS
                                     | SERVICE_CHANGE_CONFIG | SERVICE_STOP));
            if (!svc) return false;

            DWORD bytesNeeded = 0;
            QueryServiceConfigW(svc.get(), nullptr, 0, &bytesNeeded);
            std::vector<BYTE> cfgBuf(bytesNeeded);
            auto* cfg = reinterpret_cast<QUERY_SERVICE_CONFIGW*>(cfgBuf.data());
            if (!QueryServiceConfigW(svc.get(), cfg, bytesNeeded, &bytesNeeded)) {
                return false;
            }
            snap.prevStartType = cfg->dwStartType;

            SERVICE_STATUS_PROCESS status{};
            DWORD bytes = 0;
            QueryServiceStatusEx(svc.get(), SC_STATUS_PROCESS_INFO,
                                 reinterpret_cast<LPBYTE>(&status),
                                 sizeof(status), &bytes);
            snap.wasRunning = (status.dwCurrentState == SERVICE_RUNNING);

            if (snap.wasRunning) {
                SERVICE_STATUS s{};
                ControlService(svc.get(), SERVICE_CONTROL_STOP, &s);
            }
            // So it doesn't start again at the next boot if the session is cut off.
            ChangeServiceConfigW(svc.get(),
                                 SERVICE_NO_CHANGE, SERVICE_DEMAND_START,
                                 SERVICE_NO_CHANGE,
                                 nullptr, nullptr, nullptr,
                                 nullptr, nullptr, nullptr, nullptr);
            return true;
        }

        bool restartOneService(SC_HANDLE scm, const ServiceSnapshot& snap) {
            ScmHandle svc(OpenServiceW(scm, snap.name.toStdWString().c_str(),
                                       SERVICE_CHANGE_CONFIG | SERVICE_START));
            if (!svc) return GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST;
            if (!ChangeServiceConfigW(svc.get(),
                                      SERVICE_NO_CHANGE, snap.prevStartType,
                                      SERVICE_NO_CHANGE,
                                      nullptr, nullptr, nullptr,
                                      nullptr, nullptr, nullptr, nullptr))
                return false;
            if (snap.wasRunning) {
                StartServiceW(svc.get(), 0, nullptr);
            }
            return true;
        }
    }

    const QStringList& defaultCloseList() {
        static const QStringList kList = {
            QStringLiteral("GameBar.exe"),
            QStringLiteral("GameBarFTServer.exe"),
            QStringLiteral("GameBarPresenceWriter.exe"),
            QStringLiteral("XboxGameBar.exe"),
            QStringLiteral("OneDrive.exe"),
            QStringLiteral("GoogleDriveFS.exe"),
            QStringLiteral("Dropbox.exe"),
            QStringLiteral("Slack.exe"),
            QStringLiteral("Teams.exe"),
            QStringLiteral("ms-teams.exe"),
            QStringLiteral("Skype.exe"),
            QStringLiteral("Zoom.exe"),
            QStringLiteral("Discord.exe"),
            QStringLiteral("steamwebhelper.exe"),
            QStringLiteral("EpicGamesLauncher.exe"),
            QStringLiteral("EpicWebHelper.exe"),
            QStringLiteral("EADesktop.exe"),
            QStringLiteral("UbisoftConnect.exe"),
            QStringLiteral("GalaxyClient.exe"),
            QStringLiteral("RiotClientServices.exe"),
            QStringLiteral("Battle.net.exe"),
            QStringLiteral("Overwolf.exe"),
            QStringLiteral("iCUE.exe"),
            QStringLiteral("CorsairService.exe"),
            QStringLiteral("LightingService.exe"),
            QStringLiteral("SteelSeriesGG.exe"),
            QStringLiteral("Razer Synapse Service.exe"),
            QStringLiteral("MSI Center.exe"),
            QStringLiteral("Armoury Crate.exe"),
            QStringLiteral("LGHUB Updater.exe"),
            QStringLiteral("logioptionsplus.exe"),
            QStringLiteral("AdobeIPCBroker.exe"),
            QStringLiteral("CCXProcess.exe"),
            QStringLiteral("CCLibrary.exe"),
            QStringLiteral("Creative Cloud Helper.exe"),
            QStringLiteral("AdobeNotificationClient.exe"),
            QStringLiteral("GoogleUpdate.exe"),
            QStringLiteral("MicrosoftEdgeUpdate.exe"),
            QStringLiteral("SpotifyWebHelper.exe"),
            QStringLiteral("Widgets.exe"),
            QStringLiteral("WidgetService.exe"),
            QStringLiteral("PhoneExperienceHost.exe"),
            QStringLiteral("YourPhone.exe"),
            QStringLiteral("Cortana.exe"),
            QStringLiteral("Clipchamp.exe"),
            QStringLiteral("CompatTelRunner.exe"),
            QStringLiteral("DeviceCensus.exe"),
        };
        return kList;
    }

    static const QStringList& defaultProcessSuspendList() {
        static const QStringList kList = {
            QStringLiteral("steamwebhelper.exe"),
            QStringLiteral("RuntimeBroker.exe"),
            QStringLiteral("SearchUI.exe"),
            QStringLiteral("SearchApp.exe"),
            QStringLiteral("ShellExperienceHost.exe"),
            QStringLiteral("SystemSettings.exe"),
            QStringLiteral("TextInputHost.exe"),
            QStringLiteral("StartMenuExperienceHost.exe"),
            QStringLiteral("WidgetService.exe"),
            QStringLiteral("Widgets.exe"),
            QStringLiteral("GameBarPresenceWriter.exe"),
            QStringLiteral("GameBar.exe"),
            QStringLiteral("OneDrive.exe"),
            QStringLiteral("ms-teams.exe"),
            QStringLiteral("Teams.exe"),
            QStringLiteral("Slack.exe"),
            QStringLiteral("Spotify.exe"),
        };
        return kList;
    }

    const QStringList& defaultExceptions() {
        static const QStringList kList = {
            QStringLiteral("Taskmgr.exe"),
            QStringLiteral("cmd.exe"),
            QStringLiteral("powershell.exe"),
            QStringLiteral("pwsh.exe"),
            QStringLiteral("WindowsTerminal.exe"),
            QStringLiteral("WmiPrvSE.exe"),
            QStringLiteral("obs64.exe"),
            QStringLiteral("obs32.exe"),
            QStringLiteral("msedge.exe"),
            QStringLiteral("msedgewebview2.exe"),
        };
        return kList;
    }

    static const QStringList& defaultServiceStopList() {
        static const QStringList kList = {
            QStringLiteral("DiagTrack"),
            QStringLiteral("dmwappushservice"),
            QStringLiteral("WSearch"),
            QStringLiteral("SysMain"),
            QStringLiteral("WerSvc"),
            QStringLiteral("MapsBroker"),
            QStringLiteral("WMPNetworkSvc"),
            QStringLiteral("DPS"),
            QStringLiteral("WdiServiceHost"),
            QStringLiteral("WdiSystemHost"),
            QStringLiteral("RetailDemo"),
            QStringLiteral("RemoteRegistry"),
            QStringLiteral("Fax"),
            QStringLiteral("XblAuthManager"),
            QStringLiteral("XblGameSave"),
            QStringLiteral("XboxGipSvc"),
            QStringLiteral("XboxNetApiSvc"),
            QStringLiteral("Spooler"),
        };
        return kList;
    }

    static const QVector<RegTweak>& defaultPerformanceRegistryTweaks() {
        static const QVector<RegTweak> kTweaks = {
            // 0x26 is short, variable, foreground boost.
            {QStringLiteral("HKLM"),
             QStringLiteral("SYSTEM\\CurrentControlSet\\Control\\PriorityControl"),
             QStringLiteral("Win32PrioritySeparation"), true, 0x26, {}},

            {QStringLiteral("HKLM"),
             QStringLiteral("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile\\Tasks\\Games"),
             QStringLiteral("GPU Priority"), true, 8, {}},
            {QStringLiteral("HKLM"),
             QStringLiteral("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile\\Tasks\\Games"),
             QStringLiteral("Priority"), true, 6, {}},
            {QStringLiteral("HKLM"),
             QStringLiteral("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile\\Tasks\\Games"),
             QStringLiteral("Scheduling Category"), false, 0, QStringLiteral("High")},
            {QStringLiteral("HKLM"),
             QStringLiteral("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile\\Tasks\\Games"),
             QStringLiteral("SFIO Priority"), false, 0, QStringLiteral("High")},

            // The default of 20 reserves cycles for low-priority work.
            {QStringLiteral("HKLM"),
             QStringLiteral("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile"),
             QStringLiteral("SystemResponsiveness"), true, 0, {}},

            {QStringLiteral("HKCU"),
             QStringLiteral("SOFTWARE\\Microsoft\\GameBar"),
             QStringLiteral("UseNexusForGameBarEnabled"), true, 0, {}},
            {QStringLiteral("HKCU"),
             QStringLiteral("System\\GameConfigStore"),
             QStringLiteral("GameDVR_Enabled"), true, 0, {}},
            {QStringLiteral("HKLM"),
             QStringLiteral("SOFTWARE\\Policies\\Microsoft\\Windows\\GameDVR"),
             QStringLiteral("AllowGameDVR"), true, 0, {}},

            {QStringLiteral("HKCU"),
             QStringLiteral("Control Panel\\Mouse"),
             QStringLiteral("MouseSpeed"), false, 0, QStringLiteral("0")},
            {QStringLiteral("HKCU"),
             QStringLiteral("Control Panel\\Mouse"),
             QStringLiteral("MouseThreshold1"), false, 0, QStringLiteral("0")},
            {QStringLiteral("HKCU"),
             QStringLiteral("Control Panel\\Mouse"),
             QStringLiteral("MouseThreshold2"), false, 0, QStringLiteral("0")},
        };
        return kTweaks;
    }

    static const QVector<RegTweak>& defaultPrivacyTweaks() {
        static const QVector<RegTweak> kTweaks = {
            {QStringLiteral("HKLM"),
             QStringLiteral("SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection"),
             QStringLiteral("AllowTelemetry"), true, 0, {}},
            {QStringLiteral("HKLM"),
             QStringLiteral("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\DataCollection"),
             QStringLiteral("AllowTelemetry"), true, 0, {}},

            {QStringLiteral("HKLM"),
             QStringLiteral("SOFTWARE\\Policies\\Microsoft\\Windows\\AppCompat"),
             QStringLiteral("AITEnable"), true, 0, {}},
            {QStringLiteral("HKLM"),
             QStringLiteral("SOFTWARE\\Policies\\Microsoft\\Windows\\AppCompat"),
             QStringLiteral("DisableInventory"), true, 1, {}},

            {QStringLiteral("HKLM"),
             QStringLiteral("SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"),
             QStringLiteral("AllowCortana"), true, 0, {}},
            {QStringLiteral("HKLM"),
             QStringLiteral("SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"),
             QStringLiteral("DisableWebSearch"), true, 1, {}},

            {QStringLiteral("HKLM"),
             QStringLiteral("SOFTWARE\\Policies\\Microsoft\\Windows\\System"),
             QStringLiteral("EnableActivityFeed"), true, 0, {}},
            {QStringLiteral("HKLM"),
             QStringLiteral("SOFTWARE\\Policies\\Microsoft\\Windows\\System"),
             QStringLiteral("PublishUserActivities"), true, 0, {}},

            {QStringLiteral("HKCU"),
             QStringLiteral("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"),
             QStringLiteral("ContentDeliveryAllowed"), true, 0, {}},
            {QStringLiteral("HKCU"),
             QStringLiteral("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"),
             QStringLiteral("OemPreInstalledAppsEnabled"), true, 0, {}},
            {QStringLiteral("HKCU"),
             QStringLiteral("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"),
             QStringLiteral("PreInstalledAppsEnabled"), true, 0, {}},
            {QStringLiteral("HKCU"),
             QStringLiteral("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"),
             QStringLiteral("SilentInstalledAppsEnabled"), true, 0, {}},
            {QStringLiteral("HKCU"),
             QStringLiteral("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"),
             QStringLiteral("SystemPaneSuggestionsEnabled"), true, 0, {}},
        };
        return kTweaks;
    }

    QVector<ProcessEntry> enumerateProcesses() {
        QVector<ProcessEntry> out;
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return out;
        PROCESSENTRY32W pe{};
        pe.dwSize = sizeof(pe);
        if (Process32FirstW(snap, &pe)) {
            do {
                ProcessEntry e;
                e.name = QString::fromWCharArray(pe.szExeFile);
                e.pid  = pe.th32ProcessID;
                if (HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,
                                           FALSE, pe.th32ProcessID)) {
                    std::vector<wchar_t> buf(kImagePathChars, L'\0');
                    DWORD chars = kImagePathChars;
                    if (QueryFullProcessImageNameW(h, 0, buf.data(), &chars))
                        e.imagePath = QString::fromWCharArray(buf.data(),
                                                             static_cast<int>(chars));
                    CloseHandle(h);
                }
                out.append(std::move(e));
            } while (Process32NextW(snap, &pe));
        }
        CloseHandle(snap);
        return out;
    }

    static QVector<ServiceState> enumerateServiceStates() {
        QVector<ServiceState> out;
        ScmHandle scm(OpenSCManagerW(nullptr, nullptr,
                                     SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE));
        if (!scm) return out;

        DWORD needed = 0, count = 0, resume = 0;
        EnumServicesStatusExW(scm.get(), SC_ENUM_PROCESS_INFO, SERVICE_WIN32,
                              SERVICE_STATE_ALL, nullptr, 0,
                              &needed, &count, &resume, nullptr);
        if (needed == 0) return out;

        std::vector<BYTE> buf(needed);
        if (!EnumServicesStatusExW(scm.get(), SC_ENUM_PROCESS_INFO, SERVICE_WIN32,
                                   SERVICE_STATE_ALL, buf.data(), needed,
                                   &needed, &count, &resume, nullptr)) {
            return out;
        }

        auto* entries = reinterpret_cast<ENUM_SERVICE_STATUS_PROCESSW*>(buf.data());
        out.reserve(static_cast<int>(count));
        for (DWORD i = 0; i < count; ++i) {
            ServiceState s;
            s.name    = QString::fromWCharArray(entries[i].lpServiceName);
            s.running = entries[i].ServiceStatusProcess.dwCurrentState == SERVICE_RUNNING;

            ScmHandle svc(OpenServiceW(scm.get(), entries[i].lpServiceName,
                                       SERVICE_QUERY_CONFIG));
            if (svc) {
                DWORD cfgBytes = 0;
                QueryServiceConfigW(svc.get(), nullptr, 0, &cfgBytes);
                if (cfgBytes > 0) {
                    std::vector<BYTE> cfgBuf(cfgBytes);
                    auto* cfg = reinterpret_cast<QUERY_SERVICE_CONFIGW*>(cfgBuf.data());
                    if (QueryServiceConfigW(svc.get(), cfg, cfgBytes, &cfgBytes))
                        s.startType = cfg->dwStartType;
                }
            }
            out.append(std::move(s));
        }
        return out;
    }

    static Baseline captureBaseline() {
        Baseline b;
        b.capturedAt = QDateTime::currentDateTimeUtc();
        b.processes  = enumerateProcesses();
        b.services   = enumerateServiceStates();
        return b;
    }

    QStringList allowedTargets(const QStringList& requested,
                               const QStringList& exceptions) {
        QSet<QString> excepted;
        for (const auto& e : exceptions) {
            const QString t = e.trimmed();
            if (!t.isEmpty()) excepted.insert(t.toLower());
        }

        QStringList out;
        QSet<QString> seen;
        for (const auto& name : requested) {
            const QString target = name.trimmed();
            if (target.isEmpty()) continue;
            const QString key = target.toLower();
            if (seen.contains(key))     continue;
            if (excepted.contains(key)) { seen.insert(key); continue; }
            if (vth::win::processes::isProtected(target)) {
                qCWarning(catGenesis).noquote()
                    << "refused protected image:" << target
                    << "-" << vth::win::processes::protectedReason(target);
                seen.insert(key);
                continue;
            }
            seen.insert(key);
            out << target;
        }
        return out;
    }

    QStringList effectiveExceptions(const QStringList& userExceptions) {
        QStringList out = vth::win::processes::protectedNames();
        QSet<QString> seen;
        for (const auto& n : out) seen.insert(n.toLower());
        for (const auto& n : userExceptions) {
            const QString t = n.trimmed();
            if (t.isEmpty()) continue;
            const QString key = t.toLower();
            if (seen.contains(key)) continue;
            seen.insert(key);
            out << t;
        }
        return out;
    }

    static ManagerResult stopServices(const QStringList& names, State& state) {
        ManagerResult r;
        ScmHandle scm(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE));
        if (!scm) { r.ok = false; r.error = QStringLiteral("OpenSCManager failed."); return r; }
        for (const auto& n : names) {
            if (vth::win::services::isProtected(n)) {
                qCWarning(catGenesis).noquote()
                    << "refused protected service:" << n
                    << "-" << vth::win::services::protectedReason(n);
                r.failure << n;
                continue;
            }
            ServiceSnapshot snap;
            if (stopOneService(scm.get(), n, snap)) {
                r.success << n;
                state.servicesStopped.append(snap);

                TweakSnapshot t;
                t.domain            = QStringLiteral("genesis");
                t.kind              = SnapshotKind::Service;
                t.label             = QStringLiteral("Service %1 stopped").arg(n);
                t.serviceName       = snap.name;
                t.prevStartType     = snap.prevStartType;
                t.serviceWasRunning = snap.wasRunning;
                recordTweakSnapshot(std::move(t));
            } else {
                r.failure << n;
            }
        }
        return r;
    }

    static ManagerResult resumeServices(State& state) {
        ManagerResult r;
        ScmHandle scm(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));
        if (!scm) { r.ok = false; r.error = QStringLiteral("OpenSCManager failed."); return r; }
        // Failures stay in the state, so the next launch tries again.
        QVector<ServiceSnapshot> failed;
        for (const auto& snap : state.servicesStopped) {
            if (restartOneService(scm.get(), snap)) {
                r.success << snap.name;
            } else {
                r.failure << snap.name;
                failed.append(snap);
            }
        }
        state.servicesStopped = failed;
        return r;
    }

    ManagerResult closeProcesses(const QStringList& names, const QStringList& exceptions) {
        ManagerResult r;
        const QStringList running = enumerateProcessNames();
        for (const auto& target : allowedTargets(names, exceptions)) {
            bool present = false;
            for (const auto& runningName : running)
                if (runningName.compare(target, Qt::CaseInsensitive) == 0) { present = true; break; }
            if (!present) continue;
            if (killProcessByName(target)) r.success << target;
            else                           r.failure << target;
        }
        return r;
    }

    static ManagerResult suspendProcesses(const QStringList& names, const QStringList& exceptions,
                                          State& state) {
        ManagerResult r;
        const QStringList running = enumerateProcessNames();
        for (const auto& target : allowedTargets(names, exceptions)) {
            bool present = false;
            for (const auto& runningName : running)
                if (runningName.compare(target, Qt::CaseInsensitive) == 0) { present = true; break; }
            if (!present) continue;
            if (suspendResumeByName(target, true)) {
                r.success << target;
                state.suspendedProcessNames << target;
            } else {
                r.failure << target;
            }
        }
        return r;
    }

    static ManagerResult resumeProcesses(State& state) {
        ManagerResult r;
        for (const auto& name : state.suspendedProcessNames) {
            if (suspendResumeByName(name, false)) r.success << name;
            else                                  r.failure << name;
        }
        state.suspendedProcessNames.clear();
        return r;
    }

    static bool setProcessorIdleDisabled(bool disabled) {
        // PROCIDLEDISABLE
        const QString idleGuid = QStringLiteral("5d76a2ca-e8c0-402f-a133-2158492d58ad");
        auto run = [](const QStringList& args) {
            QProcess p;
            p.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* a) {
                a->flags |= 0x08000000;
            });
            p.start(system32(L"powercfg.exe"), args);
            if (!p.waitForStarted(10000)) return false;
            if (!p.waitForFinished(15000)) { p.kill(); return false; }
            return p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0;
        };
        const bool set = run({ QStringLiteral("/setacvalueindex"), QStringLiteral("scheme_current"),
                               QStringLiteral("sub_processor"), idleGuid,
                               disabled ? QStringLiteral("1") : QStringLiteral("0") });
        const bool act = run({ QStringLiteral("/setactive"), QStringLiteral("scheme_current") });
        return set && act;
    }

    static ManagerResult applyRegistryTweaks(const QVector<RegTweak>& tweaks, State& state) {
        ManagerResult r;
        for (const auto& t : tweaks) {
            RegSnapshot snap;
            snapshotRegValue(t, snap);
            if (writeRegValue(t)) {
                state.registrySnapshots.append(snap);
                const QString labelPath = QStringLiteral("%1\\%2:%3").arg(t.hive, t.subkey, t.name);
                r.success << labelPath;
                recordInTracker(snap, labelPath);
            } else {
                r.failure << QStringLiteral("%1\\%2:%3").arg(t.hive, t.subkey, t.name);
            }
        }
        return r;
    }

    static ManagerResult revertRegistryTweaks(State& state) {
        ManagerResult r;
        // Failures stay in the state, so the next launch tries again.
        QVector<RegSnapshot> failed;
        for (auto it = state.registrySnapshots.rbegin();
             it != state.registrySnapshots.rend(); ++it) {
            if (restoreRegValue(*it)) {
                r.success << QStringLiteral("%1\\%2:%3").arg(it->hive, it->subkey, it->name);
            } else {
                r.failure << QStringLiteral("%1\\%2:%3").arg(it->hive, it->subkey, it->name);
                failed.prepend(*it);
            }
        }
        state.registrySnapshots = failed;
        return r;
    }

    ManagerResult purgeStandbyList() {
        ManagerResult r;
        auto fn = resolveNtSetSystemInformation();
        if (!fn) { r.ok = false; r.error = QStringLiteral("NtSetSystemInformation unavailable."); return r; }

        HANDLE token = nullptr;
        if (OpenProcessToken(GetCurrentProcess(),
                             TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
            TOKEN_PRIVILEGES tp{};
            tp.PrivilegeCount = 1;
            tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            if (LookupPrivilegeValueW(nullptr, SE_PROF_SINGLE_PROCESS_NAME,
                                      &tp.Privileges[0].Luid)) {
                AdjustTokenPrivileges(token, FALSE, &tp, sizeof(tp), nullptr, nullptr);
            }
            CloseHandle(token);
        }

        int ec = MemoryEmptyWorkingSets;
        fn(kSystemMemoryListInformation, &ec, sizeof(ec));
        int sb = MemoryPurgeStandbyList;
        LONG status = fn(kSystemMemoryListInformation, &sb, sizeof(sb));
        if (status != 0) {
            r.ok = false;
            r.error = QStringLiteral("Standby-list purge returned 0x%1 - run elevated.")
                          .arg(static_cast<quint32>(status), 8, 16, QChar::fromLatin1('0'));
            return r;
        }
        r.success << QStringLiteral("standby");
        return r;
    }

    namespace
    {
        const QVector<RegTweak>& notificationOffTweaks() {
            static const QVector<RegTweak> off = {
                {QStringLiteral("HKCU"),
                 QStringLiteral("SOFTWARE\\Policies\\Microsoft\\Windows\\Explorer"),
                 QStringLiteral("DisableNotificationCenter"), true, 1, {}},
                {QStringLiteral("HKCU"),
                 QStringLiteral("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PushNotifications"),
                 QStringLiteral("ToastEnabled"), true, 0, {}},
                {QStringLiteral("HKCU"),
                 QStringLiteral("Software\\Microsoft\\Windows\\CurrentVersion\\Notifications\\Settings"),
                 QStringLiteral("NOC_GLOBAL_SETTING_ALLOW_TOASTS_ABOVE_LOCK"), true, 0, {}},
                {QStringLiteral("HKCU"),
                 QStringLiteral("Software\\Microsoft\\Windows\\CurrentVersion\\Notifications\\Settings"),
                 QStringLiteral("NOC_GLOBAL_SETTING_ALLOW_CRITICAL_TOASTS_ABOVE_LOCK"), true, 0, {}},
            };
            return off;
        }

        bool isNotificationSnapshot(const RegSnapshot& s) {
            for (const RegTweak& t : notificationOffTweaks())
                if (s.hive.compare(t.hive, Qt::CaseInsensitive) == 0
                    && s.subkey.compare(t.subkey, Qt::CaseInsensitive) == 0
                    && s.name.compare(t.name, Qt::CaseInsensitive) == 0)
                    return true;
            return false;
        }
    }

    static ManagerResult disableNotifications(State& state) {
        ManagerResult r;
        auto sub = applyRegistryTweaks(notificationOffTweaks(), state);
        state.notificationsDisabled = true;
        r.success = sub.success;
        r.failure = sub.failure;
        return r;
    }

    // Apps like the NVIDIA App fail when push notifications are left off. A value
    // that won't restore keeps its snapshot and the flag stays set.
    static ManagerResult enableNotifications(State& state) {
        ManagerResult r;
        QVector<RegSnapshot> keep;
        bool allRestored = true;
        for (auto it = state.registrySnapshots.rbegin();
             it != state.registrySnapshots.rend(); ++it) {
            if (!isNotificationSnapshot(*it)) {
                keep.prepend(*it);
                continue;
            }
            const QString label = QStringLiteral("%1\\%2:%3").arg(it->hive, it->subkey, it->name);
            if (restoreRegValue(*it)) {
                r.success << label;
            } else {
                r.failure << label;
                keep.prepend(*it);
                allRestored = false;
            }
        }
        state.registrySnapshots = keep;
        if (allRestored)
            state.notificationsDisabled = false;
        return r;
    }

    static ManagerResult suspendExplorer(State& state) {
        ManagerResult r;
        // Otherwise Winlogon starts explorer.exe again straight away.
        const RegTweak ars = {
            QStringLiteral("HKLM"),
            QStringLiteral("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"),
            QStringLiteral("AutoRestartShell"), true, 0, {}};
        RegSnapshot snap;
        snapshotRegValue(ars, snap);
        state.registrySnapshots.append(snap);
        // Saved first, so a crash after the kill can still bring the shell back.
        state.explorerSuspended = true;
        if (!savePersistedState(state)) {
            state.explorerSuspended = false;
            state.registrySnapshots.removeLast();
            r.ok = false;
            r.error = QStringLiteral("Could not save the Genesis state, so Explorer was left running.");
            return r;
        }
        writeRegValue(ars);

        if (!killProcessByName(QStringLiteral("explorer.exe"))) {
            state.explorerSuspended = false;
            r.ok = false;
            r.error = QStringLiteral("Could not stop explorer.exe.");
            return r;
        }
        r.success << QStringLiteral("explorer.exe");
        return r;
    }

    static ManagerResult resumeExplorer(State& state) {
        ManagerResult r;
        if (!state.explorerSuspended) return r;

        // Already running after a restart, and a second one opens a File Explorer window.
        if (enumerateProcessNames().contains(QStringLiteral("explorer.exe"),
                                             Qt::CaseInsensitive)) {
            state.explorerSuspended = false;
            r.success << QStringLiteral("explorer.exe");
            return r;
        }

        wchar_t winDir[MAX_PATH] = {};
        GetWindowsDirectoryW(winDir, MAX_PATH);
        const std::wstring exe = std::wstring(winDir) + L"\\explorer.exe";
        STARTUPINFOW si{};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi{};
        if (CreateProcessW(exe.c_str(), nullptr, nullptr, nullptr, FALSE,
                           0, nullptr, nullptr, &si, &pi)) {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            r.success << QStringLiteral("explorer.exe");
        } else {
            r.failure << QStringLiteral("explorer.exe");
        }
        state.explorerSuspended = false;
        return r;
    }

    namespace
    {
        const QString& suppressedSuffix() {
            static const QString kSuffix = QStringLiteral(".vethida-off");
            return kSuffix;
        }

        QString nativeCleanPath(const QString& path) {
            return QDir::toNativeSeparators(QDir::cleanPath(path));
        }

        // Empty when the path may be renamed; otherwise the reason it may not.
        QString renameRefusal(const QString& nativePath, const QString& imageName) {
            if (nativePath.isEmpty())
                return QStringLiteral("no image path");
            if (vth::win::processes::isProtected(imageName))
                return QStringLiteral("protected image");

            const QFileInfo info(nativePath);
            if (!info.exists() || !info.isFile())
                return QStringLiteral("not a file");
            if (info.isSymLink())
                return QStringLiteral("a link rather than the file itself");

            wchar_t winDir[MAX_PATH] = {};
            if (GetWindowsDirectoryW(winDir, MAX_PATH) > 0) {
                const QString windows = nativeCleanPath(QString::fromWCharArray(winDir));
                if (nativePath.startsWith(windows + QDir::separator(), Qt::CaseInsensitive))
                    return QStringLiteral("inside the Windows directory");
            }
            if (nativePath.contains(QStringLiteral("\\WindowsApps\\"), Qt::CaseInsensitive))
                return QStringLiteral("a packaged app");

            const QString install = nativeCleanPath(paths::installDir());
            if (!install.isEmpty()
                && nativePath.startsWith(install + QDir::separator(), Qt::CaseInsensitive))
                return QStringLiteral("inside the Vethida installation");

            if (QFileInfo::exists(nativePath + suppressedSuffix()))
                return QStringLiteral("a suffixed file is already there");

            return {};
        }

        // True when originalPath holds a runnable file after the call.
        bool moveRecordBack(const RenameRecord& rec) {
            // Failed validation when it was read.
            if (rec.originalPath.isEmpty() || rec.renamedPath.isEmpty())
                return true;
            const bool suffixed = QFileInfo::exists(rec.renamedPath);
            const bool original = QFileInfo::exists(rec.originalPath);

            if (!suffixed) {
                // A crash before the move, or something else already moved it.
                return original;
            }
            if (original) {
                // The program updated itself during the session. The suffixed file is
                // stale, but it's logged and left in place.
                qCWarning(catGenesis).noquote()
                    << "left in place, the program reappeared at its own path:"
                    << rec.renamedPath;
                return true;
            }
            const std::wstring from = rec.renamedPath.toStdWString();
            const std::wstring to   = rec.originalPath.toStdWString();
            if (MoveFileW(from.c_str(), to.c_str()))
                return true;
            qCWarning(catGenesis).noquote()
                << "could not move back:" << rec.renamedPath
                << "err" << GetLastError();
            return false;
        }
    }

    static ManagerResult suppressExecutableNames(const QStringList& allowedNames,
                                                 const Baseline&    baseline,
                                                 State&             state) {
        ManagerResult r;

        QSet<QString> want;
        for (const auto& n : allowedNames) want.insert(n.trimmed().toLower());
        if (want.isEmpty()) return r;

        // One program often has several processes.
        QSet<QString> handled;
        for (const ProcessEntry& p : baseline.processes) {
            if (!want.contains(p.name.toLower())) continue;

            const QString native = nativeCleanPath(p.imagePath);
            const QString key    = native.toLower();
            if (handled.contains(key)) continue;
            handled.insert(key);

            if (const QString refusal = renameRefusal(native, p.name); !refusal.isEmpty()) {
                qCWarning(catGenesis).noquote()
                    << "rename refused:" << p.name << refusal;
                r.failure << p.name;
                continue;
            }

            RenameRecord rec;
            rec.imageName    = p.name;
            rec.originalPath = native;
            rec.renamedPath  = native + suppressedSuffix();
            rec.stage        = RenameStage::Pending;

            // The record must be on disk before the move, or a crash leaves a renamed
            // file that nothing knows about. Don't reorder this.
            state.renames.append(rec);
            if (!savePersistedState(state)) {
                qCWarning(catGenesis).noquote() << "rename skipped, no record saved:" << native;
                state.renames.removeLast();
                r.failure << p.name;
                continue;
            }

            const std::wstring from = rec.originalPath.toStdWString();
            const std::wstring to   = rec.renamedPath.toStdWString();
            if (MoveFileW(from.c_str(), to.c_str())) {
                state.renames.last().stage = RenameStage::Renamed;
                r.success << p.name;
            } else {
                qCWarning(catGenesis).noquote()
                    << "rename failed:" << native << "err" << GetLastError();
                state.renames.removeLast();
                r.failure << p.name;
            }
        }

        savePersistedState(state);
        return r;
    }

    // Failures stay in the state, so the next launch tries again.
    static ManagerResult restoreExecutableNames(State& state) {
        ManagerResult r;
        if (state.renames.isEmpty()) return r;

        QVector<RenameRecord> stillOut;
        for (auto it = state.renames.crbegin(); it != state.renames.crend(); ++it) {
            if (moveRecordBack(*it)) {
                r.success << it->imageName;
            } else {
                r.failure << it->renamedPath;
                stillOut.prepend(*it);
            }
        }
        state.renames = stillOut;
        return r;
    }

    static quint32 requestTimerResolution(quint32 hundredNs) {
        auto fn = resolveNtSetTimerResolution();
        if (!fn) return 0;
        ULONG actual = 0;
        if (fn(hundredNs, TRUE, &actual) != 0) return 0;
        return actual;
    }

    static void releaseTimerResolution(quint32 hundredNs) {
        auto fn = resolveNtSetTimerResolution();
        if (!fn) return;
        ULONG actual = 0;
        fn(hundredNs, FALSE, &actual);
    }

    static bool enableDwmMmcss(bool on) {
        return SUCCEEDED(DwmEnableMMCSS(on ? TRUE : FALSE));
    }

    static int trimWorkingSets() {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return 0;
        PROCESSENTRY32W pe{};
        pe.dwSize = sizeof(pe);
        int trimmed = 0;
        if (Process32FirstW(snap, &pe)) {
            do {
                HANDLE h = OpenProcess(PROCESS_SET_QUOTA | PROCESS_QUERY_INFORMATION,
                                       FALSE, pe.th32ProcessID);
                if (h) {
                    if (SetProcessWorkingSetSize(h, static_cast<SIZE_T>(-1),
                                                    static_cast<SIZE_T>(-1))) {
                        ++trimmed;
                    }
                    CloseHandle(h);
                }
            } while (Process32NextW(snap, &pe));
        }
        CloseHandle(snap);
        return trimmed;
    }

    namespace
    {
        bool setProcessThrottle(DWORD pid, bool disableThrottle) {
            HANDLE h = OpenProcess(PROCESS_SET_INFORMATION, FALSE, pid);
            if (!h) return false;
            PROCESS_POWER_THROTTLING_STATE st{};
            st.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
            // A clear ControlMask goes back to system-managed.
            st.ControlMask = disableThrottle ? PROCESS_POWER_THROTTLING_EXECUTION_SPEED : 0;
            st.StateMask   = 0;
            const BOOL ok = SetProcessInformation(h, ProcessPowerThrottling, &st, sizeof(st));
            CloseHandle(h);
            return ok != FALSE;
        }

        int throttleByName(const QStringList& exeNames, bool disableThrottle) {
            if (exeNames.isEmpty()) return 0;
            QSet<QString> want;
            for (const auto& n : exeNames) want.insert(n.toLower());
            HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (snap == INVALID_HANDLE_VALUE) return 0;
            PROCESSENTRY32W pe{};
            pe.dwSize = sizeof(pe);
            int touched = 0;
            if (Process32FirstW(snap, &pe)) {
                do {
                    if (!want.contains(QString::fromWCharArray(pe.szExeFile).toLower())) continue;
                    if (setProcessThrottle(pe.th32ProcessID, disableThrottle)) ++touched;
                } while (Process32NextW(snap, &pe));
            }
            CloseHandle(snap);
            return touched;
        }

        // Empty if it's Vethida or the shell, which are never the game.
        QString foregroundExe() {
            HWND fg = GetForegroundWindow();
            if (!fg) return {};
            DWORD pid = 0;
            GetWindowThreadProcessId(fg, &pid);
            if (!pid) return {};
            HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
            if (!h) return {};
            wchar_t buf[MAX_PATH];
            DWORD sz = MAX_PATH;
            QString exe;
            if (QueryFullProcessImageNameW(h, 0, buf, &sz))
                exe = QFileInfo(QString::fromWCharArray(buf, sz)).fileName();
            CloseHandle(h);
            const QString low = exe.toLower();
            if (low.isEmpty() || low == QStringLiteral("vethida.exe")
                || low == QStringLiteral("explorer.exe"))
                return {};
            return exe;
        }
    }

    // Targets the foreground process if the list is empty.
    static ManagerResult disablePowerThrottling(const QStringList& targetExes, State& state) {
        ManagerResult res;
        QStringList targets = targetExes;
        targets.removeAll(QString{});
        if (targets.isEmpty()) {
            const QString fg = foregroundExe();
            if (!fg.isEmpty()) targets << fg;
        }
        if (targets.isEmpty()) return res;

        throttleByName(targets, /*disableThrottle=*/true);
        for (const auto& t : targets) {
            if (!state.powerThrottlingDisabledFor.contains(t, Qt::CaseInsensitive))
                state.powerThrottlingDisabledFor << t;
            res.success << t;
        }
        return res;
    }

    static ManagerResult restorePowerThrottling(State& state) {
        ManagerResult res;
        if (state.powerThrottlingDisabledFor.isEmpty()) return res;
        throttleByName(state.powerThrottlingDisabledFor, /*disableThrottle=*/false);
        res.success = state.powerThrottlingDisabledFor;
        state.powerThrottlingDisabledFor.clear();
        return res;
    }

    // Removes non-present devices. Root-enumerated ones are skipped, because they can
    // load on demand, like virtual audio loopbacks.
    static void cleanupGhostDevices() {
        QVector<vth::win::devices::GhostDevice> safeBatch;
        for (const auto& d : vth::win::devices::enumerateGhostDevices()) {
            if (!d.isNonPnp())
                safeBatch.append(d);
        }
        if (!safeBatch.isEmpty())
            vth::win::devices::removeGhostDevices(safeBatch);
    }

    namespace
    {
        // Relative to Control\Class.
        QStringList enumerateNvidiaAdapterSubkeys() {
            QStringList out;
            HKEY classKey = nullptr;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                    L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}",
                    0, KEY_READ | KEY_ENUMERATE_SUB_KEYS, &classKey) != ERROR_SUCCESS) {
                return out;
            }
            for (DWORD i = 0;; ++i) {
                wchar_t name[64] = {};
                DWORD nameLen = 64;
                const LSTATUS rc = RegEnumKeyExW(classKey, i, name, &nameLen,
                                                 nullptr, nullptr, nullptr, nullptr);
                if (rc == ERROR_NO_MORE_ITEMS) break;
                if (rc != ERROR_SUCCESS) continue;

                const QString sub = QString::fromWCharArray(name, static_cast<int>(nameLen));
                bool numeric = !sub.isEmpty();
                for (QChar c : sub) { if (!c.isDigit()) { numeric = false; break; } }
                if (!numeric) continue;

                HKEY adapter = nullptr;
                if (RegOpenKeyExW(classKey, name, 0, KEY_READ, &adapter) != ERROR_SUCCESS) continue;
                wchar_t devBuf[256] = {};
                DWORD devSize = sizeof(devBuf);
                DWORD type = 0;
                bool isNvidia = false;
                if (RegQueryValueExW(adapter, L"MatchingDeviceId", nullptr, &type,
                                     reinterpret_cast<LPBYTE>(devBuf), &devSize) == ERROR_SUCCESS) {
                    const QString did = QString::fromWCharArray(devBuf).toUpper();
                    if (did.contains(QStringLiteral("VEN_10DE"))) isNvidia = true;
                }
                RegCloseKey(adapter);
                if (isNvidia) {
                    out << QStringLiteral("{4d36e968-e325-11ce-bfc1-08002be10318}\\") + sub;
                }
            }
            RegCloseKey(classKey);
            return out;
        }

        struct NvidiaPerfValue {
            const wchar_t* name;
            quint32        value;
        };
        const QVector<NvidiaPerfValue>& nvidiaPerfValues() {
            static const QVector<NvidiaPerfValue> kValues = {
                {L"DisableDynamicPstate",         1},        // force P0
                {L"RMDisableGPUIdlePowerSaving",  1},
                {L"PowerMizerEnable",             1},
                {L"PowerMizerLevel",              1},        // max performance
                {L"PowerMizerLevelAC",            1},
                {L"RMEnablePreemption",           0},        // disable preemption
                {L"PerfLevelSrc",                 0x2222},
            };
            return kValues;
        }
    }

    // "Prefer maximum performance" for every NVIDIA adapter. The registry values are
    // read at the next driver init, and the base profile is what the control panel shows.
    static void applyNvidiaPerfMode(State& state) {
        const QStringList adapters = enumerateNvidiaAdapterSubkeys();
        if (adapters.isEmpty())
            return;

        for (const auto& sub : adapters) {
            const QString fullSub = QStringLiteral("SYSTEM\\CurrentControlSet\\Control\\Class\\")
                                  + sub;
            for (const auto& v : nvidiaPerfValues()) {
                RegTweak t;
                t.hive       = QStringLiteral("HKLM");
                t.subkey     = fullSub;
                t.name       = QString::fromWCharArray(v.name);
                t.isDword    = true;
                t.dwordValue = v.value;

                RegSnapshot snap;
                snapshotRegValue(t, snap);
                if (writeRegValue(t)) {
                    state.registrySnapshots.append(snap);
                    recordInTracker(snap, QStringLiteral("nvidia-perf %1\\%2").arg(sub, t.name));
                } else {
                    qCWarning(catGenesis).noquote() << "nvidia-perf failed:" << sub << t.name;
                }
            }
        }

        const auto nv = vth::win::nvapi::applyBasePerfProfile();
        if (nv.ok)
            state.drsBasePerfApplied = true;
        if (!nv.error.isEmpty())
            qCWarning(catGenesis).noquote() << "nvapi:" << nv.error;
    }

    ActivateResult activate(const ActivateOptions& opts,
                            const QStringList&     closeList,
                            const QStringList&     exceptions,
                            State&                 stateOut) {
        ActivateResult r;
        if (!vth::meetsTier(vth::models::ProductTier::Ultimate)) {
            r.error = QStringLiteral("Genesis needs an active Vethida Ultimate license.");
            return r;
        }
        if (stateOut.active) {
            r.error = QStringLiteral("Genesis is already active.");
            return r;
        }

        const vth::TweakBatch batch(vth::newTweakBatchId(QStringLiteral("genesis")));

        // The record must be on disk before anything changes, because the next launch
        // reads it to undo a session.
        QString prevGuid;
        for (const auto& p : vth::win::power::enumeratePlans()) {
            if (p.active) { prevGuid = p.guid; break; }
        }

        stateOut.baseline              = captureBaseline();
        stateOut.startedAt             = QDateTime::currentDateTimeUtc();
        stateOut.previousPowerPlanGuid = prevGuid;
        stateOut.active                = true;
        if (!savePersistedState(stateOut)) {
            stateOut.active = false;
            r.error = QStringLiteral("Genesis could not save its state, so nothing was changed.");
            return r;
        }

        const QString highPerf = vth::win::power::WellKnownPlans::highPerformance();
        if (!highPerf.isEmpty()) vth::win::power::setActivePlan(highPerf);

        const QStringList closeTargets = allowedTargets(closeList, exceptions);

        if (opts.killProcessList) {
            const auto closed = closeProcesses(closeTargets, exceptions);
            stateOut.processesKilled = closed.success.size();

            // After closing, and only what was closed, because a rename doesn't stop
            // what's already running.
            if (opts.renameExecutables)
                suppressExecutableNames(closed.success, stateOut.baseline, stateOut);
        }

        if (opts.suspendProcessList) {
            suspendProcesses(defaultProcessSuspendList(), exceptions, stateOut);
            savePersistedState(stateOut);
        }

        if (opts.stopServices) {
            stopServices(defaultServiceStopList(), stateOut);
            savePersistedState(stateOut);
        }

        if (opts.applyRegistryTweaks) {
            applyRegistryTweaks(defaultPerformanceRegistryTweaks(), stateOut);
            savePersistedState(stateOut);
        }

        if (opts.applyPrivacyTweaks) {
            applyRegistryTweaks(defaultPrivacyTweaks(), stateOut);
            savePersistedState(stateOut);
        }

        if (opts.purgeStandbyList)
            purgeStandbyList();

        if (opts.disableNotifications) {
            disableNotifications(stateOut);
            savePersistedState(stateOut);
        }

        if (opts.suspendExplorer)
            suspendExplorer(stateOut);

        if (opts.runGhostDeviceCleanup)
            cleanupGhostDevices();

        if (opts.applyNvidiaPerfMode) {
            applyNvidiaPerfMode(stateOut);
            savePersistedState(stateOut);
        }

        if (opts.disableProcessorIdle) {
            // Recorded first, because the plan may change even if the call reports failure.
            stateOut.processorIdleDisabled = true;
            savePersistedState(stateOut);
            if (!setProcessorIdleDisabled(true))
                stateOut.processorIdleDisabled = false;
        }

        if (opts.requestTimerResolution) {
            const quint32 actual = requestTimerResolution(5000);   // 0.5 ms
            stateOut.timerResolutionRequested = actual ? 5000 : 0;
        }
        if (opts.enableMmcss && enableDwmMmcss(true))
            stateOut.mmcssEnabled = true;
        if (opts.trimWorkingSets)
            trimWorkingSets();
        if (opts.disablePowerThrottling)
            disablePowerThrottling(QStringList{ opts.targetExe }, stateOut);

        savePersistedState(stateOut);

        r.ok = true;
        return r;
    }

    bool hasUnfinishedSession(const State& state) {
        return state.active
            || !state.renames.isEmpty()
            || !state.servicesStopped.isEmpty()
            || !state.registrySnapshots.isEmpty()
            || state.notificationsDisabled
            || state.processorIdleDisabled
            || state.drsBasePerfApplied;
    }

    bool deactivate(State& state) {
        if (!hasUnfinishedSession(state)) return false;

        // Explorer first, so the desktop is back during the rest.
        if (state.explorerSuspended) resumeExplorer(state);

        if (!state.suspendedProcessNames.isEmpty()) resumeProcesses(state);

        restoreExecutableNames(state);

        if (state.processorIdleDisabled) {
            if (setProcessorIdleDisabled(false)) state.processorIdleDisabled = false;
        }

        if (state.notificationsDisabled) enableNotifications(state);

        resumeServices(state);

        revertRegistryTweaks(state);

        if (state.timerResolutionRequested) {
            releaseTimerResolution(state.timerResolutionRequested);
            state.timerResolutionRequested = 0;
        }
        if (state.mmcssEnabled) {
            enableDwmMmcss(false);
            state.mmcssEnabled = false;
        }
        if (!state.powerThrottlingDisabledFor.isEmpty()) {
            restorePowerThrottling(state);
        }

        // Cleared so a later retry can't undo a plan the user has picked since.
        if (!state.previousPowerPlanGuid.isEmpty()
            && vth::win::power::setActivePlan(state.previousPowerPlanGuid)) {
            state.previousPowerPlanGuid.clear();
        }

        if (state.drsBasePerfApplied) {
            const auto nv = vth::win::nvapi::revertBasePerfProfile();
            if (nv.ok) state.drsBasePerfApplied = false;
        }

        state.active          = false;
        state.processesKilled = 0;
        state.startedAt       = {};
        state.baseline = {};
        savePersistedState(state);

        if (auto* tracker = ServiceRegistry::instance().get<TweakStateTracker>()) {
            tracker->clearDomain(QStringLiteral("genesis"));
        }
        return true;
    }

    namespace
    {
        QJsonObject toJson(const ServiceSnapshot& s) {
            QJsonObject o;
            o[QStringLiteral("name")]       = s.name;
            o[QStringLiteral("prevStart")]  = static_cast<int>(s.prevStartType);
            o[QStringLiteral("wasRunning")] = s.wasRunning;
            return o;
        }
        ServiceSnapshot serviceFromJson(const QJsonObject& o) {
            ServiceSnapshot s;
            s.name          = o.value(QStringLiteral("name")).toString();
            s.prevStartType = static_cast<quint32>(o.value(QStringLiteral("prevStart")).toInt());
            s.wasRunning    = o.value(QStringLiteral("wasRunning")).toBool();
            return s;
        }

        QJsonObject toJson(const RegSnapshot& s) {
            QJsonObject o;
            o[QStringLiteral("hive")]         = s.hive;
            o[QStringLiteral("subkey")]       = s.subkey;
            o[QStringLiteral("name")]         = s.name;
            o[QStringLiteral("existed")]      = s.existedBefore;
            o[QStringLiteral("isDword")]      = s.isDword;
            o[QStringLiteral("prevDword")]    = static_cast<qint64>(s.prevDword);
            o[QStringLiteral("prevString")]   = s.prevString;
            return o;
        }
        RegSnapshot regFromJson(const QJsonObject& o) {
            RegSnapshot s;
            s.hive          = o.value(QStringLiteral("hive")).toString();
            s.subkey        = o.value(QStringLiteral("subkey")).toString();
            s.name          = o.value(QStringLiteral("name")).toString();
            s.existedBefore = o.value(QStringLiteral("existed")).toBool();
            s.isDword       = o.value(QStringLiteral("isDword")).toBool();
            s.prevDword     = static_cast<quint32>(o.value(QStringLiteral("prevDword")).toVariant().toLongLong());
            s.prevString    = o.value(QStringLiteral("prevString")).toString();
            return s;
        }

        QJsonObject toJson(const ProcessEntry& p) {
            QJsonObject o;
            o[QStringLiteral("name")] = p.name;
            o[QStringLiteral("path")] = p.imagePath;
            o[QStringLiteral("pid")]  = static_cast<qint64>(p.pid);
            return o;
        }
        ProcessEntry processFromJson(const QJsonObject& o) {
            ProcessEntry p;
            p.name      = o.value(QStringLiteral("name")).toString();
            p.imagePath = o.value(QStringLiteral("path")).toString();
            p.pid       = static_cast<quint32>(o.value(QStringLiteral("pid")).toVariant().toLongLong());
            return p;
        }

        QJsonObject toJson(const ServiceState& s) {
            QJsonObject o;
            o[QStringLiteral("name")]    = s.name;
            o[QStringLiteral("start")]   = static_cast<int>(s.startType);
            o[QStringLiteral("running")] = s.running;
            return o;
        }
        ServiceState serviceStateFromJson(const QJsonObject& o) {
            ServiceState s;
            s.name      = o.value(QStringLiteral("name")).toString();
            s.startType = static_cast<quint32>(o.value(QStringLiteral("start")).toInt());
            s.running   = o.value(QStringLiteral("running")).toBool();
            return s;
        }

        QJsonObject toJson(const RenameRecord& r) {
            QJsonObject o;
            o[QStringLiteral("image")]    = r.imageName;
            o[QStringLiteral("from")]     = r.originalPath;
            o[QStringLiteral("to")]       = r.renamedPath;
            o[QStringLiteral("renamed")]  = (r.stage == RenameStage::Renamed);
            return o;
        }
        RenameRecord renameFromJson(const QJsonObject& o) {
            RenameRecord r;
            r.imageName    = o.value(QStringLiteral("image")).toString();
            r.originalPath = o.value(QStringLiteral("from")).toString();
            r.renamedPath  = o.value(QStringLiteral("to")).toString();
            // Anything else wasn't written here, and acting on it would move files as
            // an administrator.
            if (r.renamedPath != r.originalPath + suppressedSuffix()) {
                r.originalPath.clear();
                r.renamedPath.clear();
            }
            r.stage        = o.value(QStringLiteral("renamed")).toBool()
                                 ? RenameStage::Renamed
                                 : RenameStage::Pending;
            return r;
        }

        QJsonObject toJson(const Baseline& b) {
            QJsonObject o;
            o[QStringLiteral("capturedAt")] = b.capturedAt.toString(Qt::ISODate);
            QJsonArray procs;
            for (const auto& p : b.processes) procs.append(toJson(p));
            o[QStringLiteral("processes")] = procs;
            QJsonArray svcs;
            for (const auto& s : b.services) svcs.append(toJson(s));
            o[QStringLiteral("services")] = svcs;
            return o;
        }
        Baseline baselineFromJson(const QJsonObject& o) {
            Baseline b;
            b.capturedAt = QDateTime::fromString(
                o.value(QStringLiteral("capturedAt")).toString(), Qt::ISODate);
            const auto procs = o.value(QStringLiteral("processes")).toArray();
            b.processes.reserve(procs.size());
            for (const auto& v : procs) b.processes.append(processFromJson(v.toObject()));
            const auto svcs = o.value(QStringLiteral("services")).toArray();
            b.services.reserve(svcs.size());
            for (const auto& v : svcs) b.services.append(serviceStateFromJson(v.toObject()));
            return b;
        }
    }

    ActivateOptions optionsFromSettings() {
        ActivateOptions o;
        auto* settings = ServiceRegistry::instance().get<vth::Settings>();
        if (!settings)
            return o;
        const auto get = [settings](const char* key, bool fallback) {
            return settings->value(QString::fromLatin1(key), fallback).toBool();
        };
        o.killProcessList         = get("genesis/opts/kill",            o.killProcessList);
        o.suspendProcessList      = get("genesis/opts/suspendProcs",    o.suspendProcessList);
        o.stopServices            = get("genesis/opts/stopServices",    o.stopServices);
        o.applyRegistryTweaks     = get("genesis/opts/registry",        o.applyRegistryTweaks);
        o.applyPrivacyTweaks      = get("genesis/opts/privacy",         o.applyPrivacyTweaks);
        o.purgeStandbyList        = get("genesis/opts/standby",         o.purgeStandbyList);
        o.disableNotifications    = get("genesis/opts/notifications",   o.disableNotifications);
        o.suspendExplorer         = get("genesis/opts/suspendExplorer", o.suspendExplorer);
        o.requestTimerResolution  = get("genesis/opts/timer",           o.requestTimerResolution);
        o.enableMmcss             = get("genesis/opts/mmcss",           o.enableMmcss);
        o.trimWorkingSets         = get("genesis/opts/trim",            o.trimWorkingSets);
        o.runGhostDeviceCleanup   = get("genesis/opts/ghostDevices",    o.runGhostDeviceCleanup);
        o.applyNvidiaPerfMode     = get("genesis/opts/nvidiaPerfMode",  o.applyNvidiaPerfMode);
        o.disableProcessorIdle    = get("genesis/opts/processorIdle",   o.disableProcessorIdle);
        o.renameExecutables       = get("genesis/opts/renameExes",      o.renameExecutables);
        o.disablePowerThrottling  = get("genesis/opts/powerThrottling", o.disablePowerThrottling);
        return o;
    }

    namespace
    {
        QStringList listFromSettings(const char* key, const QStringList& fallback) {
            auto* settings = ServiceRegistry::instance().get<vth::Settings>();
            if (!settings) return fallback;
            const QString k = QString::fromLatin1(key);
            if (!settings->contains(k)) return fallback;
            return settings->value(k).toStringList();
        }

        void listToSettings(const char* key, const QStringList& names) {
            auto* settings = ServiceRegistry::instance().get<vth::Settings>();
            if (!settings) return;
            settings->setValue(QString::fromLatin1(key), names);
            settings->sync();
        }
    }

    QStringList closeListFromSettings() {
        return listFromSettings("genesis/closeList", defaultCloseList());
    }

    QStringList exceptionsFromSettings() {
        return listFromSettings("genesis/exceptions", defaultExceptions());
    }

    void saveCloseList(const QStringList& names) {
        listToSettings("genesis/closeList", names);
    }

    void saveExceptions(const QStringList& names) {
        listToSettings("genesis/exceptions", names);
    }

    RecoveryReport recoverInterruptedSession() {
        RecoveryReport out;

        State s = loadPersistedState();
        if (!hasUnfinishedSession(s)) return out;

        const int renamesBefore  = s.renames.size();
        const int servicesBefore = s.servicesStopped.size();
        // deactivate() clears the baseline.
        const QVector<ServiceState> wasRunning = s.baseline.services;

        qCWarning(catGenesis).noquote()
            << "a Genesis session did not end: active =" << s.active
            << ", renames =" << renamesBefore
            << ", services =" << servicesBefore
            << ", started" << s.startedAt.toString(Qt::ISODate);

        deactivate(s);

        for (const RenameRecord& rec : s.renames) out.unrestored << rec.renamedPath;

        // A session cut off between stopping a service and saving the record leaves
        // one that resumeServices doesn't know about.
        if (!wasRunning.isEmpty()) {
            QSet<QString> runningNow;
            for (const ServiceState& svc : enumerateServiceStates())
                if (svc.running) runningNow.insert(svc.name.toLower());

            QStringList stopped;
            for (const ServiceState& svc : wasRunning) {
                if (!svc.running) continue;
                if (svc.startType != static_cast<quint32>(SERVICE_AUTO_START)) continue;
                if (!runningNow.contains(svc.name.toLower())) stopped << svc.name;
            }
            if (!stopped.isEmpty()) {
                qCWarning(catGenesis).noquote()
                    << "running automatically at capture and stopped now:"
                    << stopped.join(QStringLiteral(", "));
            }
        }

        if (out.unrestored.isEmpty()) {
            qCInfo(catGenesis).noquote()
                << "session torn down:" << renamesBefore << "names,"
                << servicesBefore - s.servicesStopped.size() << "services";
        } else {
            qCCritical(catGenesis).noquote()
                << out.unrestored.size() << "program files still carry the suffix:"
                << out.unrestored.join(QStringLiteral(", "));
        }
        return out;
    }

    State loadPersistedState() {
        State s;
        QFile f(stateFilePath());
        if (!f.exists() || !f.open(QIODevice::ReadOnly)) return s;
        QJsonParseError err;
        const auto doc = QJsonDocument::fromJson(f.readAll(), &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            qCCritical(catGenesis) << "genesis-state.json is unreadable:" << err.errorString();
            return s;
        }
        const auto o = doc.object();
        s.active                 = o.value(QStringLiteral("active")).toBool();
        s.startedAt              = QDateTime::fromString(
            o.value(QStringLiteral("startedAt")).toString(), Qt::ISODate);
        s.previousPowerPlanGuid  = o.value(QStringLiteral("prevPlan")).toString();
        s.processesKilled        = o.value(QStringLiteral("killed")).toInt();

        s.baseline = baselineFromJson(o.value(QStringLiteral("baseline")).toObject());
        const auto renameArr = o.value(QStringLiteral("renames")).toArray();
        s.renames.reserve(renameArr.size());
        for (const auto& v : renameArr) s.renames.append(renameFromJson(v.toObject()));

        const auto svcArr = o.value(QStringLiteral("services")).toArray();
        for (const auto& v : svcArr) s.servicesStopped.append(serviceFromJson(v.toObject()));
        const auto regArr = o.value(QStringLiteral("regs")).toArray();
        for (const auto& v : regArr) s.registrySnapshots.append(regFromJson(v.toObject()));

        s.explorerSuspended         = o.value(QStringLiteral("explorerSuspended")).toBool();
        s.notificationsDisabled     = o.value(QStringLiteral("notificationsDisabled")).toBool();
        {
            const qint64 timerRaw = o.value(QStringLiteral("timerHundredNs"))
                                        .toVariant().toLongLong();
            s.timerResolutionRequested = (timerRaw >= 0 && timerRaw <= UINT32_MAX)
                ? static_cast<quint32>(timerRaw)
                : 0;
        }
        s.mmcssEnabled              = o.value(QStringLiteral("mmcss")).toBool();
        s.drsBasePerfApplied        = o.value(QStringLiteral("drsBasePerf")).toBool(false);
        const auto suspArr = o.value(QStringLiteral("suspended")).toArray();
        for (const auto& v : suspArr) s.suspendedProcessNames << v.toString();
        s.processorIdleDisabled     = o.value(QStringLiteral("idleOff")).toBool(false);
        const auto throttleArr = o.value(QStringLiteral("powerThrottleOff")).toArray();
        for (const auto& v : throttleArr) s.powerThrottlingDisabledFor << v.toString();
        return s;
    }

    bool savePersistedState(const State& s) {
        paths::ensureDir(paths::protectedData());
        QSaveFile f(stateFilePath());
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
        QJsonObject o;
        o[QStringLiteral("active")]    = s.active;
        o[QStringLiteral("startedAt")] = s.startedAt.toString(Qt::ISODate);
        o[QStringLiteral("prevPlan")]  = s.previousPowerPlanGuid;
        o[QStringLiteral("killed")]    = s.processesKilled;

        o[QStringLiteral("baseline")] = toJson(s.baseline);

        QJsonArray renameArr;
        for (const auto& rec : s.renames) renameArr.append(toJson(rec));
        o[QStringLiteral("renames")] = renameArr;

        QJsonArray svcArr;
        for (const auto& sv : s.servicesStopped) svcArr.append(toJson(sv));
        o[QStringLiteral("services")] = svcArr;

        QJsonArray regArr;
        for (const auto& rs : s.registrySnapshots) regArr.append(toJson(rs));
        o[QStringLiteral("regs")] = regArr;

        o[QStringLiteral("explorerSuspended")]     = s.explorerSuspended;
        o[QStringLiteral("notificationsDisabled")] = s.notificationsDisabled;
        o[QStringLiteral("timerHundredNs")]        = static_cast<qint64>(s.timerResolutionRequested);
        o[QStringLiteral("mmcss")]                 = s.mmcssEnabled;
        o[QStringLiteral("drsBasePerf")]           = s.drsBasePerfApplied;

        QJsonArray suspArr;
        for (const auto& n : s.suspendedProcessNames) suspArr.append(n);
        o[QStringLiteral("suspended")]             = suspArr;
        o[QStringLiteral("idleOff")]               = s.processorIdleDisabled;

        QJsonArray throttleArr;
        for (const auto& n : s.powerThrottlingDisabledFor) throttleArr.append(n);
        o[QStringLiteral("powerThrottleOff")]      = throttleArr;

        f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
        return f.commit();
    }
}
