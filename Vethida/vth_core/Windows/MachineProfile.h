#pragma once

#include <QString>

#include <cstdint>

namespace vth::win::machine
{
    enum class CpuVendor  { Unknown, Intel, Amd };
    // Unknown means the vendor could not be read. Other means it was read and is
    // none of the three.
    enum class GpuVendor  { Unknown, Nvidia, Amd, Intel, Other };
    enum class StorageKind { Unknown, Hdd, SataSsd, Nvme };
    enum class Chassis     { Unknown, Desktop, Laptop };

    struct Profile
    {
        CpuVendor   cpuVendor = CpuVendor::Unknown;
        QString     cpuName;
        int         cpuLogicalProcessors = 0;
        bool        cpuHybrid = false;   // CPUID hybrid flag, Intel 12th gen onward

        GpuVendor   gpuVendor = GpuVendor::Unknown;
        QString     gpuName;
        QString     gpuDriverVersion;   // can be empty

        quint64     ramTotalBytes = 0;

        StorageKind systemDrive = StorageKind::Unknown;

        Chassis     chassis = Chassis::Unknown;

        QString     osBuild;

        bool hasNvidia() const { return gpuVendor == GpuVendor::Nvidia; }
        bool hasAmdGpu() const { return gpuVendor == GpuVendor::Amd; }
        bool hasIntelGpu() const { return gpuVendor == GpuVendor::Intel; }
        bool gpuIdentified() const { return gpuVendor != GpuVendor::Unknown; }
        bool hasIntelCpu() const { return cpuVendor == CpuVendor::Intel; }
        bool hasAmdCpu() const { return cpuVendor == CpuVendor::Amd; }
        bool hasFlashStorage() const
        {
            return systemDrive == StorageKind::SataSsd || systemDrive == StorageKind::Nvme;
        }
        bool hasNvme() const { return systemDrive == StorageKind::Nvme; }
        bool isDesktop() const { return chassis == Chassis::Desktop; }   // false if unknown
        quint64 ramGigabytes() const
        {
            return ramTotalBytes / (1024ULL * 1024ULL * 1024ULL);
        }
        [[nodiscard]] bool isLaptop() const { return chassis == Chassis::Laptop; }
    };

    // Slow: run it on a worker and cache the result.
    Profile detect();
}

namespace vth::win::machine
{
    QString storageLabel(StorageKind kind);
    QString chassisLabel(Chassis chassis);
}
