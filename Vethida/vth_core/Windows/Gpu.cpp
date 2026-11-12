#include "Gpu.h"

#include "../Logger.h"
#include "Registry.h"

#include <QLatin1Char>
#include <QStringLiteral>

#include <windows.h>
#include <dxgi.h>

#include <algorithm>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "advapi32.lib")

namespace vth::win::gpu
{
    namespace
    {

        VTH_DEFINE_LOG_CATEGORY(catGpu, "vth.gpu")

        constexpr quint32 kPciNvidia    = 0x10DE;
        constexpr quint32 kPciAmd       = 0x1002;
        constexpr quint32 kPciIntel     = 0x8086;
        constexpr quint32 kPciMicrosoft = 0x1414;
        constexpr quint32 kPciQualcomm  = 0x17CB;
        constexpr quint32 kPciApple     = 0x106B;

        // Where Windows records the driver for each graphics adapter.
        constexpr wchar_t kDisplayClassKey[] =
            L"SYSTEM\\CurrentControlSet\\Control\\Class"
            L"\\{4d36e968-e325-11ce-bfc1-08002be10318}";

        // The driver version recorded for one PCI device, empty when there is
        // none to read.
        QString driverVersionFor(quint32 vendorId, quint32 deviceId) {
            HKEY classKey = nullptr;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, kDisplayClassKey, 0,
                              KEY_READ, &classKey) != ERROR_SUCCESS) {
                return {};
            }

            // A prefix: MatchingDeviceId may add a subsystem and revision.
            const QString wanted = QStringLiteral("pci\\ven_%1&dev_%2")
                                       .arg(vendorId, 4, 16, QLatin1Char('0'))
                                       .arg(deviceId, 4, 16, QLatin1Char('0'));

            QString found;
            for (DWORD i = 0;; ++i) {
                wchar_t name[64] = {};
                DWORD   len      = static_cast<DWORD>(sizeof(name) / sizeof(name[0]));
                if (RegEnumKeyExW(classKey, i, name, &len, nullptr,
                                  nullptr, nullptr, nullptr) != ERROR_SUCCESS)
                    break;

                HKEY sub = nullptr;
                if (RegOpenKeyExW(classKey, name, 0, KEY_READ, &sub) != ERROR_SUCCESS)
                    continue;
                if (reg::readStringAt(sub, L"MatchingDeviceId")
                        .startsWith(wanted, Qt::CaseInsensitive)) {
                    found = reg::readStringAt(sub, L"DriverVersion");
                }
                RegCloseKey(sub);

                if (!found.isEmpty())
                    break;
            }

            RegCloseKey(classKey);
            return found;
        }

    }

    QString vendorLabel(Vendor v) {
        switch (v) {
            case Vendor::Nvidia:    return QStringLiteral("NVIDIA");
            case Vendor::Amd:       return QStringLiteral("AMD");
            case Vendor::Intel:     return QStringLiteral("Intel");
            case Vendor::Microsoft: return QStringLiteral("Microsoft");
            case Vendor::Qualcomm:  return QStringLiteral("Qualcomm");
            case Vendor::Apple:     return QStringLiteral("Apple");
            case Vendor::Other:     break;
        }
        return QStringLiteral("Other");
    }

    Vendor vendorFromPciId(quint32 pciVendorId) noexcept {
        switch (pciVendorId) {
            case kPciNvidia:    return Vendor::Nvidia;
            case kPciAmd:       return Vendor::Amd;
            case kPciIntel:     return Vendor::Intel;
            case kPciMicrosoft: return Vendor::Microsoft;
            case kPciQualcomm:  return Vendor::Qualcomm;
            case kPciApple:     return Vendor::Apple;
            default:            return Vendor::Other;
        }
    }

    QVector<Adapter> enumerateAdapters() {
        QVector<Adapter> out;

        IDXGIFactory1* factory = nullptr;
        HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1),
                                        reinterpret_cast<void**>(&factory));
        if (FAILED(hr) || !factory) {
            qCWarning(catGpu) << "CreateDXGIFactory1 failed (hr =" << hr << ")";
            return out;
        }

        for (UINT i = 0;; ++i) {
            IDXGIAdapter1* adapter = nullptr;
            hr = factory->EnumAdapters1(i, &adapter);
            if (hr == DXGI_ERROR_NOT_FOUND) break;
            if (FAILED(hr) || !adapter) break;

            DXGI_ADAPTER_DESC1 desc{};
            if (SUCCEEDED(adapter->GetDesc1(&desc))) {
                Adapter a;
                a.name     = QString::fromWCharArray(desc.Description);
                a.vendorId = static_cast<quint32>(desc.VendorId);
                a.deviceId = static_cast<quint32>(desc.DeviceId);
                a.vendor   = vendorFromPciId(a.vendorId);
                a.dedicatedVideoMemory  = static_cast<quint64>(desc.DedicatedVideoMemory);
                a.dedicatedSystemMemory = static_cast<quint64>(desc.DedicatedSystemMemory);
                a.sharedSystemMemory    = static_cast<quint64>(desc.SharedSystemMemory);
                a.isSoftware = (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0;
                if (!a.isSoftware)
                    a.driverVersion = driverVersionFor(a.vendorId, a.deviceId);
                out.append(std::move(a));
            }
            adapter->Release();
        }
        factory->Release();

        // Keep software adapters only when there is nothing else to show.
        const bool anyHardware = std::any_of(out.cbegin(), out.cend(),
                                             [](const Adapter& a) { return !a.isSoftware; });
        if (anyHardware)
            out.removeIf([](const Adapter& a) { return a.isSoftware; });
        return out;
    }

    std::optional<Adapter> primaryAdapter() {
        std::optional<Adapter> best;
        for (const Adapter& a : enumerateAdapters()) {
            if (a.isSoftware)
                continue;
            if (!best || a.dedicatedVideoMemory > best->dedicatedVideoMemory)
                best = a;
        }
        return best;
    }

}
