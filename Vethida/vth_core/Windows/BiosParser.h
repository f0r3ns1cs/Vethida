#pragma once

#include <QString>
#include <QVector>
#include <QHash>

// Reads the SMBIOS table through GetSystemFirmwareTable.
namespace vth::win::biosparser
{
    struct BiosInfo {
        QString vendor;
        QString version;
        QString releaseDate;
        QString majorRelease;     // e.g. "5.27"
        QString embeddedFirmware; // EC version, if present
    };

    struct SystemInfo {
        QString manufacturer;
        QString productName;
        QString version;
        QString serialNumber;
        QString uuid;
        QString sku;
        QString family;
    };

    struct BaseboardInfo {
        QString manufacturer;
        QString product;
        QString version;
        QString serialNumber;
        QString assetTag;
        QString locationInChassis;
    };

    struct ChassisInfo {
        QString manufacturer;
        QString type;             // "Desktop", "Laptop", ...
        QString version;
        QString serialNumber;
        QString assetTag;
    };

    struct CpuInfo {
        QString socketDesignation;
        QString manufacturer;
        QString version;
        QString family;           // "Core i7", "Ryzen", ...
        quint16 maxSpeedMhz       = 0;
        quint16 currentSpeedMhz   = 0;
        quint16 externalClockMhz  = 0;
        quint16 coreCount         = 0;
        quint16 threadCount       = 0;
        quint8  voltageDeciVolts  = 0;
        QString signature;        // the raw processor ID, in hex
        bool    populated         = false;
    };

    struct MemoryDeviceInfo {
        QString locator;          // "DIMM_A0", ...
        QString bankLocator;
        QString manufacturer;
        QString partNumber;
        QString serialNumber;
        QString assetTag;
        QString memoryType;       // "DDR4", "DDR5", ...
        QString formFactor;       // "DIMM", "SODIMM", ...
        quint64 sizeBytes         = 0;       // 0 means no module installed
        quint32 speedMtps         = 0;       // the speed it's running at
        quint32 maxSpeedMtps      = 0;       // the speed it's rated for
        quint16 dataWidthBits     = 0;
        quint16 totalWidthBits    = 0;
        quint8  rank              = 0;
    };

    struct Snapshot {
        bool                      ok = false;
        QString                   error;
        BiosInfo                  bios;
        SystemInfo                system;
        BaseboardInfo             baseboard;
        ChassisInfo               chassis;
        QVector<CpuInfo>          cpus;
        QVector<MemoryDeviceInfo> memorySlots;
        quint8                    smbiosMajor = 0;
        quint8                    smbiosMinor = 0;
        QHash<int, int>           structureCounts;
    };

    Snapshot read();
}
