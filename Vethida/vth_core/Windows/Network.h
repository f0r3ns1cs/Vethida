#pragma once

#include <QString>
#include <QStringList>
#include <QVector>
#include <optional>

namespace vth::win::network
{
    struct Adapter {
        QString     friendlyName;   // "Wi-Fi", "Ethernet"
        QString     description;
        QStringList dnsServers;
        bool        dhcp = false;
        QString     guid;
        QStringList ipv4Addresses;
        quint64     linkSpeedBps = 0;
    };

    QVector<Adapter> enumerateAdapters();

    // Round-trip time in milliseconds, or -1 if there is no answer. Blocks for up
    // to timeoutMs.
    double probeLatencyMs(const QString& ipv4, int timeoutMs = 2000);

    enum class DnsResult {
        Ok,
        AccessDenied,
        AdapterNotFound,
        InvalidAddress,
        OtherError,
    };
    QString dnsResultLabel(DnsResult r);

    // Empty optional addresses are skipped, which leaves the IPv6 servers as they are.
    DnsResult setStaticDns(const QString& friendlyName,
                           const QString& primary,
                           const QString& secondary    = QString{},
                           const QString& primaryV6    = QString{},
                           const QString& secondaryV6  = QString{});

    // Back to DHCP-provided DNS for IPv4 and IPv6.
    DnsResult setDhcpDns(const QString& friendlyName);

    struct DnsPreset {
        QString name;
        QString label;         // name and addresses
        QString primary;
        QString secondary;
        QString primaryV6;
        QString secondaryV6;
    };

    const QVector<DnsPreset>& dnsPresets();

    // Empty if the servers don't match a preset.
    QString dnsPresetNameFor(const QStringList& servers);
}

namespace vth::win::network
{
    std::optional<Adapter> defaultAdapter();
}
