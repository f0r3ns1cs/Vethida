#include "OsVersionHelper.h"

#include <QString>

#include <windows.h>

namespace vth::win
{
    namespace
    {
        typedef LONG(WINAPI* RtlGetVersionFn)(PRTL_OSVERSIONINFOW);

        OsVersion readNow() {
            OsVersion v;

            RTL_OSVERSIONINFOEXW info = {};
            info.dwOSVersionInfoSize = sizeof(info);

            if (HMODULE ntdll = GetModuleHandleW(L"ntdll.dll")) {
                auto* fn = reinterpret_cast<RtlGetVersionFn>(
                    GetProcAddress(ntdll, "RtlGetVersion"));
                if (fn && fn(reinterpret_cast<PRTL_OSVERSIONINFOW>(&info)) == 0) {
                    v.major = info.dwMajorVersion;
                    v.minor = info.dwMinorVersion;
                    v.build = info.dwBuildNumber;
                    v.isServer = (info.wProductType != VER_NT_WORKSTATION);
                }
            }

            v.isWin11 = (v.major == 10 && v.build >= 22000);

            HKEY key = nullptr;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                0, KEY_READ | KEY_WOW64_64KEY, &key) == ERROR_SUCCESS)
            {
                auto readSz = [&](const wchar_t* name) -> QString {
                    wchar_t buf[256] = {};
                    DWORD type = 0;
                    DWORD cb = sizeof(buf);
                    if (RegQueryValueExW(key, name, nullptr, &type,
                        reinterpret_cast<LPBYTE>(buf), &cb) == ERROR_SUCCESS
                        && type == REG_SZ)
                    {
                        return QString::fromWCharArray(buf);
                    }
                    return QString();
                };
                auto readDword = [&](const wchar_t* name) -> quint32 {
                    DWORD val = 0;
                    DWORD type = 0;
                    DWORD cb = sizeof(val);
                    if (RegQueryValueExW(key, name, nullptr, &type,
                        reinterpret_cast<LPBYTE>(&val), &cb) == ERROR_SUCCESS
                        && type == REG_DWORD)
                    {
                        return static_cast<quint32>(val);
                    }
                    return 0;
                };
                v.productName = readSz(L"ProductName");
                v.displayVersion = readSz(L"DisplayVersion");
                v.ubr = readDword(L"UBR");
                RegCloseKey(key);
            }

            // ProductName still says "Windows 10" on Windows 11
            if (v.isWin11 && v.productName.contains(QStringLiteral("Windows 10"))) {
                v.productName.replace(QStringLiteral("Windows 10"),
                    QStringLiteral("Windows 11"));
            }

            return v;
        }
    }

    const OsVersion& osVersion() {
        static const OsVersion cache = readNow();
        return cache;
    }

    bool buildAtLeast(quint32 minBuild) {
        if (minBuild == 0) return true;
        return osVersion().build >= minBuild;
    }

    bool buildAtMost(quint32 maxBuild) {
        if (maxBuild == 0) return true;
        return osVersion().build <= maxBuild;
    }

    bool buildInRange(quint32 minBuild, quint32 maxBuild) {
        return buildAtLeast(minBuild) && buildAtMost(maxBuild);
    }

    QString prettyVersion() {
        const auto& v = osVersion();
        return QStringLiteral("%1 %2 (%3.%4)")
            .arg(v.productName,
                v.displayVersion,
                QString::number(v.build),
                QString::number(v.ubr));
    }

}
