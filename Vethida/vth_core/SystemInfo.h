#pragma once

#include <QDateTime>
#include <QString>
#include <QtTypes>

namespace vth::win
{
    struct SystemInfo {
        // Processor
        QString cpuVendor;            // "GenuineIntel", "AuthenticAMD"
        QString cpuBrand;             // "12th Gen Intel(R) Core(TM) i7-1280P"
        int     cpuPhysicalCores = 0;
        int     cpuLogicalProcs  = 0;
        quint32 cpuBaseClockMhz  = 0;
        QString cpuArchitecture;      // "64-bit (x64)", "ARM64", "32-bit (x86)"
        // Also set on hosts running Hyper-V or WSL2.
        bool    hypervisorPresent = false;

        // Memory
        quint64 ramTotalBytes      = 0;
        quint64 ramAvailableBytes  = 0;
        quint64 pageFileTotalBytes = 0;   // commit limit, not the file on disk
        quint64 pageFileAvailBytes = 0;
        quint64 virtualTotalBytes  = 0;

        // Graphics and display
        QString gpuName;              // from gpu::primaryAdapter
        int     displayWidth      = 0;
        int     displayHeight     = 0;
        int     displayRefreshHz  = 0;
        int     displayScalePct   = 0;    // 100, 125, 150, ...

        // Windows
        QString   osEdition;          // "Windows 11 Professional"
        QString   osBuild;            // "10.0.22631"
        QString   osDisplayVersion;   // "24H2"
        QDateTime osInstallDate;      // invalid when the key could not be read
        QString   windowsDirectory;
        QString   systemDirectory;
        QString   localeName;         // "English (United Kingdom)"
        QString   timeZone;           // the zone's own display name
        qint64    uptimeMs = 0;

        // Identity
        QString computerName;
        QString userName;
        QString domainName;           // empty when the machine is not joined

        // Firmware
        bool    uefiBoot           = false;
        bool    secureBoot         = false;
        bool    secureBootReadable = false;   // false on a legacy BIOS install
    };

    // Not for the UI thread.
    SystemInfo collect();

    // Strips "(R)", "(TM)" and "(C)" from a hardware name.
    QString tidyHardwareName(QString name);
}
