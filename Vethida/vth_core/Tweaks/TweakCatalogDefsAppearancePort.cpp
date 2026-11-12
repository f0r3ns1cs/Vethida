#include "TweakEngine.h"

namespace vth::tweaks::detail {

void appendDefsAppearancePort(QVector<TweakSpec>& c) {
    {
        TweakSpec t;
        t.id = QStringLiteral("appearance.dwm.reduce_present_latency");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Desktop Window Manager");
        t.name = QStringLiteral("Reduce DWM present latency");
        t.description = QStringLiteral("Shifts the Desktop Window Manager's internal present queue (frame latency, DirectDraw sync, and queued present buffers) toward lower latency instead of maximum smoothness.");
        t.iconName = QStringLiteral("clock");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.reversible = true;
        t.warnings = { QStringLiteral("These are undocumented DWM values; effect can vary by GPU driver and Windows build.") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("FrameLatency"), 2u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("ForceDirectDrawSync"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("MaxQueuedPresentBuffers"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("appearance.dwm.boost_io_priority");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Desktop Window Manager");
        t.name = QStringLiteral("Boost DWM I/O priority");
        t.description = QStringLiteral("Raises the I/O priority of the Desktop Window Manager (dwm.exe) via Image File Execution Options, so compositor disk/GPU I/O is scheduled ahead of background processes.");
        t.iconName = QStringLiteral("layout");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.reversible = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\dwm.exe\\PerfOptions"), QStringLiteral("IoPriority"), 3u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("appearance.dwm.disable_advanced_direct_flip");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Desktop Window Manager");
        t.name = QStringLiteral("Disable advanced DirectFlip");
        t.description = QStringLiteral("Disables DWM's 'advanced' DirectFlip presentation path, an internal optimization some users report can cause flickering or stutter on certain GPU drivers.");
        t.iconName = QStringLiteral("layout");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.reversible = true;
        t.warnings = { QStringLiteral("This is an undocumented DWM value; effect can vary by GPU driver and Windows build.") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("DisableAdvancedDirectFlip"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("appearance.explorer.disable_thumbsdb_network");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Explorer");
        t.name = QStringLiteral("Disable Thumbs.db on network folders");
        t.description = QStringLiteral("Stops Explorer from writing hidden Thumbs.db thumbnail-cache files into folders on network shares.");
        t.iconName = QStringLiteral("photo-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Explorer"), QStringLiteral("DisableThumbsDBOnNetworkFolders"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("appearance.actioncenter.hide_pinned_quick_actions");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Notifications");
        t.name = QStringLiteral("Hide pinned quick actions in Action Center");
        t.description = QStringLiteral("Sets the pinned Quick Actions slot count to zero, hiding the row of quick-toggle tiles at the top of the Action Center.");
        t.iconName = QStringLiteral("toggle-left");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.concern = QStringLiteral("Targets the legacy Windows 10-style Action Center quick-actions grid; effect (if any) on Windows 11's separate Quick Settings panel is unverified and should be confirmed before shipping.");
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Shell\\ActionCenter\\Quick Actions"), QStringLiteral("PinnedQuickActionSlotCount"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("appearance.explorer.disable_address_autosuggest");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Explorer");
        t.name = QStringLiteral("Disable Explorer address bar autosuggest");
        t.description = QStringLiteral("Turns off autosuggest/autocomplete for the File Explorer and Run-dialog address bar.");
        t.iconName = QStringLiteral("edit-off");
        t.impact = Impact::Low;
        t.requiresAdmin = false;
        t.registryOps = {
            stringOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\AutoComplete"), QStringLiteral("AutoSuggest"), QStringLiteral("no"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("appearance.svc.themes_service_disable");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Visual style");
        t.name = QStringLiteral("Disable themes service");
        t.description = QStringLiteral("Disables the Themes service, which renders visual styles (Fluent/Aero theming, custom colors and .theme files). Windows falls back to the basic/classic visual style.");
        t.iconName = QStringLiteral("palette");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = { QStringLiteral("Windows reverts to the basic/classic visual style; custom themes, accent colors and some app UI chrome will look inconsistent.") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Themes"), QStringLiteral("Start"), 4u, 2u),
        };
        c.push_back(t);
    }
}

}
