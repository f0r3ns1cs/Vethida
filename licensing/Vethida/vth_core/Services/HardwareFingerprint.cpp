#include "HardwareFingerprint.h"

#include "../Logger.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QDateTime>
#include <QtNetwork/QNetworkInterface>

#include <algorithm>
#include <vector>

#define _WIN32_DCOM
#include <windows.h>
#include <comdef.h>
#include <Wbemidl.h>
#include <winioctl.h>
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

namespace vth::services
{
    namespace
    {
        VTH_DEFINE_LOG_CATEGORY(catFingerprint, "vth.fingerprint")

        // OEM placeholder strings ("To be filled by O.E.M." etc). Treated as unreadable,
        // otherwise every board with the same placeholder gets the same hash.
        bool isSentinelValue(const QString& trimmed)
        {
            static const QStringList kSentinels = {
                QStringLiteral("to be filled by o.e.m."),
                QStringLiteral("default string"),
                QStringLiteral("system serial number"),
                QStringLiteral("not applicable"),
                QStringLiteral("not specified"),
                QStringLiteral("none"),
                QStringLiteral("unknown"),
                QStringLiteral("o.e.m."),
                QStringLiteral("oem"),
                QStringLiteral("0"),
                QStringLiteral("00000000"),
                QStringLiteral("123456789"),
                QStringLiteral("chassis serial number"),
                QStringLiteral("base board serial number"),
                QStringLiteral("serial number"),
                QStringLiteral("00000000-0000-0000-0000-000000000000"),
                QStringLiteral("ffffffff-ffff-ffff-ffff-ffffffffffff"),
            };

            if (trimmed.isEmpty())
                return true;

            const QString lower = trimmed.toLower();
            if (kSentinels.contains(lower))
                return true;

            // all zeros, all Fs, or nothing alphanumeric
            bool sawAlnum = false;
            bool allZero  = true;
            bool allF     = true;
            for (const QChar ch : lower) {
                if (ch.isLetterOrNumber())
                    sawAlnum = true;
                if (ch != QLatin1Char('0'))
                    allZero = false;
                if (ch != QLatin1Char('f'))
                    allF = false;
            }
            return !sawAlnum || allZero || allF;
        }

        // used by the WMI and disk paths. Never log the value.
        std::optional<QString> acceptOrReject(const char* source, const QString& raw)
        {
            const QString trimmed = raw.trimmed();
            if (isSentinelValue(trimmed)) {
                qCDebug(catFingerprint).noquote()
                    << "fingerprint source unreadable:" << source
                    << (trimmed.isEmpty() ? "empty" : "sentinel");
                return std::nullopt;
            }
            return trimmed;
        }

        // RAII handle
        class FileHandle {
        public:
            explicit FileHandle(HANDLE h) : m_h(h) {}
            ~FileHandle() { if (valid()) CloseHandle(m_h); }
            FileHandle(const FileHandle&) = delete;
            FileHandle& operator=(const FileHandle&) = delete;
            HANDLE get() const { return m_h; }
            bool valid() const { return m_h != nullptr && m_h != INVALID_HANDLE_VALUE; }
        private:
            HANDLE m_h;
        };

        // RAII CoInitializeEx / CoUninitialize
        class ComApartment {
        public:
            ComApartment()
            {
                const HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
                m_owns = SUCCEEDED(hr) && hr != S_FALSE;
                m_ok = SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE;
            }
            ~ComApartment()
            {
                if (m_owns)
                    CoUninitialize();
            }
            explicit operator bool() const { return m_ok; }
        private:
            bool m_owns = false;
            bool m_ok = false;
        };

        // runs body(IWbemServices*), false if WMI isn't reachable
        template <typename Body>
        bool withWmiNamespace(const wchar_t* nsName, Body&& body)
        {
            ComApartment apt;
            if (!apt)
                return false;

            // RPC_E_TOO_LATE is fine (already called elsewhere)
            CoInitializeSecurity(
                nullptr, -1, nullptr, nullptr,
                RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE,
                nullptr, EOAC_NONE, nullptr);

            IWbemLocator* loc = nullptr;
            if (FAILED(CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER,
                                        IID_IWbemLocator, reinterpret_cast<LPVOID*>(&loc))))
                return false;

            IWbemServices* svc = nullptr;
            BSTR ns = SysAllocString(nsName);
            HRESULT hr = loc->ConnectServer(ns, nullptr, nullptr, nullptr, 0, nullptr, nullptr, &svc);
            SysFreeString(ns);
            loc->Release();
            if (FAILED(hr) || svc == nullptr)
                return false;

            CoSetProxyBlanket(svc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr,
                              RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE,
                              nullptr, EOAC_NONE);

            bool ok = false;
            try {
                ok = body(svc);
            } catch (...) {
                ok = false;
            }
            svc->Release();
            return ok;
        }

        // First value that isn't empty or a placeholder. Errors just mean nullopt.
        std::optional<QString> readFirstNonEmpty(const wchar_t* wql, const wchar_t* column,
                                                 const char* sourceName)
        {
            std::optional<QString> result;
            withWmiNamespace(L"ROOT\\CIMV2", [&](IWbemServices* svc) {
                BSTR query = SysAllocString(wql);
                BSTR lang = SysAllocString(L"WQL");
                IEnumWbemClassObject* en = nullptr;
                HRESULT hr = svc->ExecQuery(lang, query,
                                            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                                            nullptr, &en);
                SysFreeString(query);
                SysFreeString(lang);
                if (FAILED(hr) || en == nullptr)
                    return false;

                IWbemClassObject* obj = nullptr;
                ULONG fetched = 0;
                while (en->Next(WBEM_INFINITE, 1, &obj, &fetched) == S_OK && fetched == 1) {
                    VARIANT v;
                    VariantInit(&v);
                    if (SUCCEEDED(obj->Get(column, 0, &v, nullptr, nullptr)) && v.vt == VT_BSTR && v.bstrVal != nullptr) {
                        const std::optional<QString> accepted =
                            acceptOrReject(sourceName, QString::fromWCharArray(v.bstrVal));
                        if (accepted) {
                            result = accepted;
                            VariantClear(&v);
                            obj->Release();
                            en->Release();
                            return true;
                        }
                    }
                    VariantClear(&v);
                    obj->Release();
                    obj = nullptr;
                }
                en->Release();
                return true;
            });
            return result;
        }

        // first running Ethernet/WiFi NIC by name
        std::optional<QString> queryPrimaryMac()
        {
            QList<QNetworkInterface> filtered;
            for (const QNetworkInterface& nic : QNetworkInterface::allInterfaces()) {
                const auto type = nic.type();
                if (type != QNetworkInterface::Ethernet && type != QNetworkInterface::Wifi)
                    continue;
                const auto flags = nic.flags();
                if (!(flags & QNetworkInterface::IsUp))
                    continue;
                if (!(flags & QNetworkInterface::IsRunning))
                    continue;
                if (nic.hardwareAddress().isEmpty())
                    continue;
                filtered.append(nic);
            }
            if (filtered.isEmpty())
                return std::nullopt;
            std::sort(filtered.begin(), filtered.end(),
                      [](const QNetworkInterface& a, const QNetworkInterface& b) {
                          return a.name() < b.name();
                      });
            // 3C:22:FB:AA:11:77
            const QString mac = filtered.first().hardwareAddress();
            return mac.isEmpty() ? std::nullopt : std::optional<QString>(mac);
        }

        // The system drive letter, as "C:". One reader for both disk sources.
        QString systemDriveLetter()
        {
            const QString fromEnv = QString::fromLocal8Bit(qgetenv("SystemDrive"));
            return fromEnv.isEmpty() ? QStringLiteral("C:") : fromEnv;
        }

        // filesystem serial, changes on format (the drive serial is queryDiskSerial)
        std::optional<QString> queryVolumeSerial()
        {
            const QString wql = QStringLiteral(
                "SELECT VolumeSerialNumber FROM Win32_LogicalDisk WHERE DeviceID='%1'")
                    .arg(systemDriveLetter());
            const std::wstring wide = wql.toStdWString();
            return readFirstNonEmpty(wide.c_str(), L"VolumeSerialNumber", "volumeSerial");
        }

        // some ATA bridges swap byte pairs, only unswap if that makes it printable
        bool isPrintableAscii(const QByteArray& b)
        {
            if (b.isEmpty())
                return false;
            for (const char c : b) {
                const unsigned char u = static_cast<unsigned char>(c);
                if (u < 0x20 || u > 0x7E)
                    return false;
            }
            return true;
        }

        QByteArray byteSwapPairs(const QByteArray& b)
        {
            QByteArray out = b;
            for (int i = 0; i + 1 < out.size(); i += 2)
                std::swap(out[i], out[i + 1]);
            return out;
        }

        // disk behind the system volume, nullopt if it spans several
        std::optional<DWORD> systemPhysicalDriveNumber()
        {
            const QString path = QStringLiteral("\\\\.\\") + systemDriveLetter();
            FileHandle vol(CreateFileW(path.toStdWString().c_str(), 0,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                       OPEN_EXISTING, 0, nullptr));
            if (!vol.valid()) {
                qCDebug(catFingerprint) << "diskSerial: cannot open the system volume";
                return std::nullopt;
            }

            // room for a few extents so a spanned volume is detected
            std::vector<char> buf(sizeof(VOLUME_DISK_EXTENTS) + 8 * sizeof(DISK_EXTENT));
            DWORD returned = 0;
            if (!DeviceIoControl(vol.get(), IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                                 nullptr, 0, buf.data(),
                                 static_cast<DWORD>(buf.size()), &returned, nullptr)) {
                qCDebug(catFingerprint) << "diskSerial: volume extents unavailable";
                return std::nullopt;
            }

            const auto* ext = reinterpret_cast<const VOLUME_DISK_EXTENTS*>(buf.data());
            if (ext->NumberOfDiskExtents != 1) {
                qCDebug(catFingerprint) << "diskSerial: system volume spans"
                                        << ext->NumberOfDiskExtents << "disks; no single serial";
                return std::nullopt;
            }
            return ext->Extents[0].DiskNumber;
        }

        // Drive firmware serial, survives format/reinstall. Often missing (RAID, USB
        // bridges), that's fine.
        std::optional<QString> queryDiskSerial()
        {
            const std::optional<DWORD> drive = systemPhysicalDriveNumber();
            if (!drive)
                return std::nullopt;

            const QString path = QStringLiteral("\\\\.\\PhysicalDrive%1").arg(*drive);
            // needs admin, can still fail
            FileHandle disk(CreateFileW(path.toStdWString().c_str(), GENERIC_READ,
                                        FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                        OPEN_EXISTING, 0, nullptr));
            if (!disk.valid()) {
                qCDebug(catFingerprint) << "diskSerial: cannot open PhysicalDrive"
                                        << *drive;
                return std::nullopt;
            }

            STORAGE_PROPERTY_QUERY query{};
            query.PropertyId = StorageDeviceProperty;
            query.QueryType  = PropertyStandardQuery;

            // get the size first, the descriptor is variable length
            STORAGE_DESCRIPTOR_HEADER header{};
            DWORD returned = 0;
            if (!DeviceIoControl(disk.get(), IOCTL_STORAGE_QUERY_PROPERTY,
                                 &query, sizeof(query), &header, sizeof(header),
                                 &returned, nullptr)
                || header.Size < sizeof(STORAGE_DEVICE_DESCRIPTOR)) {
                qCDebug(catFingerprint) << "diskSerial: descriptor size query refused";
                return std::nullopt;
            }

            std::vector<char> buf(header.Size);
            if (!DeviceIoControl(disk.get(), IOCTL_STORAGE_QUERY_PROPERTY,
                                 &query, sizeof(query), buf.data(),
                                 static_cast<DWORD>(buf.size()), &returned, nullptr)) {
                qCDebug(catFingerprint) << "diskSerial: descriptor read refused";
                return std::nullopt;
            }

            const auto* desc = reinterpret_cast<const STORAGE_DEVICE_DESCRIPTOR*>(buf.data());
            // Offset zero is the documented way a device says it has no serial.
            if (desc->SerialNumberOffset == 0
                || desc->SerialNumberOffset >= buf.size()) {
                qCDebug(catFingerprint) << "diskSerial: device reports no serial";
                return std::nullopt;
            }

            // bounded by the buffer in case the descriptor is bad
            const char* start = buf.data() + desc->SerialNumberOffset;
            const size_t room  = buf.size() - desc->SerialNumberOffset;
            const size_t len   = ::strnlen(start, room);
            QByteArray serial(start, static_cast<int>(len));

            serial = serial.trimmed();
            if (serial.size() % 2 == 0 && !isPrintableAscii(serial)) {
                const QByteArray swapped = byteSwapPairs(serial).trimmed();
                if (isPrintableAscii(swapped)) {
                    qCDebug(catFingerprint) << "diskSerial: byte-swapped ATA serial corrected";
                    serial = swapped;
                }
            }

            return acceptOrReject("diskSerial", QString::fromLatin1(serial));
        }
    }

    HardwareFingerprintVectorPathC HardwareFingerprintCollector::collect()
    {
        HardwareFingerprintVectorPathC out;
        // any source can be missing, the server only compares what both sides have
        out.motherboardUuid = readFirstNonEmpty(
            L"SELECT UUID FROM Win32_ComputerSystemProduct", L"UUID", "motherboardUuid");
        out.biosSerial = readFirstNonEmpty(
            L"SELECT SerialNumber FROM Win32_BIOS", L"SerialNumber", "biosSerial");
        out.cpuId = readFirstNonEmpty(
            L"SELECT ProcessorId FROM Win32_Processor", L"ProcessorId", "cpuId");
        out.diskSerial = queryDiskSerial();
        out.volumeSerial = queryVolumeSerial();
        out.primaryMac = queryPrimaryMac();
        if (!out.motherboardUuid && !out.biosSerial && !out.cpuId
            && !out.diskSerial && !out.volumeSerial && !out.primaryMac) {
            qCWarning(catFingerprint) << "no hardware sources readable; fingerprint is empty";
        }
        return out;
    }

    QString hashFingerprintSource(const QString& fieldName, const QString& value)
    {
        // empty stays empty, never hash it (every machine would get the same hash)
        if (value.isEmpty())
            return {};

        // the separator can't appear in a field name
        const QByteArray input =
            QByteArrayLiteral("vethida.fingerprint.v1|")
            + fieldName.toUtf8()
            + QByteArrayLiteral("|")
            + value.toUtf8();

        return QString::fromLatin1(
            QCryptographicHash::hash(input, QCryptographicHash::Sha256).toHex());
    }

    HardwareFingerprintVectorPathC hashFingerprint(
        const HardwareFingerprintVectorPathC& raw)
    {
        const auto h = [](const char* field, const std::optional<QString>& v)
            -> std::optional<QString> {
            if (!v || v->isEmpty())
                return std::nullopt;
            return hashFingerprintSource(QString::fromLatin1(field), *v);
        };

        HardwareFingerprintVectorPathC out;
        out.motherboardUuid = h("motherboardUuid", raw.motherboardUuid);
        out.biosSerial      = h("biosSerial",      raw.biosSerial);
        out.cpuId           = h("cpuId",           raw.cpuId);
        out.diskSerial      = h("diskSerial",      raw.diskSerial);
        out.volumeSerial    = h("volumeSerial",    raw.volumeSerial);
        out.primaryMac      = h("primaryMac",      raw.primaryMac);
        return out;
    }
}
