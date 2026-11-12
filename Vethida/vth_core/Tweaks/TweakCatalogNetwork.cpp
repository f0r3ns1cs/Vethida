#include "TweakEngine.h"

// Network registry tweaks, global keys only. Values that match the Windows
// default are left out, since writing them changes nothing.

namespace vth::tweaks::detail {

    namespace {
        const QString kTcpip = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters");
        const QString kDns   = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Dnscache\\Parameters");
        const QString kQos   = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\QoS");
        const QString kNcsiInt = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NlaSvc\\Parameters\\Internet");
        const QString kNcsiPol = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\NetworkConnectivityStatusIndicator");
        const QString kDo    = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DeliveryOptimization");
    }

    void appendNetwork(QVector<TweakSpec>& c) {
        // LLMNR, Remote Desktop and Remote Assistance are covered by net.disable-llmnr
        // and sys.security.disable_rdp.
        {
            TweakSpec t;
            t.id = QStringLiteral("net.disable-ipv6");
            t.category = QStringLiteral("networking");
            t.subCategory = QStringLiteral("IPv6");
            t.name = QStringLiteral("Disable IPv6 stack");
            t.description = QStringLiteral(
                "Disables every IPv6 prefix and tunneling component (Teredo, ISATAP, "
                "6to4) by setting DisabledComponents to 0xFF.");
            t.iconName = QStringLiteral("network");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "Microsoft does not recommend disabling IPv6 wholesale; leave it "
                "alone unless you have a specific reason. Most home networks are "
                "IPv4-only and won't notice.");
            t.warnings = {
                QStringLiteral("Enterprise or VPN setups that depend on IPv6 may stop working."),
            };
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip6\\Parameters"),
                        QStringLiteral("DisabledComponents"), 0xFFu, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("net.disable-smb1");
            t.category = QStringLiteral("networking");
            t.subCategory = QStringLiteral("SMB");
            t.name = QStringLiteral("Disable SMB1");
            t.description = QStringLiteral(
                "Turns off SMB protocol version 1 on both the server and client side. "
                "SMB1 is deprecated and was the vector for WannaCry and NotPetya; "
                "Microsoft recommends disabling it on every Windows 10/11 install.");
            t.iconName = QStringLiteral("shield");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Only affects shares hosted by old NAS, printer, or "
                               "Windows-XP-era devices. Modern shares use SMB2/3 and are unaffected."),
            };
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters"),
                        QStringLiteral("SMB1"), 0u, std::nullopt),
                // mrxsmb10 is the SMB1 client driver; Start=4 is Disabled, 2 is the
                // stock Automatic.
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\mrxsmb10"),
                        QStringLiteral("Start"), 4u, 2u),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("net.dns-disable-negative-cache");
            t.category = QStringLiteral("networking");
            t.subCategory = QStringLiteral("DNS");
            t.name = QStringLiteral("Disable negative DNS cache");
            t.description = QStringLiteral(
                "Stops Windows caching failed DNS lookups so it retries immediately "
                "instead of waiting out a cached failure.");
            t.iconName = QStringLiteral("globe");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.registryOps = {
                dwordOp(kDns, QStringLiteral("MaxNegativeCacheTtl"), 0, std::nullopt),
                dwordOp(kDns, QStringLiteral("NegativeSOACacheTime"), 0, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("net.qos-dscp-ef");
            t.category = QStringLiteral("networking");
            t.subCategory = QStringLiteral("QoS");
            t.name = QStringLiteral("Mark traffic expedited (DSCP EF)");
            t.description = QStringLiteral(
                "Tags outbound traffic with DSCP 46 (Expedited Forwarding) so QoS-aware "
                "routers prioritize it.");
            t.iconName = QStringLiteral("network");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "only effective if your router/ISP honors DSCP; most consumer routers "
                "ignore or strip it, making this a no-op on typical home networks.");
            t.registryOps = {
                dwordOp(kQos, QStringLiteral("DefaultDSCPValue"), 46, std::nullopt),
                dwordOp(kQos, QStringLiteral("Do not use NLA"), 1, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("net.ncsi-disable-probe");
            t.category = QStringLiteral("networking");
            t.subCategory = QStringLiteral("Connectivity probes");
            t.name = QStringLiteral("Disable connectivity probe (NCSI)");
            t.description = QStringLiteral(
                "Stops the periodic background probe to Microsoft's connectivity-check "
                "server.");
            t.iconName = QStringLiteral("globe");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.warnings = { QStringLiteral("The tray network icon may show a warning even when online.") };
            t.registryOps = {
                dwordOp(kNcsiInt, QStringLiteral("EnableActiveProbing"), 0, 1),
                dwordOp(kNcsiPol, QStringLiteral("NoActiveProbe"), 1, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("net.delivery-optimization-off");
            t.category = QStringLiteral("networking");
            t.subCategory = QStringLiteral("Delivery Optimization");
            t.name = QStringLiteral("Disable Delivery Optimization");
            t.description = QStringLiteral(
                "Turns off peer-to-peer Windows Update/Store content sharing.");
            t.iconName = QStringLiteral("network");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.registryOps = { dwordOp(kDo, QStringLiteral("DODownloadMode"), 0, std::nullopt) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("net.cellular-failover-off");
            t.category = QStringLiteral("networking");
            t.subCategory = QStringLiteral("Cellular");
            t.name = QStringLiteral("Disable cellular failover");
            t.description = QStringLiteral("Stops Windows falling back to a cellular connection.");
            t.iconName = QStringLiteral("network");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral("only relevant on laptops/tablets with a cellular modem.");
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\WwanSvc\\Parameters\\CellularFailover"),
                        QStringLiteral("DisableCellularFailover"), 1, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("net.tcp1323opts-off");
            t.category = QStringLiteral("networking");
            t.name = QStringLiteral("Disable TCP timestamps");
            t.description = QStringLiteral("Disables RFC 1323 TCP timestamps, saving 12 bytes per packet.");
            t.iconName = QStringLiteral("network");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "risky: Tcp1323Opts also governs window scaling; forcing 0 can reduce "
                "throughput on high-bandwidth/high-latency links. Marginal benefit.");
            t.registryOps = { dwordOp(kTcpip, QStringLiteral("Tcp1323Opts"), 0, std::nullopt) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("net.sackopts-off");
            t.category = QStringLiteral("networking");
            t.name = QStringLiteral("Disable selective ACKs");
            t.description = QStringLiteral("Disables TCP Selective Acknowledgments (SACK).");
            t.iconName = QStringLiteral("network");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "risky: SACK improves recovery on lossy links; disabling it hurts more "
                "than it helps on real-world connections. Placebo for latency.");
            t.registryOps = { dwordOp(kTcpip, QStringLiteral("SackOpts"), 0, 1) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("net.afd-buffers");
            t.category = QStringLiteral("networking");
            t.name = QStringLiteral("Winsock (AFD) buffers");
            t.description = QStringLiteral(
                "Sets fixed AFD receive/send windows and small fast-path thresholds for "
                "Winsock sockets.");
            t.iconName = QStringLiteral("network");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "placebo on modern Windows: these values match the auto-tuned defaults, so "
                "the effect is negligible.");
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AFD\\Parameters"),
                        QStringLiteral("DefaultReceiveWindow"), 65536, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AFD\\Parameters"),
                        QStringLiteral("DefaultSendWindow"), 65536, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AFD\\Parameters"),
                        QStringLiteral("FastSendDatagramThreshold"), 1024, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AFD\\Parameters"),
                        QStringLiteral("FastCopyReceiveThreshold"), 1024, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("net.qos-policy");
            t.category = QStringLiteral("networking");
            t.name = QStringLiteral("QoS policy: max autotuning + DSCP marking");
            t.description = QStringLiteral(
                "Allows apps to set DSCP, bypasses Network Location Awareness for QoS, and "
                "sets TCP autotuning to its most aggressive level.");
            t.iconName = QStringLiteral("network");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "risky: 'Experimental' TCP autotuning hurts some networks; DSCP marking only "
                "helps if your router honors it. Measure before keeping.");
            t.registryOps = {
                stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\QoS"),
                         QStringLiteral("Do not use NLA"), QStringLiteral("1"), std::nullopt),
                stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\QoS"),
                         QStringLiteral("Application DSCP Marking Request"), QStringLiteral("Allowed"), std::nullopt),
                stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\QoS"),
                         QStringLiteral("Tcp Autotuning Level"), QStringLiteral("Experimental"), std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("net.ctcp-congestion");
            t.category = QStringLiteral("networking");
            t.name = QStringLiteral("Compound TCP congestion control");
            t.description = QStringLiteral(
                "Switches the TCP congestion provider to Compound TCP (CTCP), which ramps "
                "the send rate up more aggressively on high-bandwidth links.");
            t.iconName = QStringLiteral("network");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "outdated-leaning: modern Windows defaults to CUBIC, which is generally as "
                "good or better; CTCP is unlikely to help on a typical connection.");
            CommandOp op;
            op.program = QStringLiteral("netsh");
            op.applyArgs  = { QStringLiteral("int"), QStringLiteral("tcp"), QStringLiteral("set"),
                              QStringLiteral("supplemental"), QStringLiteral("template=internet"),
                              QStringLiteral("congestionprovider=ctcp") };
            op.revertArgs = { QStringLiteral("int"), QStringLiteral("tcp"), QStringLiteral("set"),
                              QStringLiteral("supplemental"), QStringLiteral("template=internet"),
                              QStringLiteral("congestionprovider=default") };
            t.commandOps = { op };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("net.hit-registration-priority");
            t.category = QStringLiteral("networking");
            t.name = QStringLiteral("Prioritize Hosts/DNS name resolution");
            t.description = QStringLiteral(
                "Disables IP source routing and reorders the name-resolution provider "
                "priority (local, hosts file, DNS, NetBT) so cached/local lookups win, "
                "cutting resolution latency for competitive multiplayer hit registration.");
            t.iconName = QStringLiteral("network");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.registryOps = {
                dwordOp(kTcpip, QStringLiteral("DisableIPSourceRouting"), 2, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\ServiceProvider"),
                        QStringLiteral("LocalPriority"), 4, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\ServiceProvider"),
                        QStringLiteral("HostsPriority"), 5, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\ServiceProvider"),
                        QStringLiteral("DnsPriority"), 6, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\ServiceProvider"),
                        QStringLiteral("NetbtPriority"), 7, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("net.advanced-stack-tuning");
            t.category = QStringLiteral("networking");
            t.name = QStringLiteral("Advanced TCP/IP and QoS power settings");
            t.description = QStringLiteral(
                "Caps RSS CPU count, disables connection-rate limiting, widens the free-TCB "
                "and window-size ceilings, disables NBL-owner tracking and SMB IRP/buffer "
                "sizing, and turns off every QoS network-adapter power-saving feature "
                "(dynamic/connected power gating, Wake-on-LAN, GigaLite).");
            t.iconName = QStringLiteral("network");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "risky: disables adapter power management and Wake-on-LAN entirely; some "
                "values (MaxFreeTcbs, GlobalMaxTcpWindowSize) use non-standard hex literals "
                "from the source and should be treated as approximate.");
            t.registryOps = {
                dwordOp(kTcpip, QStringLiteral("MaxNumRssCpus"), 4, std::nullopt),
                dwordOp(kTcpip, QStringLiteral("EnableConnectionRateLimiting"), 0, std::nullopt),
                dwordOp(kTcpip, QStringLiteral("TCP1323Opts"), 1, std::nullopt),
                dwordOp(kTcpip, QStringLiteral("MaxFreeTcbs"), 0x00065536u, std::nullopt),
                dwordOp(kTcpip, QStringLiteral("GlobalMaxTcpWindowSize"), 0x00065535u, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NDIS\\Parameters"),
                        QStringLiteral("TrackNblOwner"), 0, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters"),
                        QStringLiteral("IRPStackSize"), 0x00000032u, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters"),
                        QStringLiteral("SizReqBuf"), 0x00017424u, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer"),
                        QStringLiteral("ThreadPriority"), 0, std::nullopt),
                stringOp(kQos, QStringLiteral("EnableDynamicPowerGating"), QStringLiteral("0"), std::nullopt),
                stringOp(kQos, QStringLiteral("EnableConnectedPowerGating"), QStringLiteral("0"), std::nullopt),
                stringOp(kQos, QStringLiteral("ULPMode"), QStringLiteral("0"), std::nullopt),
                stringOp(kQos, QStringLiteral("ReduceSpeedOnPowerDown"), QStringLiteral("0"), std::nullopt),
                stringOp(kQos, QStringLiteral("S5WakeOnLan"), QStringLiteral("0"), std::nullopt),
                stringOp(kQos, QStringLiteral("PowerSavingMode"), QStringLiteral("0"), std::nullopt),
                stringOp(kQos, QStringLiteral("GigaLite"), QStringLiteral("0"), std::nullopt),
                stringOp(kQos, QStringLiteral("WakeOnLinkChange"), QStringLiteral("0"), std::nullopt),
                stringOp(kQos, QStringLiteral("WolShutdownLinkSpeed"), QStringLiteral("2"), std::nullopt),
                stringOp(kQos, QStringLiteral("WakeOnLink"), QStringLiteral("0"), std::nullopt),
                dwordOp(kQos, QStringLiteral("PnPCapabilities"), 0x00000018u, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("net.throughput-and-ncsi-probes");
            t.category = QStringLiteral("networking");
            t.name = QStringLiteral("Throughput and connectivity-probe settings");
            t.description = QStringLiteral(
                "Relaxes QoS non-best-effort limits and gateway/MTU discovery, and points "
                "the NCSI active/passive connectivity probes at Google's DNS and connectivity "
                "endpoints with shorter timers.");
            t.iconName = QStringLiteral("network");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "redirects Windows' connectivity check (NCSI) to Google infrastructure "
                "instead of Microsoft's; consider whether that's an acceptable trust/privacy "
                "trade for your deployment before applying.");
            t.registryOps = {
                dwordOp(kQos, QStringLiteral("NonBestEffortLimit"), 0, std::nullopt),
                dwordOp(kQos, QStringLiteral("DeadGWDetectDefault"), 1, std::nullopt),
                dwordOp(kQos, QStringLiteral("PerformRouterDiscovery"), 1, std::nullopt),
                dwordOp(kQos, QStringLiteral("TcpInitialRTT"), 2, std::nullopt),
                dwordOp(kQos, QStringLiteral("MTU"), 0x000005dcu, std::nullopt),
                dwordOp(kQos, QStringLiteral("UseZeroBroadcast"), 0, std::nullopt),
                stringOp(kNcsiInt, QStringLiteral("ActiveDnsProbeContent"), QStringLiteral("8.8.4.4"), std::nullopt),
                stringOp(kNcsiInt, QStringLiteral("ActiveDnsProbeContentV6"), QStringLiteral("2001:4860:4860::8844"), std::nullopt),
                stringOp(kNcsiInt, QStringLiteral("ActiveDnsProbeHost"), QStringLiteral("dns.google"), std::nullopt),
                stringOp(kNcsiInt, QStringLiteral("ActiveDnsProbeHostV6"), QStringLiteral("dns.google"), std::nullopt),
                stringOp(kNcsiInt, QStringLiteral("ActiveWebProbeContent"), QStringLiteral("Microsoft Connect Test"), std::nullopt),
                stringOp(kNcsiInt, QStringLiteral("ActiveWebProbeContentV6"), QStringLiteral("Microsoft Connect Test"), std::nullopt),
                stringOp(kNcsiInt, QStringLiteral("ActiveWebProbeHost"), QStringLiteral("www.msftconnecttest.com"), std::nullopt),
                stringOp(kNcsiInt, QStringLiteral("ActiveWebProbePath"), QStringLiteral("connecttest.txt"), std::nullopt),
                stringOp(kNcsiInt, QStringLiteral("ActiveWebProbePathV6"), QStringLiteral("connecttest.txt"), std::nullopt),
                dwordOp(kNcsiInt, QStringLiteral("CaptivePortalTimer"), 0, std::nullopt),
                dwordOp(kNcsiInt, QStringLiteral("CaptivePortalTimerBackOffIncrementsInSeconds"), 5, std::nullopt),
                dwordOp(kNcsiInt, QStringLiteral("CaptivePortalTimerMaxInSeconds"), 30, std::nullopt),
                dwordOp(kNcsiInt, QStringLiteral("PassivePollPeriod"), 15, std::nullopt),
                dwordOp(kNcsiInt, QStringLiteral("StaleThreshold"), 30, std::nullopt),
                dwordOp(kNcsiInt, QStringLiteral("WebTimeout"), 35, std::nullopt),
            };
            c.push_back(t);
        }
    }

} // namespace vth::tweaks::detail
