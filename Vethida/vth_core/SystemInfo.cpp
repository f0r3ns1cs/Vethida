#include "SystemInfo.h"

#include "Windows/Gpu.h"

#include <QString>
#include <QStringLiteral>

#include <windows.h>
#include <intrin.h>

#include <cstring>
#include <vector>

#pragma comment(lib, "advapi32.lib")

namespace vth::win
{
    namespace
    {
        // Returns empty on failure.
        QString regString(HKEY root, const wchar_t* subKey, const wchar_t* value) {
            HKEY key = nullptr;
            if (RegOpenKeyExW(root, subKey, 0, KEY_READ, &key) != ERROR_SUCCESS)
                return {};
            wchar_t buf[512] = {};
            DWORD   bytes = sizeof(buf);
            DWORD   type  = 0;
            const LSTATUS rc =
                RegQueryValueExW(key, value, nullptr, &type,
                                 reinterpret_cast<LPBYTE>(buf), &bytes);
            RegCloseKey(key);
            if (rc != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ))
                return {};
            return QString::fromWCharArray(buf).trimmed();
        }

        // `found` separates a zero value from no value at all.
        quint32 regDword(HKEY root, const wchar_t* subKey, const wchar_t* value,
                         bool* found = nullptr) {
            if (found) *found = false;
            HKEY key = nullptr;
            if (RegOpenKeyExW(root, subKey, 0, KEY_READ, &key) != ERROR_SUCCESS)
                return 0;
            DWORD data = 0;
            DWORD bytes = sizeof(data);
            DWORD type  = 0;
            const LSTATUS rc =
                RegQueryValueExW(key, value, nullptr, &type,
                                 reinterpret_cast<LPBYTE>(&data), &bytes);
            RegCloseKey(key);
            if (rc != ERROR_SUCCESS || type != REG_DWORD)
                return 0;
            if (found) *found = true;
            return static_cast<quint32>(data);
        }

        void collectCpu(SystemInfo& info) {
            int regs[4] = {};

            __cpuid(regs, 0);
            char vendor[13] = {};
            std::memcpy(vendor + 0, &regs[1], 4);   // EBX
            std::memcpy(vendor + 4, &regs[3], 4);   // EDX
            std::memcpy(vendor + 8, &regs[2], 4);   // ECX
            info.cpuVendor = QString::fromLatin1(vendor);

            // Leaf 1, ECX bit 31 is the hypervisor-present bit.
            __cpuid(regs, 1);
            info.hypervisorPresent = (static_cast<unsigned>(regs[2]) & (1u << 31)) != 0;

            // The brand string needs extended leaves up to 0x80000004.
            __cpuid(regs, 0x80000000);
            if (static_cast<unsigned>(regs[0]) >= 0x80000004u) {
                char brand[49] = {};
                for (int i = 0; i < 3; ++i) {
                    __cpuid(reinterpret_cast<int*>(brand + i * 16),
                        0x80000002 + i);
                }
                const char* p = brand;
                while (*p == ' ') ++p;
                info.cpuBrand = QString::fromLatin1(p).trimmed();
            }

            SYSTEM_INFO sysInfo = {};
            GetNativeSystemInfo(&sysInfo);
            info.cpuLogicalProcs = static_cast<int>(sysInfo.dwNumberOfProcessors);

            switch (sysInfo.wProcessorArchitecture) {
            case PROCESSOR_ARCHITECTURE_AMD64:
                info.cpuArchitecture = QStringLiteral("64-bit (x64)");
                break;
            case PROCESSOR_ARCHITECTURE_ARM64:
                info.cpuArchitecture = QStringLiteral("64-bit (ARM64)");
                break;
            case PROCESSOR_ARCHITECTURE_INTEL:
                info.cpuArchitecture = QStringLiteral("32-bit (x86)");
                break;
            default:
                break;
            }

            // Physical cores. dwNumberOfProcessors above counts logical ones.
            DWORD bytes = 0;
            GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &bytes);
            if (bytes > 0 && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                std::vector<char> buffer(bytes);
                if (GetLogicalProcessorInformationEx(
                        RelationProcessorCore,
                        reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data()),
                        &bytes)) {
                    DWORD offset = 0;
                    while (offset < bytes) {
                        auto* rec = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(
                            buffer.data() + offset);
                        if (rec->Size == 0)
                            break;   // malformed; walking on would not terminate
                        if (rec->Relationship == RelationProcessorCore)
                            ++info.cpuPhysicalCores;
                        offset += rec->Size;
                    }
                }
            }

            // Set by firmware at boot, so this isn't the live clock speed.
            info.cpuBaseClockMhz = regDword(
                HKEY_LOCAL_MACHINE,
                L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", L"~MHz");
        }

        void collectMemory(SystemInfo& info) {
            MEMORYSTATUSEX mem = {};
            mem.dwLength = sizeof(mem);
            if (GlobalMemoryStatusEx(&mem)) {
                info.ramTotalBytes      = mem.ullTotalPhys;
                info.ramAvailableBytes  = mem.ullAvailPhys;
                info.pageFileTotalBytes = mem.ullTotalPageFile;
                info.pageFileAvailBytes = mem.ullAvailPageFile;
                info.virtualTotalBytes  = mem.ullTotalVirtual;
            }
        }

        using GetDpiForSystemFn = UINT(WINAPI*)();

        void collectDisplay(SystemInfo& info) {
            if (const auto adapter = gpu::primaryAdapter())
                info.gpuName = adapter->name;

            // No hardware adapter, such as on the basic display driver.
            if (info.gpuName.isEmpty()) {
                DISPLAY_DEVICEW dd = {};
                dd.cb = sizeof(dd);
                for (DWORD i = 0; EnumDisplayDevicesW(nullptr, i, &dd, 0); ++i) {
                    if (dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) {
                        info.gpuName = QString::fromWCharArray(dd.DeviceString);
                        break;
                    }
                }
            }

            DEVMODEW dm = {};
            dm.dmSize = sizeof(dm);
            if (EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &dm)) {
                info.displayWidth     = static_cast<int>(dm.dmPelsWidth);
                info.displayHeight    = static_cast<int>(dm.dmPelsHeight);
                info.displayRefreshHz = static_cast<int>(dm.dmDisplayFrequency);
            }

            // Resolved at runtime: the SDK headers guard it behind a WINVER floor.
            if (HMODULE user32 = GetModuleHandleW(L"user32.dll")) {
                if (auto fn = reinterpret_cast<GetDpiForSystemFn>(
                        GetProcAddress(user32, "GetDpiForSystem"))) {
                    const UINT dpi = fn();
                    if (dpi > 0)
                        info.displayScalePct = static_cast<int>(dpi * 100 / USER_DEFAULT_SCREEN_DPI);
                }
            }
        }

        using RtlGetVersionFn = LONG(WINAPI*)(PRTL_OSVERSIONINFOEXW);

        // GetVersionExW is manifest-gated, so use RtlGetVersion. ProductName still
        // says "Windows 10" on Windows 11, so go by the build number.
        void collectOs(SystemInfo& info) {
            static constexpr const wchar_t* kCurrentVersion =
                L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";

            HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
            if (!ntdll) return;

            auto rtlGetVersion = reinterpret_cast<RtlGetVersionFn>(
                GetProcAddress(ntdll, "RtlGetVersion"));
            if (!rtlGetVersion) return;

            RTL_OSVERSIONINFOEXW vi = {};
            vi.dwOSVersionInfoSize = sizeof(vi);
            if (rtlGetVersion(&vi) != 0) return;   // STATUS_SUCCESS

            info.osBuild = QStringLiteral("%1.%2.%3")
                .arg(vi.dwMajorVersion)
                .arg(vi.dwMinorVersion)
                .arg(vi.dwBuildNumber);

            QString edition = (vi.dwBuildNumber >= 22000)
                ? QStringLiteral("Windows 11")
                : QStringLiteral("Windows 10");

            const QString sku = regString(HKEY_LOCAL_MACHINE, kCurrentVersion, L"EditionID");
            if (!sku.isEmpty())
                edition.append(QLatin1Char(' ')).append(sku);
            info.osEdition = edition;

            // DisplayVersion ("24H2") exists from 20H2 on; ReleaseId ("2004") before.
            info.osDisplayVersion =
                regString(HKEY_LOCAL_MACHINE, kCurrentVersion, L"DisplayVersion");
            if (info.osDisplayVersion.isEmpty())
                info.osDisplayVersion =
                    regString(HKEY_LOCAL_MACHINE, kCurrentVersion, L"ReleaseId");

            const quint32 installed =
                regDword(HKEY_LOCAL_MACHINE, kCurrentVersion, L"InstallDate");
            if (installed > 0) {
                info.osInstallDate = QDateTime::fromSecsSinceEpoch(
                    static_cast<qint64>(installed));
            }

            wchar_t dir[MAX_PATH] = {};
            if (GetWindowsDirectoryW(dir, MAX_PATH) > 0)
                info.windowsDirectory = QString::fromWCharArray(dir);
            if (GetSystemDirectoryW(dir, MAX_PATH) > 0)
                info.systemDirectory = QString::fromWCharArray(dir);

            info.uptimeMs = static_cast<qint64>(GetTickCount64());
        }

        void collectLocale(SystemInfo& info) {
            wchar_t name[LOCALE_NAME_MAX_LENGTH] = {};
            if (GetUserDefaultLocaleName(name, LOCALE_NAME_MAX_LENGTH) > 0) {
                wchar_t display[256] = {};
                if (GetLocaleInfoEx(name, LOCALE_SLOCALIZEDDISPLAYNAME,
                                    display, 256) > 0)
                    info.localeName = QString::fromWCharArray(display);
                else
                    info.localeName = QString::fromWCharArray(name);
            }

            TIME_ZONE_INFORMATION tz = {};
            switch (GetTimeZoneInformation(&tz)) {
            case TIME_ZONE_ID_DAYLIGHT:
                info.timeZone = QString::fromWCharArray(tz.DaylightName);
                break;
            case TIME_ZONE_ID_STANDARD:
            case TIME_ZONE_ID_UNKNOWN:
                info.timeZone = QString::fromWCharArray(tz.StandardName);
                break;
            default:
                break;
            }
        }

        using GetFirmwareTypeFn = BOOL(WINAPI*)(PFIRMWARE_TYPE);

        void collectFirmware(SystemInfo& info) {
            if (HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll")) {
                if (auto fn = reinterpret_cast<GetFirmwareTypeFn>(
                        GetProcAddress(kernel32, "GetFirmwareType"))) {
                    FIRMWARE_TYPE type = FirmwareTypeUnknown;
                    if (fn(&type))
                        info.uefiBoot = (type == FirmwareTypeUefi);
                }
            }

            bool present = false;
            const quint32 enabled = regDword(
                HKEY_LOCAL_MACHINE,
                L"SYSTEM\\CurrentControlSet\\Control\\SecureBoot\\State",
                L"UEFISecureBootEnabled", &present);
            info.secureBootReadable = present;
            info.secureBoot         = present && enabled != 0;
        }

        void collectIdentity(SystemInfo& info) {
            {
                wchar_t buf[256] = {};
                DWORD   sz = sizeof(buf) / sizeof(wchar_t);
                if (GetComputerNameExW(ComputerNameDnsHostname, buf, &sz)) {
                    info.computerName = QString::fromWCharArray(buf);
                }
            }
            {
                wchar_t buf[256] = {};
                DWORD   sz = sizeof(buf) / sizeof(wchar_t);
                if (GetUserNameW(buf, &sz)) {
                    info.userName = QString::fromWCharArray(buf);
                }
            }
            {
                // Empty when the machine isn't domain-joined.
                wchar_t buf[256] = {};
                DWORD   sz = sizeof(buf) / sizeof(wchar_t);
                if (GetComputerNameExW(ComputerNameDnsDomain, buf, &sz)) {
                    info.domainName = QString::fromWCharArray(buf);
                }
            }
        }

    }

    SystemInfo collect() {
        SystemInfo info;
        collectCpu(info);
        collectMemory(info);
        collectDisplay(info);
        collectOs(info);
        collectLocale(info);
        collectFirmware(info);
        collectIdentity(info);
        return info;
    }

    QString tidyHardwareName(QString name) {
        name.remove(QStringLiteral("(R)"), Qt::CaseInsensitive);
        name.remove(QStringLiteral("(TM)"), Qt::CaseInsensitive);
        name.remove(QStringLiteral("(C)"), Qt::CaseInsensitive);
        return name.simplified();
    }
}

namespace vth::win
{
namespace
{
        void collectFirmwarePosture(SystemInfo& info) {
            if (HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll")) {
                if (auto fn = reinterpret_cast<GetFirmwareTypeFn>(
                        GetProcAddress(kernel32, "GetFirmwareType"))) {
                    FIRMWARE_TYPE type = FirmwareTypeUnknown;
                    if (fn(&type))
                        info.uefiBoot = (type == FirmwareTypeUefi);
                }
            }
            bool present = false;
            const quint32 enabled = regDword(
                HKEY_LOCAL_MACHINE,
                L"SYSTEM\\CurrentControlSet\\Control\\SecureBoot\\State",
                L"UEFISecureBootEnabled", &present);
            info.secureBootReadable = present;
            info.secureBoot         = present && enabled != 0;
        }
}
}
