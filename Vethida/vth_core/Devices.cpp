#include "Devices.h"

#include "Logger.h"

#include <QHash>
#include <QStringList>
#include <QStringLiteral>
#include <QTimeZone>

#include <windows.h>

// Must precede devpkey.h and setupapi.h.
#include <initguid.h>
#include <devpkey.h>
#include <setupapi.h>
#include <cfgmgr32.h>

#include <array>
#include <vector>
#include "Windows/Registry.h"

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "cfgmgr32.lib")

namespace vth::win::devices
{
    namespace
    {
        VTH_DEFINE_LOG_CATEGORY(catDevices, "vth.devices")

        QString readDeviceProperty(HDEVINFO hDev, SP_DEVINFO_DATA& info, DWORD property) {
            DWORD regType  = 0;
            DWORD required = 0;
            SetupDiGetDeviceRegistryPropertyW(
                hDev, &info, property, &regType, nullptr, 0, &required);
            if (required == 0) return {};

            std::vector<wchar_t> buf(required / sizeof(wchar_t) + 1);
            if (!SetupDiGetDeviceRegistryPropertyW(
                    hDev, &info, property, &regType,
                    reinterpret_cast<PBYTE>(buf.data()),
                    static_cast<DWORD>(buf.size() * sizeof(wchar_t)), nullptr)) {
                return {};
            }
            return QString::fromWCharArray(buf.data());
        }

        QString readMultiSzProperty(HDEVINFO hDev, SP_DEVINFO_DATA& info, DWORD property) {
            DWORD regType  = 0;
            DWORD required = 0;
            SetupDiGetDeviceRegistryPropertyW(
                hDev, &info, property, &regType, nullptr, 0, &required);
            if (required == 0) return {};

            std::vector<wchar_t> buf(required / sizeof(wchar_t) + 1);
            if (!SetupDiGetDeviceRegistryPropertyW(
                    hDev, &info, property, &regType,
                    reinterpret_cast<PBYTE>(buf.data()),
                    static_cast<DWORD>(buf.size() * sizeof(wchar_t)), nullptr)) {
                return {};
            }
            QStringList parts;
            const wchar_t* p = buf.data();
            while (*p) {
                const QString s = QString::fromWCharArray(p);
                if (!s.isEmpty()) parts << s;
                p += s.size() + 1;
            }
            return parts.join(QStringLiteral("; "));
        }

        QString readInstanceId(HDEVINFO hDev, SP_DEVINFO_DATA& info) {
            wchar_t buf[MAX_DEVICE_ID_LEN + 1] = {};
            DWORD required = 0;
            if (!SetupDiGetDeviceInstanceIdW(hDev, &info, buf, MAX_DEVICE_ID_LEN, &required)) {
                return {};
            }
            return QString::fromWCharArray(buf);
        }

        QDateTime readFileTimeProperty(HDEVINFO hDev, SP_DEVINFO_DATA& info,
                                       const DEVPROPKEY& key) {
            DEVPROPTYPE type = 0;
            FILETIME ft = {};
            DWORD required = 0;
            if (!SetupDiGetDevicePropertyW(hDev, &info, &key, &type,
                                           reinterpret_cast<PBYTE>(&ft), sizeof(ft),
                                           &required, 0)) {
                return {};
            }
            if (ft.dwLowDateTime == 0 && ft.dwHighDateTime == 0) return {};

            SYSTEMTIME st = {};
            if (!FileTimeToSystemTime(&ft, &st)) return {};

            return QDateTime(QDate(st.wYear, st.wMonth, st.wDay),
                             QTime(st.wHour, st.wMinute, st.wSecond, st.wMilliseconds),
                             QTimeZone::UTC);
        }

        QDateTime readLastUsed(HDEVINFO hDev, SP_DEVINFO_DATA& info) {
            QDateTime t = readFileTimeProperty(hDev, info, DEVPKEY_Device_LastRemovalDate);
            if (t.isValid()) return t;
            t = readFileTimeProperty(hDev, info, DEVPKEY_Device_LastArrivalDate);
            if (t.isValid()) return t;
            return readFileTimeProperty(hDev, info, DEVPKEY_Device_InstallDate);
        }

        // Any other failure doesn't tell us whether the device is present.
        bool meansDeviceAbsent(CONFIGRET cr) noexcept {
            return cr == CR_NO_SUCH_DEVINST || cr == CR_NO_SUCH_VALUE;
        }
    }

    bool GhostDevice::isNonPnp() const noexcept {
        static const std::array<QStringView, 4> kPrefixes = {
            QStringView{u"HTREE\\ROOT\\"},
            QStringView{u"ROOT\\"},
            QStringView{u"SWD\\"},
            QStringView{u"SW\\{"},
        };
        for (auto p : kPrefixes) {
            if (instanceId.startsWith(p, Qt::CaseInsensitive)) return true;
        }
        return false;
    }

    QVector<GhostDevice> enumerateGhostDevices() {
        QVector<GhostDevice> out;

        // No DIGCF_PRESENT: the set contains every registered device, and a
        // failed devnode-status query identifies the ones no longer present.
        HDEVINFO hDev = SetupDiGetClassDevsW(nullptr, nullptr, nullptr, DIGCF_ALLCLASSES);
        if (hDev == INVALID_HANDLE_VALUE) return out;

        SP_DEVINFO_DATA info{};
        info.cbSize = sizeof(info);
        DWORD index = 0;
        while (SetupDiEnumDeviceInfo(hDev, index++, &info)) {
            ULONG status  = 0;
            ULONG problem = 0;
            const CONFIGRET cr = CM_Get_DevNode_Status(&status, &problem, info.DevInst, 0);

            if (meansDeviceAbsent(cr)) {
                GhostDevice d;
                d.instanceId     = readInstanceId(hDev, info);
                d.description    = readDeviceProperty(hDev, info, SPDRP_DEVICEDESC);
                d.deviceName     = readDeviceProperty(hDev, info, SPDRP_FRIENDLYNAME);
                d.className      = readDeviceProperty(hDev, info, SPDRP_CLASS);
                d.classGuid      = readDeviceProperty(hDev, info, SPDRP_CLASSGUID);
                d.manufacturer   = readDeviceProperty(hDev, info, SPDRP_MFG);
                d.service        = readDeviceProperty(hDev, info, SPDRP_SERVICE);
                d.driver         = readDeviceProperty(hDev, info, SPDRP_DRIVER);
                d.hardwareIds    = readMultiSzProperty(hDev, info, SPDRP_HARDWAREID);
                d.enumeratorName = readDeviceProperty(hDev, info, SPDRP_ENUMERATOR_NAME);
                d.lastUsed       = readLastUsed(hDev, info);
                out.append(d);
            }

            info = {};
            info.cbSize = sizeof(info);
        }
        SetupDiDestroyDeviceInfoList(hDev);

        return out;
    }

    GhostRemoveResult removeGhostDevices(const QVector<GhostDevice>& devices) {
        GhostRemoveResult res;
        if (devices.isEmpty()) return res;

        // The devices came from an earlier enumeration; re-locate each in a
        // fresh set and re-check its status before touching it.
        HDEVINFO hDev = SetupDiGetClassDevsW(nullptr, nullptr, nullptr, DIGCF_ALLCLASSES);
        if (hDev == INVALID_HANDLE_VALUE) {
            res.failed = static_cast<int>(devices.size());
            return res;
        }

        QHash<QString, SP_DEVINFO_DATA> instances;
        SP_DEVINFO_DATA info{};
        info.cbSize = sizeof(info);
        DWORD idx = 0;
        while (SetupDiEnumDeviceInfo(hDev, idx++, &info)) {
            const QString id = readInstanceId(hDev, info);
            if (!id.isEmpty()) instances.insert(id.toUpper(), info);
            info = {};
            info.cbSize = sizeof(info);
        }

        for (const auto& d : devices) {
            const auto it = instances.constFind(d.instanceId.toUpper());
            if (it == instances.constEnd()) {
                ++res.failed;
                continue;
            }
            SP_DEVINFO_DATA target = it.value();

            ULONG status  = 0;
            ULONG problem = 0;
            const CONFIGRET cr = CM_Get_DevNode_Status(&status, &problem, target.DevInst, 0);
            if (cr == CR_SUCCESS) {
                // reappeared since enumeration; removing would hit a live device
                qCInfo(catDevices) << "Skipping" << d.instanceId << "- device is present again";
                ++res.skipped;
                continue;
            }
            if (!meansDeviceAbsent(cr)) {
                qCWarning(catDevices) << "Not removing" << d.instanceId
                                      << "- status query failed with" << cr;
                ++res.failed;
                continue;
            }

            SP_REMOVEDEVICE_PARAMS rp{};
            rp.ClassInstallHeader.cbSize          = sizeof(SP_CLASSINSTALL_HEADER);
            rp.ClassInstallHeader.InstallFunction = DIF_REMOVE;
            rp.Scope     = DI_REMOVEDEVICE_GLOBAL;
            rp.HwProfile = 0;

            const BOOL paramsSet = SetupDiSetClassInstallParamsW(
                hDev, &target,
                reinterpret_cast<PSP_CLASSINSTALL_HEADER>(&rp), sizeof(rp));

            bool removed = false;
            if (paramsSet) {
                removed = SetupDiCallClassInstaller(DIF_REMOVE, hDev, &target);
            }
            if (!removed) {
                // direct removal when the class installer refuses (e.g. the
                // driver package is already uninstalled)
                removed = SetupDiRemoveDevice(hDev, &target);
            }
            if (removed) ++res.removed;
            else         ++res.failed;
        }

        SetupDiDestroyDeviceInfoList(hDev);
        qCInfo(catDevices) << "Ghost device removal:" << res.removed << "removed,"
                           << res.failed << "failed," << res.skipped << "skipped";
        return res;
    }

}

namespace vth::win::devices
{
namespace
{
        constexpr int kOldDriverDays = 365 * 2;
        struct DriverDetails {
            QString version;
            QString date;
            QString infPath;
            QString signer;
            bool    isSigned = false;
        };
        DriverDetails readDriverDetailsFromRegistry(const QString& driverKey) {
            DriverDetails out;
            if (driverKey.isEmpty()) return out;
            const QString sub =
                QStringLiteral("SYSTEM\\CurrentControlSet\\Control\\Class\\") + driverKey;
            HKEY h = nullptr;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, sub.toStdWString().c_str(),
                              0, KEY_READ, &h) != ERROR_SUCCESS) {
                return out;
            }
            out.version  = reg::readStringAt(h, L"DriverVersion");
            out.date     = reg::readStringAt(h, L"DriverDate");
            out.infPath  = reg::readStringAt(h, L"InfPath");
            out.signer   = reg::readStringAt(h, L"DigitalSigner");
            out.isSigned = !out.signer.isEmpty();
            RegCloseKey(h);
            return out;
        }
        int driverAgeDays(const QString& dateStr) {
            if (dateStr.isEmpty()) return 0;
            static const QStringList kFormats = {
                QStringLiteral("M-d-yyyy"),
                QStringLiteral("MM-dd-yyyy"),
                QStringLiteral("M/d/yyyy"),
                QStringLiteral("MM/dd/yyyy"),
                QStringLiteral("yyyy-MM-dd"),
            };
            for (const auto& fmt : kFormats) {
                const QDate d = QDate::fromString(dateStr, fmt);
                if (d.isValid()) return d.daysTo(QDate::currentDate());
            }
            return 0;
        }
}
}

namespace vth::win::devices
{
    DriverAuditResult auditDrivers() {
        DriverAuditResult out;
        HDEVINFO hDev = SetupDiGetClassDevsW(
            nullptr, nullptr, nullptr, DIGCF_PRESENT | DIGCF_ALLCLASSES);
        if (hDev == INVALID_HANDLE_VALUE) return out;
        SP_DEVINFO_DATA info{};
        info.cbSize = sizeof(info);
        DWORD index = 0;
        while (SetupDiEnumDeviceInfo(hDev, index++, &info)) {
            const QString name      = readDeviceProperty(hDev, info, SPDRP_DEVICEDESC);
            const QString provider  = readDeviceProperty(hDev, info, SPDRP_MFG);
            const QString className = readDeviceProperty(hDev, info, SPDRP_CLASS);
            const QString driverKey = readDeviceProperty(hDev, info, SPDRP_DRIVER);
            info = {};
            info.cbSize = sizeof(info);
            if (name.isEmpty()) continue;
            if (provider.contains(QStringLiteral("Microsoft"), Qt::CaseInsensitive)) {
                continue;
            }
            const auto details = readDriverDetailsFromRegistry(driverKey);
            const int  age     = driverAgeDays(details.date);
            DriverInfo d;
            d.name            = name;
            d.provider        = provider;
            d.version         = details.version;
            d.date            = details.date;
            d.infPath         = details.infPath;
            d.className       = className;
            d.signer          = details.signer;
            d.isSigned        = details.isSigned;
            d.ageDays         = age;
            d.flaggedOld      = age > kOldDriverDays;
            d.flaggedUnsigned = !details.isSigned;
            out.drivers.append(d);
        }
        SetupDiDestroyDeviceInfoList(hDev);
        QHash<QString, QVector<DriverInfo>> groups;
        for (const auto& d : out.drivers) {
            const QString key = d.className.toLower() + QChar::fromLatin1('|')
                              + d.provider.toLower();
            groups[key].append(d);
        }
        for (auto it = groups.cbegin(); it != groups.cend(); ++it) {
            if (it.value().size() <= 1) continue;
            DriverDuplicateGroup g;
            g.className = it.value().first().className;
            g.provider  = it.value().first().provider;
            g.drivers   = it.value();
            out.duplicates.append(g);
        }
        out.summary.totalDrivers = static_cast<int>(out.drivers.size());
        for (const auto& g : out.duplicates) {
            out.summary.duplicateCount += static_cast<int>(g.drivers.size()) - 1;
        }
        for (const auto& d : out.drivers) {
            if (d.isSigned)   ++out.summary.signedCount;
            else              ++out.summary.unsignedCount;
            if (d.flaggedOld) ++out.summary.oldCount;
        }
        return out;
    }
}
