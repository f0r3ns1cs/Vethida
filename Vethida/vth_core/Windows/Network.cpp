#include "Network.h"

#include "../Logger.h"

#include <QProcess>
#include <QRegularExpression>
#include <QStringLiteral>

#include <vector>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <windows.h>
#include <limits>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

namespace vth::win::network
{
    namespace
    {
        VTH_DEFINE_LOG_CATEGORY(catNet, "vth.network")

        QString ipv4ToString(SOCKET_ADDRESS s) {
            if (!s.lpSockaddr || s.lpSockaddr->sa_family != AF_INET)
                return {};
            const auto* sa4 = reinterpret_cast<const sockaddr_in*>(s.lpSockaddr);
            char buf[INET_ADDRSTRLEN]{};
            if (!InetNtopA(AF_INET, &sa4->sin_addr, buf, sizeof(buf)))
                return {};
            return QString::fromLatin1(buf);
        }

        bool isValidIpv4(const QString& s) {
            static const QRegularExpression re(
                QStringLiteral(
                    "^(?:25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])"
                    "(?:\\.(?:25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])){3}$"));
            return re.match(s).hasMatch();
        }

        bool isPlausibleIpv6(const QString& s) {
            if (!s.contains(QChar::fromLatin1(':'))) return false;
            static const QRegularExpression re(QStringLiteral("^[0-9a-fA-F:]+$"));
            return re.match(s).hasMatch();
        }

        QString netshNameArg(const QString& friendlyName) {
            return QStringLiteral("name=%1").arg(friendlyName);
        }

        // Absolute, because this runs elevated and PATH could be hijacked.
        DnsResult runNetshDns(const QStringList& args, const char* what) {
            wchar_t dir[MAX_PATH] = {};
            const UINT n = GetSystemDirectoryW(dir, MAX_PATH);
            const QString system32 = (n > 0 && n < MAX_PATH)
                ? QString::fromWCharArray(dir, static_cast<int>(n))
                : QStringLiteral("C:\\Windows\\System32");

            QProcess proc;
            proc.setProcessChannelMode(QProcess::MergedChannels);
            proc.start(system32 + QStringLiteral("\\netsh.exe"), args);
            if (!proc.waitForFinished(15000)) {
                proc.kill();
                proc.waitForFinished(2000);
                qCWarning(catNet) << what << "timed out";
                return DnsResult::OtherError;
            }
            if (proc.exitCode() != 0) {
                qCWarning(catNet).noquote() << what << "failed:" << proc.readAll().trimmed();
                // netsh's exit codes don't say why, so assume most likely cause.
                return DnsResult::AccessDenied;
            }
            return DnsResult::Ok;
        }
    }

    QVector<Adapter> enumerateAdapters() {
        QVector<Adapter> out;

        ULONG bufLen = 16 * 1024;
        std::vector<BYTE> buffer(bufLen);
        auto* addrs = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());

        constexpr ULONG kFlags = GAA_FLAG_INCLUDE_PREFIX
                               | GAA_FLAG_SKIP_ANYCAST
                               | GAA_FLAG_SKIP_MULTICAST;

        DWORD r = GetAdaptersAddresses(AF_INET, kFlags, nullptr, addrs, &bufLen);
        if (r == ERROR_BUFFER_OVERFLOW) {
            buffer.resize(bufLen);
            addrs = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
            r = GetAdaptersAddresses(AF_INET, kFlags, nullptr, addrs, &bufLen);
        }
        if (r != NO_ERROR) return out;

        for (auto* a = addrs; a; a = a->Next) {
            if (a->OperStatus != IfOperStatusUp) continue;
            if (a->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;

            Adapter ad;
            ad.friendlyName = QString::fromWCharArray(a->FriendlyName);
            ad.description  = QString::fromWCharArray(a->Description);
            ad.dhcp         = a->Dhcpv4Enabled != 0;
            ad.guid         = QString::fromLatin1(a->AdapterName);
            ad.linkSpeedBps = static_cast<quint64>(a->TransmitLinkSpeed);
            if (ad.linkSpeedBps == std::numeric_limits<quint64>::max())
                ad.linkSpeedBps = 0;
            for (auto* u = a->FirstUnicastAddress; u; u = u->Next) {
                const QString s = ipv4ToString(u->Address);
                if (!s.isEmpty()) ad.ipv4Addresses.append(s);
            }

            for (auto* d = a->FirstDnsServerAddress; d; d = d->Next) {
                const QString s = ipv4ToString(d->Address);
                if (!s.isEmpty()) ad.dnsServers.append(s);
            }
            out.append(std::move(ad));
        }

        return out;
    }

    double probeLatencyMs(const QString& ipv4, int timeoutMs) {
        if (ipv4.isEmpty()) return -1;

        HANDLE icmp = IcmpCreateFile();
        if (icmp == INVALID_HANDLE_VALUE) return -1;

        IPAddr target = 0;
        const QByteArray addr = ipv4.toLatin1();
        if (InetPtonA(AF_INET, addr.constData(), &target) != 1) {
            IcmpCloseHandle(icmp);
            return -1;
        }

        char payload[32] = {};
        std::vector<BYTE> reply(sizeof(ICMP_ECHO_REPLY) + sizeof(payload) + 8);

        const DWORD count = IcmpSendEcho(
            icmp, target, payload, sizeof(payload),
            nullptr,
            reply.data(), static_cast<DWORD>(reply.size()),
            static_cast<DWORD>(timeoutMs));
        IcmpCloseHandle(icmp);

        if (count == 0) return -1;
        const auto* r = reinterpret_cast<const ICMP_ECHO_REPLY*>(reply.data());
        if (r->Status != IP_SUCCESS) return -1;
        return static_cast<double>(r->RoundTripTime);
    }

    QString dnsResultLabel(DnsResult r) {
        switch (r) {
            case DnsResult::Ok:
                return QStringLiteral("DNS updated.");
            case DnsResult::AccessDenied:
                return QStringLiteral(
                    "Windows refused the DNS change for this adapter. A VPN "
                    "client or security app that manages DNS usually holds "
                    "these settings; pause it and try again.");
            case DnsResult::AdapterNotFound:
                return QStringLiteral("Adapter not found.");
            case DnsResult::InvalidAddress:
                return QStringLiteral("Invalid IPv4 address.");
            case DnsResult::OtherError:
                return QStringLiteral("Windows could not apply the DNS change.");
        }
        return {};
    }

    DnsResult setStaticDns(const QString& friendlyName,
                           const QString& primary,
                           const QString& secondary,
                           const QString& primaryV6,
                           const QString& secondaryV6) {
        if (friendlyName.isEmpty()) return DnsResult::AdapterNotFound;
        if (!isValidIpv4(primary))  return DnsResult::InvalidAddress;
        if (!secondary.isEmpty() && !isValidIpv4(secondary))
            return DnsResult::InvalidAddress;
        if (!primaryV6.isEmpty() && !isPlausibleIpv6(primaryV6))
            return DnsResult::InvalidAddress;
        if (!secondaryV6.isEmpty() && !isPlausibleIpv6(secondaryV6))
            return DnsResult::InvalidAddress;

        const DnsResult first = runNetshDns({
            QStringLiteral("interface"),
            QStringLiteral("ipv4"),
            QStringLiteral("set"),
            QStringLiteral("dns"),
            netshNameArg(friendlyName),
            QStringLiteral("source=static"),
            QStringLiteral("address=%1").arg(primary),
            QStringLiteral("validate=no"),
        }, "netsh ipv4 set dns");
        if (first != DnsResult::Ok) return first;

        // The primary is already set, so later failures are OtherError.
        if (!secondary.isEmpty()
            && runNetshDns({
                QStringLiteral("interface"),
                QStringLiteral("ipv4"),
                QStringLiteral("add"),
                QStringLiteral("dns"),
                netshNameArg(friendlyName),
                QStringLiteral("address=%1").arg(secondary),
                QStringLiteral("index=2"),
                QStringLiteral("validate=no"),
            }, "netsh ipv4 add dns") != DnsResult::Ok)
            return DnsResult::OtherError;

        if (!primaryV6.isEmpty()
            && runNetshDns({
                QStringLiteral("interface"),
                QStringLiteral("ipv6"),
                QStringLiteral("set"),
                QStringLiteral("dnsservers"),
                netshNameArg(friendlyName),
                QStringLiteral("source=static"),
                QStringLiteral("address=%1").arg(primaryV6),
                QStringLiteral("validate=no"),
            }, "netsh ipv6 set dnsservers") != DnsResult::Ok)
            return DnsResult::OtherError;

        if (!secondaryV6.isEmpty()
            && runNetshDns({
                QStringLiteral("interface"),
                QStringLiteral("ipv6"),
                QStringLiteral("add"),
                QStringLiteral("dnsservers"),
                netshNameArg(friendlyName),
                QStringLiteral("address=%1").arg(secondaryV6),
                QStringLiteral("index=2"),
                QStringLiteral("validate=no"),
            }, "netsh ipv6 add dnsservers") != DnsResult::Ok)
            return DnsResult::OtherError;

        return DnsResult::Ok;
    }

    DnsResult setDhcpDns(const QString& friendlyName) {
        if (friendlyName.isEmpty()) return DnsResult::AdapterNotFound;

        const DnsResult first = runNetshDns({
            QStringLiteral("interface"),
            QStringLiteral("ipv4"),
            QStringLiteral("set"),
            QStringLiteral("dns"),
            netshNameArg(friendlyName),
            QStringLiteral("source=dhcp"),
        }, "netsh ipv4 set dns dhcp");
        if (first != DnsResult::Ok) return first;

        if (runNetshDns({
                QStringLiteral("interface"),
                QStringLiteral("ipv6"),
                QStringLiteral("set"),
                QStringLiteral("dnsservers"),
                netshNameArg(friendlyName),
                QStringLiteral("source=dhcp"),
            }, "netsh ipv6 set dnsservers dhcp") != DnsResult::Ok)
            return DnsResult::OtherError;

        return DnsResult::Ok;
    }

    // Vendor names and addresses, so these aren't translated.
    const QVector<DnsPreset>& dnsPresets() {
        static const QVector<DnsPreset> kList = {
            { QStringLiteral("Cloudflare"),
              QStringLiteral("Cloudflare 1.1.1.1 / 1.0.0.1"),
              QStringLiteral("1.1.1.1"),    QStringLiteral("1.0.0.1"),
              QStringLiteral("2606:4700:4700::1111"),
              QStringLiteral("2606:4700:4700::1001") },
            { QStringLiteral("Cloudflare (malware-blocking)"),
              QStringLiteral("Cloudflare malware-blocking 1.1.1.2"),
              QStringLiteral("1.1.1.2"),    QStringLiteral("1.0.0.2"),
              QStringLiteral("2606:4700:4700::1112"),
              QStringLiteral("2606:4700:4700::1002") },
            { QStringLiteral("Google"),
              QStringLiteral("Google 8.8.8.8 / 8.8.4.4"),
              QStringLiteral("8.8.8.8"),    QStringLiteral("8.8.4.4"),
              QStringLiteral("2001:4860:4860::8888"),
              QStringLiteral("2001:4860:4860::8844") },
            { QStringLiteral("Quad9"),
              QStringLiteral("Quad9 9.9.9.9 / 149.112.112.112"),
              QStringLiteral("9.9.9.9"),    QStringLiteral("149.112.112.112"),
              QStringLiteral("2620:fe::fe"),
              QStringLiteral("2620:fe::9") },
            { QStringLiteral("AdGuard"),
              QStringLiteral("AdGuard 94.140.14.14 / 94.140.15.15"),
              QStringLiteral("94.140.14.14"), QStringLiteral("94.140.15.15"),
              QStringLiteral("2a10:50c0::ad1:ff"),
              QStringLiteral("2a10:50c0::ad2:ff") },
            { QStringLiteral("OpenDNS"),
              QStringLiteral("OpenDNS 208.67.222.222 / 208.67.220.220"),
              QStringLiteral("208.67.222.222"), QStringLiteral("208.67.220.220"),
              QString(), QString() },
        };
        return kList;
    }

    QString dnsPresetNameFor(const QStringList& servers) {
        for (const DnsPreset& p : dnsPresets())
            if (servers.contains(p.primary))
                return p.name;
        return {};
    }
}

namespace vth::win::network
{
namespace
{
        QString genericSockAddrToString(SOCKET_ADDRESS s) {
            if (!s.lpSockaddr) return {};
            if (s.lpSockaddr->sa_family == AF_INET) {
                const auto* sa4 = reinterpret_cast<const sockaddr_in*>(s.lpSockaddr);
                char buf[INET_ADDRSTRLEN]{};
                if (InetNtopA(AF_INET, &sa4->sin_addr, buf, sizeof(buf))) {
                    return QString::fromLatin1(buf);
                }
            }
            return {};
        }
        DnsResult mapNetshExitCode(int code) {
            if (code == 0) return DnsResult::Ok;
            return DnsResult::AccessDenied;
        }
}
}

namespace vth::win::network
{
    std::optional<Adapter> defaultAdapter() {
        sockaddr_in dest{};
        dest.sin_family = AF_INET;
        dest.sin_addr.s_addr = INADDR_ANY;
        DWORD ifIndex = 0;
        if (GetBestInterfaceEx(reinterpret_cast<sockaddr*>(&dest), &ifIndex) != NO_ERROR)
            return std::nullopt;
        ULONG bufLen = 16 * 1024;
        std::vector<BYTE> buffer(bufLen);
        auto* addrs = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
        constexpr ULONG kFlags = GAA_FLAG_INCLUDE_PREFIX
                               | GAA_FLAG_SKIP_ANYCAST
                               | GAA_FLAG_SKIP_MULTICAST;
        DWORD r = GetAdaptersAddresses(AF_INET, kFlags, nullptr, addrs, &bufLen);
        if (r == ERROR_BUFFER_OVERFLOW) {
            buffer.resize(bufLen);
            addrs = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
            r = GetAdaptersAddresses(AF_INET, kFlags, nullptr, addrs, &bufLen);
        }
        if (r != NO_ERROR) return std::nullopt;
        for (auto* a = addrs; a; a = a->Next) {
            if (a->IfIndex != ifIndex) continue;
            Adapter ad;
            ad.guid         = QString::fromLatin1(a->AdapterName);
            ad.friendlyName = QString::fromWCharArray(a->FriendlyName);
            ad.description  = QString::fromWCharArray(a->Description);
            ad.dhcp         = (a->Dhcpv4Enabled != 0);
            ad.linkSpeedBps = static_cast<quint64>(a->TransmitLinkSpeed);
            if (ad.linkSpeedBps == std::numeric_limits<quint64>::max())
                ad.linkSpeedBps = 0;
            for (auto* u = a->FirstUnicastAddress; u; u = u->Next) {
                const QString s = genericSockAddrToString(u->Address);
                if (!s.isEmpty()) ad.ipv4Addresses.append(s);
            }
            for (auto* d = a->FirstDnsServerAddress; d; d = d->Next) {
                const QString s = genericSockAddrToString(d->Address);
                if (!s.isEmpty()) ad.dnsServers.append(s);
            }
            return ad;
        }
        return std::nullopt;
    }
}
