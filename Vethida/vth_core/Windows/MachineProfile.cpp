#include "MachineProfile.h"

#include "../SystemInfo.h"
#include "Gpu.h"

#include <QString>
#include <QStringLiteral>

#include <optional>

#include <windows.h>
#include <winioctl.h>
#include <powrprof.h>

#include <intrin.h>
#include <QObject>

#pragma comment(lib, "powrprof.lib")

namespace vth::win::machine
{
    namespace
    {
        CpuVendor parseCpuVendor(const QString& vendorId, const QString& brand)
        {
            // The brand string is for when the vendor id is a hypervisor's.
            if (vendorId.compare(QStringLiteral("GenuineIntel"), Qt::CaseInsensitive) == 0)
                return CpuVendor::Intel;
            if (vendorId.compare(QStringLiteral("AuthenticAMD"), Qt::CaseInsensitive) == 0)
                return CpuVendor::Amd;
            if (brand.contains(QStringLiteral("Intel"), Qt::CaseInsensitive))
                return CpuVendor::Intel;
            if (brand.contains(QStringLiteral("AMD"), Qt::CaseInsensitive)
                || brand.contains(QStringLiteral("Ryzen"), Qt::CaseInsensitive))
                return CpuVendor::Amd;
            return CpuVendor::Unknown;
        }

        // Only used when DXGI gives no PCI vendor id.
        GpuVendor parseGpuVendor(const QString& name)
        {
            if (name.contains(QStringLiteral("NVIDIA"), Qt::CaseInsensitive)
                || name.contains(QStringLiteral("GeForce"), Qt::CaseInsensitive)
                || name.contains(QStringLiteral("Quadro"), Qt::CaseInsensitive))
                return GpuVendor::Nvidia;
            if (name.contains(QStringLiteral("Intel"), Qt::CaseInsensitive)
                || name.contains(QStringLiteral("Iris"), Qt::CaseInsensitive)
                || name.contains(QStringLiteral("UHD Graphics"), Qt::CaseInsensitive))
                return GpuVendor::Intel;
            if (name.contains(QStringLiteral("AMD"), Qt::CaseInsensitive)
                || name.contains(QStringLiteral("Radeon"), Qt::CaseInsensitive))
                return GpuVendor::Amd;
            return GpuVendor::Unknown;
        }

        GpuVendor mapGpuVendor(gpu::Vendor v)
        {
            switch (v) {
                case gpu::Vendor::Nvidia: return GpuVendor::Nvidia;
                case gpu::Vendor::Amd:    return GpuVendor::Amd;
                case gpu::Vendor::Intel:  return GpuVendor::Intel;
                case gpu::Vendor::Microsoft:
                case gpu::Vendor::Qualcomm:
                case gpu::Vendor::Apple:
                case gpu::Vendor::Other:  break;
            }
            return GpuVendor::Other;
        }

        // CPUID leaf 7 subleaf 0, EDX bit 15.
        bool detectHybridCpu()
        {
            int regs[4] = {};
            __cpuidex(regs, 0, 0);
            if (regs[0] < 7)
                return false;
            __cpuidex(regs, 7, 0);
            return (static_cast<unsigned>(regs[3]) & (1u << 15)) != 0;
        }

        bool queryStorageProperty(HANDLE device, STORAGE_PROPERTY_ID id,
                                  void* out, DWORD size)
        {
            STORAGE_PROPERTY_QUERY query = {};
            query.PropertyId = id;
            query.QueryType  = PropertyStandardQuery;
            DWORD returned = 0;
            return DeviceIoControl(device, IOCTL_STORAGE_QUERY_PROPERTY,
                                   &query, sizeof(query),
                                   out, size, &returned, nullptr) != FALSE;
        }

        struct DriveFacts
        {
            STORAGE_BUS_TYPE bus         = BusTypeUnknown;
            bool             seekKnown   = false;
            bool             seekPenalty = false;
            bool             trimEnabled = false;
        };

        DriveFacts readDevice(HANDLE device)
        {
            DriveFacts f;

            // Adapter first, then device: a virtual volume may answer only one.
            STORAGE_ADAPTER_DESCRIPTOR adapter = {};
            if (queryStorageProperty(device, StorageAdapterProperty,
                                     &adapter, sizeof(adapter)))
                f.bus = static_cast<STORAGE_BUS_TYPE>(adapter.BusType);

            if (f.bus == BusTypeUnknown) {
                // Variable length, but BusType is in the fixed part.
                alignas(STORAGE_DEVICE_DESCRIPTOR) BYTE raw[1024] = {};
                if (queryStorageProperty(device, StorageDeviceProperty,
                                         raw, sizeof(raw))) {
                    const auto* desc =
                        reinterpret_cast<const STORAGE_DEVICE_DESCRIPTOR*>(raw);
                    f.bus = desc->BusType;
                }
            }

            DEVICE_SEEK_PENALTY_DESCRIPTOR seek = {};
            if (queryStorageProperty(device, StorageDeviceSeekPenaltyProperty,
                                     &seek, sizeof(seek))) {
                f.seekKnown   = true;
                f.seekPenalty = seek.IncursSeekPenalty != FALSE;
            }

            // TRIM is only enabled on flash, but not every SSD enables it.
            DEVICE_TRIM_DESCRIPTOR trim = {};
            if (queryStorageProperty(device, StorageDeviceTrimProperty,
                                     &trim, sizeof(trim)))
                f.trimEnabled = trim.TrimEnabled != FALSE;

            return f;
        }

        StorageKind classify(const DriveFacts& f)
        {
            if (f.bus == BusTypeNvme)
                return StorageKind::Nvme;

            if (f.seekKnown)
                return f.seekPenalty ? StorageKind::Hdd : StorageKind::SataSsd;

            if (f.trimEnabled)
                return StorageKind::SataSsd;

            return StorageKind::Unknown;
        }

        // The physical disk the system volume lives on, or -1.
        int physicalDriveNumber(HANDLE volume)
        {
            DWORD returned = 0;

            STORAGE_DEVICE_NUMBER number = {};
            if (DeviceIoControl(volume, IOCTL_STORAGE_GET_DEVICE_NUMBER,
                                nullptr, 0, &number, sizeof(number),
                                &returned, nullptr))
                return static_cast<int>(number.DeviceNumber);

            alignas(VOLUME_DISK_EXTENTS)
                BYTE raw[sizeof(VOLUME_DISK_EXTENTS) + 8 * sizeof(DISK_EXTENT)] = {};
            if (DeviceIoControl(volume, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                                nullptr, 0, raw, sizeof(raw), &returned, nullptr)) {
                const auto* extents =
                    reinterpret_cast<const VOLUME_DISK_EXTENTS*>(raw);
                if (extents->NumberOfDiskExtents > 0)
                    return static_cast<int>(extents->Extents[0].DiskNumber);
            }

            return -1;
        }

        // Asks the volume, then the disk underneath it.
        StorageKind detectSystemDrive()
        {
            wchar_t windowsDir[MAX_PATH] = {};
            if (!GetWindowsDirectoryW(windowsDir, MAX_PATH))
                return StorageKind::Unknown;
            // No drive letter means no volume to open.
            if (windowsDir[0] == L'\0' || windowsDir[1] != L':')
                return StorageKind::Unknown;

            const wchar_t volumePath[] = { L'\\', L'\\', L'.', L'\\',
                                           windowsDir[0], L':', L'\0' };
            const HANDLE volume = CreateFileW(volumePath, 0,
                                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                                              nullptr, OPEN_EXISTING, 0, nullptr);
            if (volume == INVALID_HANDLE_VALUE)
                return StorageKind::Unknown;

            StorageKind kind = classify(readDevice(volume));
            const int drive = (kind == StorageKind::Unknown)
                                  ? physicalDriveNumber(volume) : -1;
            CloseHandle(volume);

            if (kind != StorageKind::Unknown || drive < 0)
                return kind;

            // Happens on Storage Spaces and dynamic volumes.
            const QString diskPath =
                QStringLiteral("\\\\.\\PhysicalDrive%1").arg(drive);
            const HANDLE disk = CreateFileW(diskPath.toStdWString().c_str(), 0,
                                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                                            nullptr, OPEN_EXISTING, 0, nullptr);
            if (disk == INVALID_HANDLE_VALUE)
                return StorageKind::Unknown;

            kind = classify(readDevice(disk));
            CloseHandle(disk);
            return kind;
        }

        Chassis detectChassis()
        {
            SYSTEM_POWER_CAPABILITIES caps = {};
            if (GetPwrCapabilities(&caps))
                return (caps.LidPresent || caps.SystemBatteriesPresent)
                           ? Chassis::Laptop : Chassis::Desktop;

            SYSTEM_POWER_STATUS power = {};
            if (GetSystemPowerStatus(&power)) {
                // 128 is "no system battery", 255 is "status unknown".
                if (power.BatteryFlag == 128)
                    return Chassis::Desktop;
                if (power.BatteryFlag != 255)
                    return Chassis::Laptop;
            }

            return Chassis::Unknown;
        }
    }

    Profile detect()
    {
        Profile p;

        const SystemInfo info = collect();

        p.cpuVendor           = parseCpuVendor(info.cpuVendor, info.cpuBrand);
        p.cpuName             = info.cpuBrand;
        p.cpuLogicalProcessors = info.cpuLogicalProcs;
        p.cpuHybrid           = detectHybridCpu();

        if (const std::optional<gpu::Adapter> adapter = gpu::primaryAdapter()) {
            p.gpuVendor        = mapGpuVendor(adapter->vendor);
            p.gpuName          = adapter->name.simplified();
            p.gpuDriverVersion = adapter->driverVersion;
        } else {
            p.gpuVendor = parseGpuVendor(info.gpuName);
            p.gpuName   = info.gpuName;
        }

        p.ramTotalBytes = info.ramTotalBytes;
        p.osBuild       = info.osBuild;

        p.systemDrive = detectSystemDrive();
        p.chassis     = detectChassis();

        return p;
    }
}

namespace vth::win::machine
{
    QString storageLabel(StorageKind kind)
    {
        switch (kind) {
        case StorageKind::Nvme:    return QObject::tr("NVMe solid-state");
        case StorageKind::SataSsd: return QObject::tr("SATA solid-state");
        case StorageKind::Hdd:     return QObject::tr("Hard disk");
        case StorageKind::Unknown: break;
        }
        return QObject::tr("Not detected");
    }
    QString chassisLabel(Chassis chassis)
    {
        switch (chassis) {
        case Chassis::Laptop:  return QObject::tr("Laptop");
        case Chassis::Desktop: return QObject::tr("Desktop");
        case Chassis::Unknown: break;
        }
        return QObject::tr("Not detected");
    }
}

