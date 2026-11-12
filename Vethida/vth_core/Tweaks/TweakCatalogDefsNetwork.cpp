#include "TweakEngine.h"

namespace vth::tweaks::detail {

void appendDefsNetwork(QVector<TweakSpec>& c) {
    {
        TweakSpec t;
        t.id = QStringLiteral("net.afd.backlog");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("AFD / Winsock");
        t.name = QStringLiteral("AFD dynamic backlog configuration");
        t.description = QStringLiteral("Enables dynamic listen backlog with a fixed minimum (32), maximum (4096), and growth delta (16) for server workloads.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Higher memory usage on systems accepting many connections"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AFD\\Parameters"), QStringLiteral("EnableDynamicBacklog"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AFD\\Parameters"), QStringLiteral("MinimumDynamicBacklog"), 32u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AFD\\Parameters"), QStringLiteral("MaximumDynamicBacklog"), 4096u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AFD\\Parameters"), QStringLiteral("DynamicBacklogGrowthDelta"), 16u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.afd.buffer_tuning");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("AFD / Winsock");
        t.name = QStringLiteral("AFD socket buffer optimization");
        t.description = QStringLiteral("Optimizes Winsock socket buffers: send/receive windows, datagram thresholds, fast copy limits, large buffer sizes, and NIC buffer handling for high-throughput networking.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Apps that set their own socket buffers can conflict with these globals; high-volume servers may drop connections"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AFD\\Parameters"), QStringLiteral("DynamicSendBufferDisable"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AFD\\Parameters"), QStringLiteral("FastSendDatagramThreshold"), 1500u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AFD\\Parameters"), QStringLiteral("FastCopyReceiveThreshold"), 1500u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AFD\\Parameters"), QStringLiteral("MaxActiveTransmitFileCount"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AFD\\Parameters"), QStringLiteral("MaxFastCopyTransmit"), 4096u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AFD\\Parameters"), QStringLiteral("MaxFastTransmit"), 32768u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AFD\\Parameters"), QStringLiteral("PriorityBoost"), 4u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AFD\\Parameters"), QStringLiteral("DoNotHoldNICBuffers"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AFD\\Parameters"), QStringLiteral("TransmitWorker"), 32u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AFD\\Parameters"), QStringLiteral("DefaultReceiveWindow"), 33170u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AFD\\Parameters"), QStringLiteral("DefaultSendWindow"), 33170u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AFD\\Parameters"), QStringLiteral("DisableAddressSharing"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AFD\\Parameters"), QStringLiteral("LargeBufferSize"), 65536u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AFD\\Parameters"), QStringLiteral("BufferMultiplier"), 1024u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AFD\\Parameters"), QStringLiteral("KeepAliveInterval"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.bits.peercache_disable");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("Delivery Optimization");
        t.name = QStringLiteral("Disable BITS peer caching");
        t.description = QStringLiteral("Disables Background Intelligent Transfer Service (BITS) peer caching to prevent bandwidth usage for peer-to-peer update distribution.");
        t.iconName = QStringLiteral("rotate");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("BITS jobs may be slower without local peer sources"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\BITS"), QStringLiteral("EnablePeercaching"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\BITS"), QStringLiteral("DisablePeerCachingClient"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\BITS"), QStringLiteral("DisablePeerCachingServer"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.branchcache.disable");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("Delivery Optimization");
        t.name = QStringLiteral("Disable BranchCache");
        t.description = QStringLiteral("Disables BranchCache in both WinHTTP and BITS policies to prevent local caching of remote content for branch offices.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Branch office content caching disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\WinHttp"), QStringLiteral("DisableBranchCache"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\BITS"), QStringLiteral("DisableBranchCache"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.disable-cellular-failover");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("Wi-Fi & WCM");
        t.name = QStringLiteral("Disable cellular failover");
        t.description = QStringLiteral("Stops Windows from automatically failing over from Wi-Fi to a cellular modem when Wi-Fi degrades.");
        t.iconName = QStringLiteral("world");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WcmSvc"), QStringLiteral("DisableCellularFailover"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.delivery.p2p_disable");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("Delivery Optimization");
        t.name = QStringLiteral("Disable Delivery Optimization P2P");
        t.description = QStringLiteral("Disables peer-to-peer Delivery Optimization so Windows only downloads updates from Microsoft.");
        t.iconName = QStringLiteral("download");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Updates and Store app downloads come only from Microsoft; initial downloads are slower on metered or distant links"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DeliveryOptimization"), QStringLiteral("DODownloadMode"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DeliveryOptimization"), QStringLiteral("DownloadMode"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DeliveryOptimization\\Config"), QStringLiteral("DownloadMode"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DeliveryOptimization\\Config"), QStringLiteral("DODownloadMode"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.disable-llmnr");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("Security hardening");
        t.name = QStringLiteral("Disable link-local multicast name resolution");
        t.description = QStringLiteral("Removes a fallback name-resolution channel best known as a credential-relay attack vector.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows NT\\DNSClient"), QStringLiteral("EnableMulticast"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.qos.ncsi_disable");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("Connectivity probes");
        t.name = QStringLiteral("Disable NCSI active probing");
        t.description = QStringLiteral("Disables the Network Connectivity Status Indicator that periodically pings Microsoft servers.");
        t.iconName = QStringLiteral("wifi");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Windows may incorrectly display 'No Internet' status"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\NetworkConnectivityStatusIndicator"), QStringLiteral("NoActiveProbe"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\Connectivity\\DisallowNetworkConnectivityActiveTests"), QStringLiteral("value"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.power.saving_disable");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("Power & Adapter");
        t.name = QStringLiteral("Disable network power saving features");
        t.description = QStringLiteral("Disables network adapter power saving, packet coalescing, and wake-on-LAN for consistent network performance.");
        t.iconName = QStringLiteral("battery-charging");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Dead gateway detection is off; a failed primary gateway stops network traffic until it is manually switched"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("EnablePMTUDiscovery"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("EnableDeadGWDetect"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NDIS\\Parameters"), QStringLiteral("DefaultPnPCapabilities"), 0x18u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.dns.smart_resolution");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("DNS");
        t.name = QStringLiteral("Disable smart name resolution");
        t.description = QStringLiteral("Disables Smart Multi-Homed Name Resolution which sends DNS queries to all adapters simultaneously, plus IDN mapping, DNS registration, multicast DNS, and smart protocol reordering.");
        t.iconName = QStringLiteral("globe");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("VPN or split-tunnel setups relying on multi-adapter DNS resolution may resolve internal names slowly"),
            QStringLiteral("Local service discovery (mDNS) is disabled - Chromecast, AirPlay, and Bonjour stop working"),
        };
        t.registryOps = {
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows NT\\DNSClient"), QStringLiteral("DisableSmartNameResolution"), QStringLiteral("1"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows NT\\DNSClient"), QStringLiteral("EnableIdnMapping"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows NT\\DNSClient"), QStringLiteral("RegistrationEnabled"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows NT\\DNSClient"), QStringLiteral("PreferLocalOverLowerBindingDNS"), QStringLiteral("1"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows NT\\DNSClient"), QStringLiteral("EnableMulticast"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows NT\\DNSClient"), QStringLiteral("DisableSmartProtocolReordering"), QStringLiteral("1"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.wifi.power_management");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("Wi-Fi & WCM");
        // Writes EnableWsd and fMinimizeConnections only. It doesn't change the
        // adapter's own power management setting.
        t.name = QStringLiteral("Disable web services discovery");
        t.description = QStringLiteral("Disables Web Services Discovery (EnableWsd=0) and the minimize-connections policy, which turns off WSD-based auto-discovery of network printers and media devices. Wi-Fi adapter power settings are not affected.");
        t.iconName = QStringLiteral("wifi");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Automatic discovery of printers and media devices via WSD stops working on the local network"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("EnableWsd"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WcmSvc\\GroupPolicy"), QStringLiteral("fMinimizeConnections"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.dns-over-https");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("DNS");
        t.name = QStringLiteral("Enable DNS-over-HTTPS");
        t.description = QStringLiteral("Switches the system DNS resolver to encrypted DoH against the configured providers.");
        t.iconName = QStringLiteral("lock");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows NT\\DNSClient"), QStringLiteral("DoHPolicy"), 2u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.dscp-ef-game-traffic");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("QoS");
        t.name = QStringLiteral("Tag game traffic DSCP EF (46)");
        t.description = QStringLiteral("Adds a QoS policy that marks UDP traffic from common gaming ports with DSCP Expedited Forwarding.");
        t.iconName = QStringLiteral("tag");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.registryOps = {
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\QoS\\Vethida-Game-EF"), QStringLiteral("Version"), QStringLiteral("1.0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\QoS\\Vethida-Game-EF"), QStringLiteral("Application Name"), QStringLiteral("*"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\QoS\\Vethida-Game-EF"), QStringLiteral("Protocol"), QStringLiteral("UDP"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\QoS\\Vethida-Game-EF"), QStringLiteral("Local Port"), QStringLiteral("1024:65535"), std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\QoS\\Vethida-Game-EF"), QStringLiteral("DSCP Value"), 46u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.qos.game_priority");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("QoS");
        t.name = QStringLiteral("Game packet DSCP prioritization");
        t.description = QStringLiteral("Marks game traffic with high-priority DSCP values (EF/46) so routers prioritize gaming packets.");
        t.iconName = QStringLiteral("gamepad-2");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Tags all UDP traffic on this PC as high-priority, not just game traffic"),
            QStringLiteral("Some ISPs ignore or rewrite DSCP tags, so the effect varies by network"),
            QStringLiteral("On a corporate or managed network this may conflict with existing QoS policies"),
        };
        t.registryOps = {
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\QoS"), QStringLiteral("Do not use NLA"), QStringLiteral("1"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\QoS\\Gaming Default"), QStringLiteral("Version"), QStringLiteral("1.0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\QoS\\Gaming Default"), QStringLiteral("Application Name"), QStringLiteral("*"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\QoS\\Gaming Default"), QStringLiteral("Protocol"), QStringLiteral("UDP"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\QoS\\Gaming Default"), QStringLiteral("Local Port"), QStringLiteral("*"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\QoS\\Gaming Default"), QStringLiteral("Local IP"), QStringLiteral("*"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\QoS\\Gaming Default"), QStringLiteral("Local IP Prefix Length"), QStringLiteral("*"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\QoS\\Gaming Default"), QStringLiteral("Remote Port"), QStringLiteral("*"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\QoS\\Gaming Default"), QStringLiteral("Remote IP"), QStringLiteral("*"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\QoS\\Gaming Default"), QStringLiteral("Remote IP Prefix Length"), QStringLiteral("*"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\QoS\\Gaming Default"), QStringLiteral("DSCP Value"), QStringLiteral("46"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\QoS\\Gaming Default"), QStringLiteral("Throttle Rate"), QStringLiteral("-1"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.ics.hotspot_timeout");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("Hotspot & ICS");
        t.name = QStringLiteral("Disable Mobile Hotspot timeout");
        t.description = QStringLiteral("Disables the automatic timeout for mobile hotspot (ICS) connections and sets a longer public connection timeout.");
        t.iconName = QStringLiteral("clock");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Hotspot will remain active indefinitely when enabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\icssvc\\Settings"), QStringLiteral("PeerlessTimeoutEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\icssvc\\Settings"), QStringLiteral("PublicConnectionTimeout"), 120u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.internet.control_panel_policies");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("TLS & Internet Settings");
        t.name = QStringLiteral("Internet Control Panel security and protocol policies");
        t.description = QStringLiteral("Sets legacy WinINET/Internet Options security defaults: turns on certificate-revocation and bad-certificate warnings, enables HTTP/2 and Punycode, and sets the SecureProtocols mask to 0x2AA0 (SSL 3.0 through TLS 1.3). Note this enables, not restricts to, TLS 1.2/1.3, and modern browsers negotiate protocols independently of this value.");
        t.iconName = QStringLiteral("globe");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Forces TLS 1.2/1.3-only; legacy sites or intranet apps that require SSL 3.0 or TLS 1.0 stop loading"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"), QStringLiteral("CertificateRevocation"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"), QStringLiteral("DisableCachingOfSSLPages"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"), QStringLiteral("PrivacyAdvanced"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"), QStringLiteral("SecureProtocols"), 10912u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"), QStringLiteral("EnableNegotiate"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"), QStringLiteral("WarnonZoneCrossing"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"), QStringLiteral("EnableHttp1_1"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"), QStringLiteral("ProxyHttp1.1"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"), QStringLiteral("EnableHTTP2"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"), QStringLiteral("EnablePunycode"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"), QStringLiteral("UrlEncoding"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"), QStringLiteral("WarnonBadCertRecving"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"), QStringLiteral("WarnOnPostRedirect"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"), QStringLiteral("SyncMode5"), 3u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"), QStringLiteral("CertificateRevocation"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"), QStringLiteral("SecureProtocols"), 10912u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"), QStringLiteral("EnableHttp1_1"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"), QStringLiteral("EnableHTTP2"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"), QStringLiteral("EnablePunycode"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.ipv6.lltd_teredo");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("IPv6 & Xbox");
        t.name = QStringLiteral("IPv6 LLTD and Teredo configuration");
        t.description = QStringLiteral("Configures Link Layer Topology Discovery (LLTD) and Teredo tunneling for Xbox Live gaming compatibility. Enables LLTD I/O on domain networks, configures Teredo with Microsoft servers, and ensures IPv6 components are not disabled.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Teredo tunnel may bypass firewall rules"),
            QStringLiteral("LLTD enables network topology discovery"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\LLTD"), QStringLiteral("EnableLLTDIO"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\LLTD"), QStringLiteral("AllowLLTDIOOnDomain"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\LLTD"), QStringLiteral("ProhibitLLTDIOOnPrivateNet"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\LLTD"), QStringLiteral("AllowLLTDIOOnPublicNet"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\LLTD"), QStringLiteral("EnableRspndr"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\LLTD"), QStringLiteral("AllowRspndrOnDomain"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\LLTD"), QStringLiteral("AllowRspndrOnPublicNet"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\LLTD"), QStringLiteral("ProhibitRspndrOnPrivateNet"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\TCPIP\\v6Transition"), QStringLiteral("Teredo_ClientPort"), 0u, std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\TCPIP\\v6Transition"), QStringLiteral("Teredo_DefaultQualified"), QStringLiteral("Enabled"), std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\TCPIP\\v6Transition"), QStringLiteral("Teredo_RefreshRate"), 30u, std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\TCPIP\\v6Transition"), QStringLiteral("Teredo_ServerName"), QStringLiteral("win10.ipv6.microsoft.com"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\TCPIP\\v6Transition"), QStringLiteral("Teredo_State"), QStringLiteral("Enterprise Client"), std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip6\\Parameters"), QStringLiteral("DisabledComponents"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.legacy.netbios_disable");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("Legacy protocols");
        t.name = QStringLiteral("Disable NetBIOS over TCP/IP");
        t.description = QStringLiteral("Disables the legacy NetBIOS protocol that broadcasts device names on the network. Reduces network overhead and eliminates a common attack vector. Not needed for modern networks.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Legacy file shares, old printers, and SMBv1 devices that rely on NetBIOS name resolution become unreachable"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NetBT\\Parameters"), QStringLiteral("NodeType"), 2u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.miracast.config");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("Casting");
        t.name = QStringLiteral("Miracast and wireless display configuration");
        t.description = QStringLiteral("Configures Miracast receiver settings, disables mDNS advertisement while allowing discovery, enables projection to PC, and disables AC-only restriction.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("mDNS advertisement disabled may affect discovery by others"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\MiracastReceiver"), QStringLiteral("NetworkQualificationEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PlayToReceiver"), QStringLiteral("AutoEnabled"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\MiracastReceiver"), QStringLiteral("EnabledOnACOnly"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\WirelessDisplay\\AllowMdnsAdvertisement"), QStringLiteral("value"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\WirelessDisplay\\AllowMdnsDiscovery"), QStringLiteral("value"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\WirelessDisplay\\AllowProjectionToPC"), QStringLiteral("value"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.ncd.autosetup_disable");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("Discovery");
        t.name = QStringLiteral("Disable network device auto setup");
        t.description = QStringLiteral("Prevents Windows from automatically setting up newly connected network devices. Reduces unexpected network configuration changes.");
        t.iconName = QStringLiteral("wifi");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("New network devices must be configured manually"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\NcdAutoSetup\\Private"), QStringLiteral("AutoSetup"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.adapter.offload_tuning");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("TCP/IP");
        t.name = QStringLiteral("Network adapter offload and buffers");
        t.description = QStringLiteral("Optimizes TCP/IP stack buffers, disables ECN, and reconfigures AFD (Ancillary Function Driver) for gaming.");
        t.iconName = QStringLiteral("wifi");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Disables ECN, which a few ISPs and corporate networks expect to be on"),
            QStringLiteral("Changes global TCP and socket buffer sizes that affect every app that uses the network"),
            QStringLiteral("If downloads, video calls, or VPNs act up after applying, try reverting first"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("EnableECN"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("DefaultTTL"), 64u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("Tcp1323Opts"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("MaxUserPort"), 65534u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("TcpTimedWaitDelay"), 30u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AFD\\Parameters"), QStringLiteral("FastSendDatagramThreshold"), 1500u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AFD\\Parameters"), QStringLiteral("DefaultReceiveWindow"), 65536u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AFD\\Parameters"), QStringLiteral("DefaultSendWindow"), 65536u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.newnetwork.popup_disable");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("Discovery");
        t.name = QStringLiteral("Disable new network window popup");
        t.description = QStringLiteral("Suppresses the 'New network detected' popup window that appears when connecting to a new network by creating the NewNetworkWindowOff registry key.");
        t.iconName = QStringLiteral("wifi");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("No prompt appears to classify new networks as private or public; new connections default to public"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Network\\NewNetworkWindowOff"), QStringLiteral(""), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.nic.rss_config");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("RSS");
        t.name = QStringLiteral("NDIS receive side scaling configuration");
        t.description = QStringLiteral("Distributes network receive processing across CPUs (RssBaseCpu=1 avoids core 0), limits to 4 RSS CPUs, disables NBL owner tracking and NIC power saving.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("RSS base CPU change requires NIC driver restart"),
            QStringLiteral("DefaultPnPCapabilities disable may prevent NIC sleep"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NDIS\\Parameters"), QStringLiteral("RssBaseCpu"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NDIS\\Parameters"), QStringLiteral("MaxNumRssCpus"), 4u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NDIS\\Parameters"), QStringLiteral("TrackNblOwner"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NDIS\\Parameters"), QStringLiteral("DefaultPnPCapabilities"), 36u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.nla.active_probing_disable");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("Connectivity probes");
        t.name = QStringLiteral("Disable NLA active probing");
        t.description = QStringLiteral("Disables NLA active probing to prevent Windows from contacting external servers to verify connectivity. Sets minimum internet hop count to 2 for faster connectivity determination.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Captive portal detection disabled"),
            QStringLiteral("Network icon may show incorrect status"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NlaSvc\\Parameters\\Internet"), QStringLiteral("EnableActiveProbing"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NlaSvc\\Parameters\\Internet"), QStringLiteral("MinimumInternetHopCount"), 2u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.nla.custom_probe_endpoints");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("Connectivity probes");
        t.name = QStringLiteral("Custom NLA probe endpoints");
        t.description = QStringLiteral("Redirects NLA connectivity probe endpoints to OpenDNS and Firefox detect portal instead of default Microsoft servers for improved privacy.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Probes will fail if custom endpoints are unreachable"),
        };
        t.registryOps = {
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NlaSvc\\Parameters\\Internet"), QStringLiteral("ActiveDnsProbeContent"), QStringLiteral("208.67.222.222"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NlaSvc\\Parameters\\Internet"), QStringLiteral("ActiveDnsProbeContentV6"), QStringLiteral("2620:119:35::35"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NlaSvc\\Parameters\\Internet"), QStringLiteral("ActiveDnsProbeHost"), QStringLiteral("resolver1.opendns.com"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NlaSvc\\Parameters\\Internet"), QStringLiteral("ActiveDnsProbeHostV6"), QStringLiteral("resolver1.opendns.com"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NlaSvc\\Parameters\\Internet"), QStringLiteral("ActiveWebProbeContent"), QStringLiteral("success"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NlaSvc\\Parameters\\Internet"), QStringLiteral("ActiveWebProbeContentV6"), QStringLiteral("success"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NlaSvc\\Parameters\\Internet"), QStringLiteral("ActiveWebProbeHost"), QStringLiteral("detectportal.firefox.com"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NlaSvc\\Parameters\\Internet"), QStringLiteral("ActiveWebProbeHostV6"), QStringLiteral("detectportal.firefox.com"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NlaSvc\\Parameters\\Internet"), QStringLiteral("ActiveWebProbePath"), QStringLiteral("success.txt"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NlaSvc\\Parameters\\Internet"), QStringLiteral("ActiveWebProbePathV6"), QStringLiteral("success.txt"), std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NlaSvc\\Parameters\\Internet"), QStringLiteral("CaptivePortalTimer"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NlaSvc\\Parameters\\Internet"), QStringLiteral("CaptivePortalTimerBackOffIncrementsInSeconds"), 5u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NlaSvc\\Parameters\\Internet"), QStringLiteral("CaptivePortalTimerMaxInSeconds"), 30u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NlaSvc\\Parameters\\Internet"), QStringLiteral("PassivePollPeriod"), 15u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NlaSvc\\Parameters\\Internet"), QStringLiteral("StaleThreshold"), 30u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NlaSvc\\Parameters\\Internet"), QStringLiteral("WebTimeout"), 35u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.nla.detection_disable");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("Connectivity probes");
        t.name = QStringLiteral("Disable NLA gateway detection");
        t.description = QStringLiteral("Disables Network Location Awareness gateway detection and reduces NLA corporate location probe timeout. Prevents Windows from periodically probing the network to determine connection type (Public/Private/Domain).");
        t.iconName = QStringLiteral("wifi");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Windows may misclassify the network as Public; domain-aware apps and firewall profiles can misbehave"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\services\\NlaSvc\\Parameters\\Internet"), QStringLiteral("EnableNoGatewayLocationDetection"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\services\\NlaSvc\\Parameters\\Internet"), QStringLiteral("CorpLocationProbeTimeout"), 30u, 60u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.peernet.disable");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("Legacy protocols");
        t.name = QStringLiteral("Disable Windows peer-to-peer networking");
        t.description = QStringLiteral("Disables Windows Peer Networking and PNRP (Peer Name Resolution Protocol) across all IPv6 scopes including multicast bootstrap. Reduces network overhead and attack surface.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Windows P2P networking broken"),
            QStringLiteral("PNRP name resolution unavailable"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Peernet"), QStringLiteral("Disabled"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Peernet\\Pnrp\\IPv6-Global"), QStringLiteral("DisableMulticastBootstrap"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Peernet\\Pnrp\\IPv6-Global"), QStringLiteral("Disabled"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Peernet\\Pnrp\\IPv6-Local"), QStringLiteral("DisableMulticastBootstrap"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Peernet\\Pnrp\\IPv6-Local"), QStringLiteral("Disabled"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Peernet\\Pnrp\\IPv6-SiteLocal"), QStringLiteral("DisableMulticastBootstrap"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Peernet\\Pnrp\\IPv6-SiteLocal"), QStringLiteral("Disabled"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.qos.diffserv_mapping");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("QoS");
        t.name = QStringLiteral("Configure QoS DiffServ byte mappings");
        t.description = QStringLiteral("Sets DiffServ conforming and non-conforming byte mappings for Guaranteed (0x2E/EF) and NetworkControl (0x38/CS7) service types, plus user priority mappings.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Custom DSCP markings are only honored if your router and ISP respect them; home networks typically ignore them"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Psched\\DiffservByteMappingConforming"), QStringLiteral("ServiceTypeGuaranteed"), 46u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Psched\\DiffservByteMappingConforming"), QStringLiteral("ServiceTypeNetworkControl"), 56u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Psched\\DiffservByteMappingNonConforming"), QStringLiteral("ServiceTypeGuaranteed"), 46u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Psched\\DiffservByteMappingNonConforming"), QStringLiteral("ServiceTypeNetworkControl"), 56u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Psched\\UserPriorityMapping"), QStringLiteral("ServiceTypeGuaranteed"), 5u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Psched\\UserPriorityMapping"), QStringLiteral("ServiceTypeNetworkControl"), 7u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.qos.packet_scheduler");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("QoS");
        t.name = QStringLiteral("Enable QoS packet scheduler");
        t.description = QStringLiteral("Ensures the QoS Packet Scheduler service and driver are enabled, required for DSCP traffic marking to work.");
        t.iconName = QStringLiteral("wifi");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Re-enabling Psched on a system that intentionally removed it reintroduces the 20 percent reservable-bandwidth filter"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Psched"), QStringLiteral("Start"), 1u, 4u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.rasman.ppp_security");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("VPN & RAS");
        t.name = QStringLiteral("RAS manager PPP security settings");
        t.description = QStringLiteral("Configures Remote Access Service PPP settings: disables restart timer, forces encrypted data and passwords, and enables secure VPN mode.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("May prevent connection to legacy unencrypted VPN servers"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\RasMan\\PPP"), QStringLiteral("RestartTimer"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\RasMan\\PPP"), QStringLiteral("ForceEncryptedData"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\RasMan\\PPP"), QStringLiteral("ForceEncryptedPassword"), 2u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\RasMan\\PPP"), QStringLiteral("SecureVPN"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.smb.network_provider");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("SMB");
        t.name = QStringLiteral("Network provider and wake-on-LAN settings");
        t.description = QStringLiteral("Enables automatic connection restore and disables Wake-on-LAN to prevent unwanted wake events.");
        t.iconName = QStringLiteral("wifi");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Wake-on-LAN disabled cannot wake PC remotely"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\NetworkProvider"), QStringLiteral("RestoreConnection"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\NetworkProvider"), QStringLiteral("WakeUp"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.smb.throttle_disable");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("SMB");
        t.name = QStringLiteral("Disable SMB bandwidth throttling");
        t.description = QStringLiteral("Removes SMB client bandwidth throttling (DisableBandwidthThrottling=1), enables large MTU, and increases LanmanServer IRP stack size for complex I/O.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("SMB may consume more bandwidth on shared networks"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanWorkstation\\Parameters"), QStringLiteral("DisableBandwidthThrottling"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanWorkstation\\Parameters"), QStringLiteral("DisableLargeMtu"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters"), QStringLiteral("IRPStackSize"), 100u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.tcp.connection_rate");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("TCP/IP");
        t.name = QStringLiteral("Disable TCP connection rate limiting");
        t.description = QStringLiteral("Removes TCP connection rate cap (EnableConnectionRateLimiting=0) for faster connection establishment.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Apps opening thousands of simultaneous TCP connections may trip ISP abuse heuristics or NAT state limits"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("EnableConnectionRateLimiting"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("TcpCreateAndConnectTcbRateLimitDepth"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.wcm.cellular_failover_disable");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("Wi-Fi & WCM");
        t.name = QStringLiteral("Disable cellular failover when Wi-Fi is poor");
        t.description = QStringLiteral("Prevents Windows from automatically switching to cellular data when Wi-Fi signal is poor. Saves mobile data usage.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("May lose connectivity when Wi-Fi is unstable"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\WcmSvc\\CellularFailover"), QStringLiteral("AllowFailover"), 0u, 2u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.webdav.optimization");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("WebDAV");
        t.name = QStringLiteral("WebDAV client parameter optimization");
        t.description = QStringLiteral("Optimizes WebDAV (WebClient) service parameters: removes file size limit, reduces server timeouts to 5 seconds, sets send/receive timeout to 15 seconds, and reduces server not found cache lifetime.");
        t.iconName = QStringLiteral("globe");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Aggressive timeouts may cause failures on slow connections"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\WebClient\\Parameters"), QStringLiteral("FileSizeLimitInBytes"), 4294967295u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\WebClient\\Parameters"), QStringLiteral("InternetServerTimeoutInSec"), 5u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\WebClient\\Parameters"), QStringLiteral("LocalServerTimeoutInSec"), 5u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\WebClient\\Parameters"), QStringLiteral("SendReceiveTimeoutInSec"), 15u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\WebClient\\Parameters"), QStringLiteral("ServerNotFoundCacheLifeTimeInSec"), 15u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\WebClient\\Parameters"), QStringLiteral("ServiceDebug"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.wifi.connection_settings");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("Wi-Fi & WCM");
        t.name = QStringLiteral("Wi-Fi Sense and hotspot security settings");
        t.description = QStringLiteral("Disables WiFi Sense credential sharing, social network WiFi sharing (Facebook, Skype), hotspot auto-connect, hotspot authentication and reporting, WCN UI, and configures soft disconnect and minimize connections policies.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("WiFi Sense features disabled"),
            QStringLiteral("Hotspot auto-connect disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DataCollection\\Default\\WifiAutoConnectConfig"), QStringLiteral("AutoConnectEnabled"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\WiFi\\AllowAutoConnectToWiFiSenseHotspots"), QStringLiteral("Value"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\WcmSvc\\wifinetworkmanager\\SocialNetworks\\ABCH"), QStringLiteral("OptInStatus"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\WcmSvc\\wifinetworkmanager\\SocialNetworks\\ABCH-SKYPE"), QStringLiteral("OptInStatus"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\WcmSvc\\wifinetworkmanager\\SocialNetworks\\FACEBOOK"), QStringLiteral("OptInStatus"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\WcmSvc\\wifinetworkmanager\\config"), QStringLiteral("AutoConnectAllowedOEM"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\WcmSvc\\wifinetworkmanager"), QStringLiteral("WiFiSenseCredShared"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\WcmSvc\\wifinetworkmanager"), QStringLiteral("WiFiSenseOpen"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WcmSvc\\GroupPolicy"), QStringLiteral("fSoftDisconnectConnections"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WCN\\UI"), QStringLiteral("DisableWcnUi"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\HotspotAuthentication"), QStringLiteral("Enabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\WiFi\\AllowWiFiHotSpotReporting"), QStringLiteral("value"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\WlanSvc\\AnqpCache"), QStringLiteral("OsuRegistrationStatus"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.winhttp.proxy_clear");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("Proxy");
        t.name = QStringLiteral("Clear unused WinHTTP proxy settings");
        t.description = QStringLiteral("Removes stale WinHTTP proxy configurations that can add latency to every HTTP request. Clears DefaultConnectionSettings and proxy override lists. Only apply if you do NOT use a corporate proxy.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Breaks connectivity if a proxy is actually required"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"), QStringLiteral("ProxyEnable"), 0u, std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"), QStringLiteral("ProxyOverride"), QStringLiteral(""), std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Connections"), QStringLiteral("WinHttpSettings"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.winlogon.cached_disable");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("Credentials & Security");
        t.name = QStringLiteral("Disable cached Windows logons");
        t.description = QStringLiteral("Sets cached logon count to zero to prevent storing domain credentials locally. Improves security on shared machines.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Domain users cannot log in when the domain controller is unreachable (VPN down, offline laptops)"),
        };
        t.registryOps = {
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"), QStringLiteral("CachedLogonsCount"), QStringLiteral("0"), QStringLiteral("10")),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.hpet.disable");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Timers");
        t.declaredTier = vth::models::ProductTier::Ultimate;
        // Sets the BCD flag in its name and doesn't disable HPET or dynamic tick. The
        // id keeps its older spelling because saved tweak state is keyed on it.
        t.name = QStringLiteral("Force the platform tick source");
        t.description = QStringLiteral("Runs 'bcdedit /set useplatformtick yes', forcing the platform tick source. It does not disable dynamic tick or HPET, which are separate BCD flags. Its latency benefit is debated and hardware-dependent.");
        t.iconName = QStringLiteral("clock");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        CommandOp bo0;
        bo0.program = QStringLiteral("bcdedit");
        bo0.applyArgs = { QStringLiteral("/set"), QStringLiteral("useplatformtick"), QStringLiteral("yes") };
        bo0.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("useplatformtick") };
        t.commandOps.push_back(bo0);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.disable-nagle");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Latency & Offloads");
        t.name = QStringLiteral("Disable Nagle's algorithm (per-NIC)");
        t.description = QStringLiteral("Disables Nagle's algorithm on every TCP/IP interface (per-NIC).");
        t.iconName = QStringLiteral("world-www");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("This removal is not fully reversible."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("Get-ChildItem 'HKLM:\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces' | ForEach-Object { Set-ItemProperty -Path $_.PSPath -Name TcpAckFrequency -Value 1 -Type DWord -ErrorAction SilentlyContinue; Set-ItemProperty -Path $_.PSPath -Name TCPNoDelay -Value 1 -Type DWord -ErrorAction SilentlyContinue }") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("Get-ChildItem 'HKLM:\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces' | ForEach-Object { Remove-ItemProperty -Path $_.PSPath -Name TcpAckFrequency -ErrorAction SilentlyContinue; Remove-ItemProperty -Path $_.PSPath -Name TCPNoDelay -ErrorAction SilentlyContinue }") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.disable-netbios");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Legacy protocols");
        t.name = QStringLiteral("Disable NetBIOS over TCP/IP");
        t.description = QStringLiteral("Disables NetBIOS over TCP/IP on every adapter. Removes NBT-NS poisoning attack surface.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("This removal is not fully reversible."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("Get-ChildItem 'HKLM:\\SYSTEM\\CurrentControlSet\\Services\\NetBT\\Parameters\\Interfaces' | Where-Object { $_.PSChildName -like 'Tcpip_*' } | ForEach-Object { Set-ItemProperty -Path $_.PSPath -Name NetbiosOptions -Value 2 -Type DWord -ErrorAction SilentlyContinue }") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("Get-ChildItem 'HKLM:\\SYSTEM\\CurrentControlSet\\Services\\NetBT\\Parameters\\Interfaces' | Where-Object { $_.PSChildName -like 'Tcpip_*' } | ForEach-Object { Set-ItemProperty -Path $_.PSPath -Name NetbiosOptions -Value 0 -Type DWord -ErrorAction SilentlyContinue }") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.disable-nic-flow-control");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Latency & Offloads");
        t.name = QStringLiteral("Disable NIC IEEE 802.3x flow control");
        t.description = QStringLiteral("Turns off the standard NDIS *FlowControl per-adapter setting.");
        t.iconName = QStringLiteral("network");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("This removal is not fully reversible."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("Set-NetAdapterAdvancedProperty -Name * -RegistryKeyword '*FlowControl' -RegistryValue '0' -ErrorAction SilentlyContinue") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("Set-NetAdapterAdvancedProperty -Name * -RegistryKeyword '*FlowControl' -RegistryValue '3' -ErrorAction SilentlyContinue") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.disable-nic-interrupt-moderation");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Latency & Offloads");
        t.name = QStringLiteral("Disable NIC adaptive interrupt moderation");
        t.description = QStringLiteral("Turns off the NIC's *InterruptModeration. Lowest latency at cost of more interrupts.");
        t.iconName = QStringLiteral("network");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Interrupt rate may climb to ~2.65M/s under heavy load, stealing CPU from other work."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("Set-NetAdapterAdvancedProperty -Name * -RegistryKeyword '*InterruptModeration' -RegistryValue '0' -ErrorAction SilentlyContinue") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("Set-NetAdapterAdvancedProperty -Name * -RegistryKeyword '*InterruptModeration' -RegistryValue '1' -ErrorAction SilentlyContinue") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.disable-nic-pm");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Power & Adapter");
        t.name = QStringLiteral("Disable NIC power management");
        t.description = QStringLiteral("Turns off the NIC's power-management knobs. Eliminates wake-from-D3 latency.");
        t.iconName = QStringLiteral("bolt");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("This removal is not fully reversible."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$keys = '*EnablePowerManagement','*EnableDynamicPowerGating','*EnablePME','*NicAutoPowerSaver'; foreach ($k in $keys) { Set-NetAdapterAdvancedProperty -Name * -RegistryKeyword $k -RegistryValue '0' -ErrorAction SilentlyContinue }") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$keys = '*EnablePowerManagement','*EnableDynamicPowerGating','*EnablePME','*NicAutoPowerSaver'; foreach ($k in $keys) { Set-NetAdapterAdvancedProperty -Name * -RegistryKeyword $k -RegistryValue '1' -ErrorAction SilentlyContinue }") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.disable-nic-wol");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Power & Adapter");
        t.name = QStringLiteral("Disable wake-on-LAN");
        t.description = QStringLiteral("Turns off *WakeOnMagicPacket and *WakeOnPattern on every adapter.");
        t.iconName = QStringLiteral("moon");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("This removal is not fully reversible."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$keys = '*WakeOnMagicPacket','*WakeOnPattern'; foreach ($k in $keys) { Set-NetAdapterAdvancedProperty -Name * -RegistryKeyword $k -RegistryValue '0' -ErrorAction SilentlyContinue }") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$keys = '*WakeOnMagicPacket','*WakeOnPattern'; foreach ($k in $keys) { Set-NetAdapterAdvancedProperty -Name * -RegistryKeyword $k -RegistryValue '1' -ErrorAction SilentlyContinue }") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.disable-rsc-ipv4");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Latency & Offloads");
        t.name = QStringLiteral("Disable receive segment coalescing (IPv4)");
        t.description = QStringLiteral("RSC merges inbound TCP segments at the NIC. Hurts latency on real-time traffic.");
        t.iconName = QStringLiteral("network");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("This removal is not fully reversible."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("Set-NetAdapterAdvancedProperty -Name * -RegistryKeyword '*RscIPv4' -RegistryValue '0' -ErrorAction SilentlyContinue") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("Set-NetAdapterAdvancedProperty -Name * -RegistryKeyword '*RscIPv4' -RegistryValue '1' -ErrorAction SilentlyContinue") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.disable-rsc-ipv6");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Latency & Offloads");
        t.name = QStringLiteral("Disable receive segment coalescing (IPv6)");
        t.description = QStringLiteral("Same as IPv4 variant; turns off receive coalescing for IPv6 traffic on all active NICs.");
        t.iconName = QStringLiteral("network");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("This removal is not fully reversible."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("Set-NetAdapterAdvancedProperty -Name * -RegistryKeyword '*RscIPv6' -RegistryValue '0' -ErrorAction SilentlyContinue") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("Set-NetAdapterAdvancedProperty -Name * -RegistryKeyword '*RscIPv6' -RegistryValue '1' -ErrorAction SilentlyContinue") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.afd.irp_security");
        t.category = QStringLiteral("networking");
        t.name = QStringLiteral("AFD IRP stack and security settings");
        t.description = QStringLiteral("Sets IRP stack size to 20, disables raw socket security, disables direct AcceptEx and chained receive for stability.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::Critical;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Disables raw-socket security and AcceptEx direct mode; malicious local code can open raw sockets more easily"),
            QStringLiteral("An incorrect IRP stack size can bugcheck the system under complex filter-driver network stacks"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AFD\\Parameters"), QStringLiteral("IRPStackSize"), 20u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AFD\\Parameters"), QStringLiteral("DisableRawSecurity"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AFD\\Parameters"), QStringLiteral("DisableDirectAcceptEx"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AFD\\Parameters"), QStringLiteral("DisableChainedReceive"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.attachment.lowrisk_filetype");
        t.category = QStringLiteral("networking");
        t.name = QStringLiteral("Low-risk file type attachment policy");
        t.description = QStringLiteral("Configures attachment handling to treat common file types as low-risk, reduces antivirus scanning friction, hides zone info on properties, and disables AppHost signature checking.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Reduced malware protection on downloads"),
            QStringLiteral("Zone information hidden from file properties"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Attachments"), QStringLiteral("ScanWithAntiVirus"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Attachments"), QStringLiteral("HideZoneInfoOnProperties"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Attachments"), QStringLiteral("DefaultFileTypeRisk"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Attachments"), QStringLiteral("ScanWithAntiVirus"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Attachments"), QStringLiteral("HideZoneInfoOnProperties"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Attachments"), QStringLiteral("DefaultFileTypeRisk"), 1u, std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Associations"), QStringLiteral("LowRiskFileTypes"), QStringLiteral(".zip;.rar;.nfo;.txt;.exe;.bat;.com;.cmd;.reg;.msi;.htm;.html;.gif;.bmp;.jpg;.avi;.mpg;.mpeg;.mov;.mp3;.m3u;.msu;.wav;"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppHost\\Download"), QStringLiteral("CheckExeSignatures"), QStringLiteral("no"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.cert.revocation_timeout");
        t.category = QStringLiteral("networking");
        t.name = QStringLiteral("Aggressive certificate revocation timeout");
        t.description = QStringLiteral("Lowers certificate revocation check timeouts to 14ms and sets cross-cert download interval to 168 hours to prevent TLS handshake stalls.");
        t.iconName = QStringLiteral("clock");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("14 ms CRL/OCSP timeout often fails silently on high-latency links; revoked certificates may be trusted"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\SystemCertificates\\ChainEngine\\Config"), QStringLiteral("ChainRevAccumulativeUrlRetrievalTimeoutMilliseconds"), 14u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\SystemCertificates\\ChainEngine\\Config"), QStringLiteral("ChainUrlRetrievalTimeoutMilliseconds"), 14u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\SystemCertificates\\ChainEngine\\Config"), QStringLiteral("CrossCertDownloadIntervalHours"), 168u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.tcp.nagle_disable");
        t.category = QStringLiteral("networking");
        t.name = QStringLiteral("Disable Nagle's algorithm");
        t.description = QStringLiteral("Disables TCP packet coalescing (Nagle's Algorithm) and delayed ACKs for lower network latency.");
        t.iconName = QStringLiteral("wifi");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Slightly increased network overhead on bandwidth-constrained connections"),
        };
        t.registryOps = {
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("TcpNoDelay"), QStringLiteral("1"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("TcpAckFrequency"), QStringLiteral("1"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("DelayedAckFrequency"), QStringLiteral("1"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("DelayedAckTicks"), QStringLiteral("1"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.qos.bandwidth_reserve");
        t.category = QStringLiteral("networking");
        t.name = QStringLiteral("Disable QoS bandwidth reserve");
        t.description = QStringLiteral("Removes the reserved QoS bandwidth (NonBestEffortLimit=0), tightens the QoS timer, and disables NLA-based QoS classification.");
        t.iconName = QStringLiteral("wifi");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("QoS-aware apps (enterprise VoIP, WAN optimizers) lose their reserved bandwidth allotment"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Psched"), QStringLiteral("TimerResolution"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Psched"), QStringLiteral("MaxOutstandingSends"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Psched"), QStringLiteral("NonBestEffortLimit"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\QoS"), QStringLiteral("Do not use NLA"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.disable-task-offload");
        t.category = QStringLiteral("networking");
        t.name = QStringLiteral("Disable TCP/IP task offload");
        t.description = QStringLiteral("Stops the TCP/IP stack from offloading checksums and segmentation to NIC firmware. This moves work onto the CPU and generally hurts throughput - it is a troubleshooting step, not an optimization.");
        t.iconName = QStringLiteral("route-off");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("kills NIC hardware offload (raises CPU, lowers throughput); opt-in only, never recommended.");
        t.warnings = {
            QStringLiteral("Moves checksum/segmentation work from the NIC to the CPU - usually lowers throughput and raises CPU on high-bandwidth links."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("DisableTaskOffload"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.disable-tcp-delayed-ack");
        t.category = QStringLiteral("networking");
        t.name = QStringLiteral("Disable TCP delayed ACK");
        t.description = QStringLiteral("Stops Windows from delaying TCP ACKs to coalesce them. Companion to Nagle disable.");
        t.iconName = QStringLiteral("network");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("TcpDelAckTicks"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.dns.cache_tuning");
        t.category = QStringLiteral("networking");
        t.name = QStringLiteral("DNS cache and service optimization");
        t.description = QStringLiteral("Tightens DNS cache TTLs (MaxCacheTtl=1), increases UDP packet size, disables parallel A/AAAA queries, and reduces SOA cache entry TTL for aggressive cache refresh.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("MaxCacheTtl=1 forces repeated DNS lookups that can slow browsing and increase resolver load"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Dnscache\\Parameters"), QStringLiteral("MaxSOACacheEntryTtlLimit"), 301u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Dnscache\\Parameters"), QStringLiteral("ServiceDllUnloadOnStop"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Dnscache\\Parameters"), QStringLiteral("DisableParallelAandAAAA"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Dnscache\\Parameters"), QStringLiteral("MaxCacheTtl"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Dnscache\\Parameters"), QStringLiteral("MaximumUdpPacketSize"), 4864u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Dnscache\\Parameters"), QStringLiteral("DisableSmartNameResolution"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.ie.download_signature_bypass");
        t.category = QStringLiteral("networking");
        t.name = QStringLiteral("Disable open file security dialog");
        t.description = QStringLiteral("Disables executable signature checking on downloads, allows running files with invalid signatures, and saves zone information in attachments. Reduces download friction.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Downloaded executables with missing or invalid Authenticode signatures run with no warning"),
            QStringLiteral("Mark-of-the-Web protections are weakened so files from the internet bypass SmartScreen and attachment-execution prompts"),
        };
        t.registryOps = {
            stringOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Internet Explorer\\Download"), QStringLiteral("CheckExeSignatures"), QStringLiteral("no"), std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Internet Explorer\\Download"), QStringLiteral("RunInvalidSignatures"), 1u, std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Internet Explorer\\Download"), QStringLiteral("CheckExeSignatures"), QStringLiteral("no"), std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Internet Explorer\\Download"), QStringLiteral("RunInvalidSignatures"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Attachments"), QStringLiteral("SaveZoneInformation"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Attachments"), QStringLiteral("SaveZoneInformation"), 1u, std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Associations"), QStringLiteral("LowRiskFileTypes"), QStringLiteral(".zip;.rar;.nfo;.txt;.exe;.bat;.com;.cmd;.reg;.msi;.htm;.html;.gif;.bmp;.jpg;.avi;.mpg;.mpeg;.mov;.mp3;.m3u;.wav;"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Associations"), QStringLiteral("ModRiskFileTypes"), QStringLiteral(".bat;.exe;.reg;.vbs;.chm;.msi;.js;.cmd"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Associations"), QStringLiteral("LowRiskFileTypes"), QStringLiteral(".zip;.rar;.nfo;.txt;.exe;.bat;.com;.cmd;.reg;.msi;.htm;.html;.gif;.bmp;.jpg;.avi;.mpg;.mpeg;.mov;.mp3;.m3u;.wav;"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Associations"), QStringLiteral("ModRiskFileTypes"), QStringLiteral(".bat;.exe;.reg;.vbs;.chm;.msi;.js;.cmd"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.qos.dscp_gaming");
        t.category = QStringLiteral("networking");
        t.name = QStringLiteral("Mark outbound traffic DSCP EF (46)");
        // Every selector below is 0, which is the QoS wizard's "All
        // applications", "Any source IP address" and "any port". Microsoft's
        // Manage QoS Policy documents that as matching everything the machine
        // sends, so nothing here can single out a game.
        t.description = QStringLiteral("Writes a machine-wide QoS policy that marks outbound packets with DSCP Expedited Forwarding (EF/46). It sets no application, port, protocol or address condition, so it applies to everything this PC sends.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("DSCP tagging only helps if upstream routers honor it; many consumer ISPs strip or ignore the bits"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\QoS\\Gaming Traffic"), QStringLiteral("Application Name"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\QoS\\Gaming Traffic"), QStringLiteral("DSCP Value"), 46u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\QoS\\Gaming Traffic"), QStringLiteral("Local IP"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\QoS\\Gaming Traffic"), QStringLiteral("Local IP Prefix Length"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\QoS\\Gaming Traffic"), QStringLiteral("Local Port"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\QoS\\Gaming Traffic"), QStringLiteral("Protocol"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\QoS\\Gaming Traffic"), QStringLiteral("Remote IP"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\QoS\\Gaming Traffic"), QStringLiteral("Remote IP Prefix Length"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\QoS\\Gaming Traffic"), QStringLiteral("Remote Port"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\QoS\\Gaming Traffic"), QStringLiteral("Throttle Rate"), 4294967295u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\QoS\\Gaming Traffic"), QStringLiteral("Version"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.tcp.autotuning_persistent");
        t.category = QStringLiteral("networking");
        t.name = QStringLiteral("Persistent TCP AutoTuning and QoS internet settings");
        t.description = QStringLiteral("Enables persistent TCP autotuning in WinHTTP and Internet Settings (both 32-bit and 64-bit paths), sets QoS autotuning level to Experimental with DSCP marking allowed, enables IPv6 transition autotuning, and sets TCP initial RTT and hybrid ACK. \"Experimental\" autotuning is not universally better - measure before keeping.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.concern = QStringLiteral("\"Experimental\" TCP autotuning helps some links and hurts others; opt-in only, never recommended.");
        t.warnings = {
            QStringLiteral("Experimental autotuning scales windows beyond ISP expectations; some consumer ISPs respond by silently dropping packets"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\WinHttp"), QStringLiteral("TcpAutotuning"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"), QStringLiteral("TcpAutotuning"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\WinHttp"), QStringLiteral("TcpAutotuning"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"), QStringLiteral("TcpAutotuning"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("TcpInitialRTT"), 12288u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("TcpHybridAck"), 0u, std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\QoS"), QStringLiteral("Tcp Autotuning Level"), QStringLiteral("Experimental"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\QoS"), QStringLiteral("Application DSCP Marking Request"), QStringLiteral("Allowed"), std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\TCPIP\\v6Transition"), QStringLiteral("TcpAutotuningLevel"), 3u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.tcp.core_tuning");
        t.category = QStringLiteral("networking");
        t.name = QStringLiteral("TCP core parameters");
        t.description = QStringLiteral("Sets core TCP parameters: window sizes, retransmission limits, forwarding buffers, ARP cache, connection limits, TTL, and SACK.");
        t.iconName = QStringLiteral("wifi");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Changes low-level TCP behavior for every network connection on this PC"),
            QStringLiteral("Increased memory usage from enlarged forwarding buffer and connection-table pools"),
            QStringLiteral("If a specific app or VPN misbehaves after applying, reverting this tweak is the first thing to try"),
        };
        t.registryOps = {
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("Tcp1323Opts"), QStringLiteral("1"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("TcpTimedWaitDelay"), QStringLiteral("30"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("GlobalMaxTcpWindowSize"), QStringLiteral("65535"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("TcpWindowSize"), QStringLiteral("65535"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("DefaultTTL"), QStringLiteral("64"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("TcpMaxDupAcks"), QStringLiteral("2"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("MaxUserPort"), QStringLiteral("65534"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("TcpFinWaitDelay"), QStringLiteral("6"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("DisableIPSourceRouting"), QStringLiteral("2"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("DisableTaskOffload"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("TcpMaxDataRetransmissions"), QStringLiteral("5"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("TcpMaxConnectRetransmissions"), QStringLiteral("2"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("TcpMaxConnectResponseRetransmissions"), QStringLiteral("2"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("ForwardBufferMemory"), QStringLiteral("1376278"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("MaxForwardBufferMemory"), QStringLiteral("1376278"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("NumForwardPackets"), QStringLiteral("1414"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("MaxNumForwardPackets"), QStringLiteral("1414"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("MaxFreeTcbs"), QStringLiteral("65535"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("MaxHashTableSize"), QStringLiteral("65535"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("MaxFreeTWTcbs"), QStringLiteral("65536"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("ArpAlwaysSourceRoute"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("ArpCacheLife"), QStringLiteral("6144"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("ArpCacheMinReferencedLife"), QStringLiteral("13824"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("ArpCacheSize"), QStringLiteral("512"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("MaxConnectionsPer1_0Server"), QStringLiteral("10"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("MaxConnectionsPerServer"), QStringLiteral("10"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("EnableFastRouteLookup"), QStringLiteral("1"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("SynAttackProtect"), QStringLiteral("1"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("EnableTCPA"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("DeadGWDetectDefault"), QStringLiteral("1"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("SackOpts"), QStringLiteral("1"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.tcp.extended_optimization");
        t.category = QStringLiteral("networking");
        t.name = QStringLiteral("Extended TCP/IP and server optimization");
        t.description = QStringLiteral("Applies additional TCP/IP optimizations: LanmanWorkstation offline files policy, LanmanServer security/compression settings, SizReqBuf (~95KB), ThreadPriority=0, MSMQ TCP NoDelay, WinHTTP tracing disable, DNS query IP matching, UDP fast send, Winsock parameters, and extended Tcpip parameters.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("SMB security signing is turned off; man-in-the-middle tampering of file-share traffic is no longer detected"),
            QStringLiteral("IPEnableRouter=1 turns the PC into an IP forwarder; unexpected routing behavior can occur on multihomed hosts"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\LanmanWorkstation"), QStringLiteral("AllowOfflineFilesforCAShares"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters"), QStringLiteral("AutoShareWks"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters"), QStringLiteral("DisableCompression"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters"), QStringLiteral("EnableAuthenticateUserSharing"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters"), QStringLiteral("ServiceDllUnloadOnStop"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters"), QStringLiteral("autodisconnect"), 15u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters"), QStringLiteral("EnableSecuritySignature"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters"), QStringLiteral("RequireSecuritySignature"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters"), QStringLiteral("restrictnullsessaccess"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters"), QStringLiteral("SMB1"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters"), QStringLiteral("SizReqBuf"), 95268u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer"), QStringLiteral("ThreadPriority"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\MSMQ\\Parameters"), QStringLiteral("TCPNoDelay"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\MSMQ\\Parameters\\Security"), QStringLiteral("SecureDSCommunication"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\WinHttp\\Tracing"), QStringLiteral("Enabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Dnscache\\Parameters"), QStringLiteral("QueryIpMatching"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Internet Connection Wizard"), QStringLiteral("ExitOnMSICW"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ZoneMap"), QStringLiteral("AutoDetect"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"), QStringLiteral("AutoDetect"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\UDP\\Parameters"), QStringLiteral("EnableUDPFastSend"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("TcpUseRFC1122UrgentPointe"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("TcpMaxSendFree"), 415029u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("TCPCongestionControl"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("CongestionAlgorithm"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("EnableDCA"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("DisableLargeMtu"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("AllowUnqualifiedQuery"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("DisableMediaSenseEventLog"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("DisableRss"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("DisableTcpChimneyOffload"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("EnableAddrMaskReply"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("EnableHeuristics"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("EnableIPAutoConfigurationLimits"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("KeepAliveTime"), 7200000u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("QualifyingDestinationThreshold"), 3u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("SyncDomainWithMembership"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("StrictTimeWaitSeqCheck"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("NumTcbTablePartitions"), 34u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("MultihopSets"), 15u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("IPEnableRouter"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("DnsOutstandingQueriesCount"), 1000u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("TCP1323Opts"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.winsock.params");
        t.category = QStringLiteral("networking");
        t.name = QStringLiteral("Winsock socket parameters");
        t.description = QStringLiteral("Configures Winsock parameters: disables delayed acceptance for faster connection setup, and sets socket address lengths.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Restricting socket address length to 16 breaks IPv6 or Bluetooth sockets which need longer sockaddr structures"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Winsock"), QStringLiteral("UseDelayedAcceptance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Winsock"), QStringLiteral("MaxSockAddrLength"), 16u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Winsock"), QStringLiteral("MinSockAddrLength"), 16u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.cap-rss-queues");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Cap RSS queues to 2");
        t.description = QStringLiteral("Receive-Side Scaling above two queues rarely helps consumer NICs and frequently increases jitter.");
        t.iconName = QStringLiteral("router");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("This removal is not fully reversible."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("Set-NetAdapterAdvancedProperty -Name * -RegistryKeyword '*NumRssQueues' -RegistryValue '2' -ErrorAction SilentlyContinue") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("Set-NetAdapterAdvancedProperty -Name * -RegistryKeyword '*NumRssQueues' -RegistryValue '8' -ErrorAction SilentlyContinue") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.disable-nic-checksum-offload");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Disable NIC IPv4 checksum offload");
        t.description = QStringLiteral("Turns off *IPChecksumOffloadIPv4 / *TCPChecksumOffloadIPv4 / *UDPChecksumOffloadIPv4 on every NIC.");
        t.iconName = QStringLiteral("router");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("This removal is not fully reversible."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$keys = '*IPChecksumOffloadIPv4','*TCPChecksumOffloadIPv4','*UDPChecksumOffloadIPv4'; foreach ($k in $keys) { Set-NetAdapterAdvancedProperty -Name * -RegistryKeyword $k -RegistryValue '0' -ErrorAction SilentlyContinue }") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$keys = '*IPChecksumOffloadIPv4','*TCPChecksumOffloadIPv4','*UDPChecksumOffloadIPv4'; foreach ($k in $keys) { Set-NetAdapterAdvancedProperty -Name * -RegistryKeyword $k -RegistryValue '3' -ErrorAction SilentlyContinue }") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.disable-nic-jumbo-packet");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Set NIC MTU to standard 1514 (no jumbo frames)");
        t.description = QStringLiteral("Sets *JumboPacket=1514 (standard Ethernet frame size). Disables jumbo-frame support.");
        t.iconName = QStringLiteral("router");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("This removal is not fully reversible."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("Set-NetAdapterAdvancedProperty -Name * -RegistryKeyword '*JumboPacket' -RegistryValue '1514' -ErrorAction SilentlyContinue") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("Set-NetAdapterAdvancedProperty -Name * -RegistryKeyword '*JumboPacket' -RegistryValue '9014' -ErrorAction SilentlyContinue") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.disable-nic-lso");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Disable NIC large send offload (TSO/LSO)");
        t.description = QStringLiteral("Turns off LSO v1 IPv4, LSO v2 IPv4, and LSO v2 IPv6 on every adapter.");
        t.iconName = QStringLiteral("router");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("This removal is not fully reversible."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$keys = '*LsoV1IPv4','*LsoV2IPv4','*LsoV2IPv6'; foreach ($k in $keys) { Set-NetAdapterAdvancedProperty -Name * -RegistryKeyword $k -RegistryValue '0' -ErrorAction SilentlyContinue }") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$keys = '*LsoV1IPv4','*LsoV2IPv4','*LsoV2IPv6'; foreach ($k in $keys) { Set-NetAdapterAdvancedProperty -Name * -RegistryKeyword $k -RegistryValue '1' -ErrorAction SilentlyContinue }") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.ps.stack_tuning");
        t.category = QStringLiteral("networking");
        t.name = QStringLiteral("Advanced network stack settings");
        t.description = QStringLiteral("Applies comprehensive TCP/IP stack optimizations via netsh and PowerShell: sets autotuning to experimental, disables scaling heuristics, sets CTCP congestion provider, disables packet coalescing and chimney offload, enables RSS while disabling RSC, enables TCP FastOpen and ECN, disables ISATAP tunnel, increases neighbor cache, and disables HyStart slow-start.");
        t.iconName = QStringLiteral("activity");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Experimental autotuning may cause throughput regressions"),
            QStringLiteral("Disabling chimney offload increases CPU usage"),
            QStringLiteral("CTCP may not suit high-latency connections"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("Set-NetTCPSetting") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("Set-NetTCPSetting") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }
}

} // namespace vth::tweaks::detail
