#include "TweakEngine.h"

namespace vth::tweaks::detail {

void appendDefsShell(QVector<TweakSpec>& c) {
    {
        TweakSpec t;
        t.id = QStringLiteral("shell.classic-explorer-folders");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Explorer");
        t.name = QStringLiteral("Show full path in Explorer title bar");
        t.description = QStringLiteral("Shows the full folder path in the File Explorer title bar and taskbar tooltip instead of just the folder name (the registry equivalent of Folder Options > 'Display the full path in the title bar'). The address-bar breadcrumb is unaffected. Cosmetic preference only, no performance impact.");
        t.iconName = QStringLiteral("folder");
        t.impact = Impact::Low;
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CabinetState"), QStringLiteral("FullPath"), 1u, std::nullopt),
        };
        c.push_back(t);
    }


    {
        TweakSpec t;
        t.id = QStringLiteral("shell.disable-thumbnail-cache");
        t.category = QStringLiteral("general");
        t.name = QStringLiteral("Disable thumbnail cache on network shares");
        t.description = QStringLiteral("Tells Explorer not to write thumbcache_*.db files for items opened from network shares.");
        t.iconName = QStringLiteral("photo-off");
        t.impact = Impact::Low;
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("DisableThumbsDBOnNetworkFolders"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("shell.disable-thumbnail-network");
        t.category = QStringLiteral("general");
        t.name = QStringLiteral("Disable thumbnail generation on network shares");
        t.description = QStringLiteral("Stops Explorer from streaming entire files over the network just to render previews. Massive perceived speedup on slow links.");
        t.iconName = QStringLiteral("photo-off");
        t.impact = Impact::Low;
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("DisableThumbnailsOnNetworkFolders"), 1u, std::nullopt),
        };
        c.push_back(t);
    }
}

} // namespace vth::tweaks::detail
