#include "TweakEngine.h"

namespace vth::tweaks::detail {

void appendDefsServices(QVector<TweakSpec>& c) {
    {
        TweakSpec t;
        t.id = QStringLiteral("svc.disable-browser");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable computer browser");
        t.description = QStringLiteral("Old SMB1-era 'computer browser' (Master Browser / Backup Browser) feature for the Network Neighborhood. Tied to NetBIOS / SMB1 broadcasting; disable along with SMBv1.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Browser"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("svc.disable-lltdsvc");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable link-layer topology discovery mapper");
        t.description = QStringLiteral("Old 'Network Map' feature in Windows that drew the topology of devices on a LAN. Removed from the UI in modern Windows. Service still ships.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\lltdsvc"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("svc.disable-uevagentservice");
        t.category = QStringLiteral("advanced");
        t.name = QStringLiteral("Disable user experience virtualization");
        t.description = QStringLiteral("Roaming-profile feature for app settings. Enterprise feature; never used on personal PCs.");
        t.iconName = QStringLiteral("settings-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\UevAgentService"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }
}

} // namespace vth::tweaks::detail
