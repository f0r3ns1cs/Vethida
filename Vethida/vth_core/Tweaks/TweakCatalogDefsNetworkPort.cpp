#include "TweakEngine.h"

namespace vth::tweaks::detail {

void appendDefsNetworkPort(QVector<TweakSpec>& c) {
    {
        TweakSpec t;
        t.id = QStringLiteral("net.tcpip.name_resolution_priority_tuning");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("DNS");
        t.name = QStringLiteral("Name resolution provider priority");
        t.description = QStringLiteral("Lowers the Local/Hosts/DNS/NetBIOS provider priority values Windows consults when more than one resolver can answer the same name lookup.");
        t.iconName = QStringLiteral("list-numbers");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.reversible = true;
        t.concern = QStringLiteral("The stock values (499/500/2000/2001) already rank Local < Hosts < DNS < NetBT in the same relative order as the tweaked values (4/5/6/7), so this widely-copied tweak likely has no measurable effect on real resolution behavior - worth a call on whether it's worth shipping at all.");
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\ServiceProvider"), QStringLiteral("LocalPriority"), 4u, 499u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\ServiceProvider"), QStringLiteral("HostsPriority"), 5u, 500u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\ServiceProvider"), QStringLiteral("DnsPriority"), 6u, 2000u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\ServiceProvider"), QStringLiteral("NetbtPriority"), 7u, 2001u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.tcpip.disable_ip_source_routing");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("Hardening");
        t.name = QStringLiteral("Disable IP source routing");
        t.description = QStringLiteral("Blocks the TCP/IP stack from honoring IP source-routed packets, closing off a classic technique for spoofing traffic or bypassing network path filtering.");
        t.iconName = QStringLiteral("route-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.reversible = true;
        t.warnings = { QStringLiteral("Any legacy application that intentionally relies on IP source routing will stop working (extremely rare on modern networks)") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("DisableIPSourceRouting"), 2u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.msmq.tcp_nodelay");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("MSMQ");
        t.name = QStringLiteral("Disable Nagle for message queuing (MSMQ)");
        t.description = QStringLiteral("Sets TCPNoDelay=1 for the Message Queuing (MSMQ) service, disabling Nagle's algorithm on MSMQ's TCP connections for lower-latency delivery.");
        t.iconName = QStringLiteral("network");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.reversible = true;
        t.warnings = { QStringLiteral("Only takes effect if the Message Queuing (MSMQ) Windows feature is installed; MSMQ is not installed by default") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\MSMQ\\Parameters"), QStringLiteral("TCPNoDelay"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.tcp.legacy_stack_tuning");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("TCP/IP");
        t.name = QStringLiteral("Legacy TCP stack values");
        t.description = QStringLiteral("Sets legacy pre-Vista TCP/IP stack values (RSS CPU cap, free TCB pool size, max TCP receive window) carried over from Windows XP/Server 2003-era performance guides.");
        t.iconName = QStringLiteral("network");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.reversible = true;
        t.warnings = { QStringLiteral("These are legacy Windows XP/Server 2003-era TCP/IP parameters; the rewritten Vista+ network stack auto-manages RSS CPU count and TCP window scaling, so real-world effect on Windows 10/11 is likely minimal to none") };
        t.concern = QStringLiteral("Ported for fidelity to the legacy guide it originates from; the modern auto-tuning TCP/IP stack likely makes this inert on Windows 10/11 - a human curator should judge whether it is worth shipping. The source's own literal values (0x00065536, 0x00065535) look like an unintended hex-prefixed transcription of decimal 65536/65535 in the ORIGINAL reference tool, not an error introduced here - kept faithful to source rather than silently reinterpreted.");
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("MaxNumRssCpus"), 4u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("MaxFreeTcbs"), 0x00065536u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), QStringLiteral("GlobalMaxTcpWindowSize"), 0x00065535u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.qos.autotuning_policy");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("QoS");
        t.name = QStringLiteral("Force experimental QoS autotuning policy");
        t.description = QStringLiteral("Configures the Policy-based QoS group policy values for TCP receive-window autotuning level (Experimental) and application DSCP marking (Allowed) - the same settings exposed by the 'Configure TCP Autotuning' and 'Configure DSCP Marking Request' group policy items.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.reversible = true;
        t.warnings = { QStringLiteral("On a domain-joined or managed PC this may conflict with QoS policy settings pushed by Group Policy") };
        t.concern = QStringLiteral("Force-configures a Group Policy administrative template setting outside of Group Policy itself; could be silently overwritten by a domain GPO refresh or confuse admins inspecting policy state.");
        t.registryOps = {
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\QoS"), QStringLiteral("Tcp Autotuning Level"), QStringLiteral("Experimental"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\QoS"), QStringLiteral("Application DSCP Marking Request"), QStringLiteral("Allowed"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.smb.legacy_request_buffer_size");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("SMB");
        t.name = QStringLiteral("Legacy SMB request buffer size");
        t.description = QStringLiteral(
            "Enlarges the buffer Windows uses to serve files to other machines on "
            "the network. Only affects this PC when something is reading from it.");
        t.iconName = QStringLiteral("server");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.reversible = true;
        t.concern = QStringLiteral("SizReqBuf is a legacy SMB1-era buffer tunable; the modern SMB2/SMB3 server stack does not consult it, so this is likely a no-op on Windows 10/11 - a human curator should judge whether it is worth shipping.");
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters"), QStringLiteral("SizReqBuf"), 0x00017424u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("networking.svc.connectivity_helpers_disable");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("Connectivity helpers");
        t.name = QStringLiteral("Disable network connectivity helper services");
        t.description = QStringLiteral("Disables Network Connected Devices Auto-Setup, the Network Connection Broker (lets Store apps react to connectivity changes), and the Network Connectivity Assistant (captive-portal/no-internet detection).");
        t.iconName = QStringLiteral("wifi-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("UWP/Store apps lose network-connectivity-change notifications."),
            QStringLiteral("Captive-portal detection and the 'no internet access' diagnosis may stop working."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NcdAutoSetup"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NcbService"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NcaSvc"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }
}

}
