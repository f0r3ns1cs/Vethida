#pragma once

#include <QString>
#include <QVector>
#include <QtTypes>

#include <optional>

namespace vth::win::gpu
{

    enum class Vendor {
        Nvidia,
        Amd,
        Intel,
        Microsoft,
        Qualcomm,
        Apple,
        Other,
    };

    QString vendorLabel(Vendor v);
    Vendor  vendorFromPciId(quint32 pciVendorId) noexcept;

    struct Adapter {
        QString  name;
        Vendor   vendor   = Vendor::Other;
        quint32  vendorId = 0;
        quint32  deviceId = 0;
        quint64  dedicatedVideoMemory  = 0;
        quint64  dedicatedSystemMemory = 0;
        quint64  sharedSystemMemory    = 0;
        bool     isSoftware = false;   // WARP, Microsoft Basic Render Driver
        QString  driverVersion;        // empty for virtual and inbox drivers
    };

    QVector<Adapter> enumerateAdapters();

    // The hardware adapter with the most video memory.
    std::optional<Adapter> primaryAdapter();
}
