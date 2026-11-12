#include "TweakEngine.h"

namespace vth::tweaks::detail {

void appendDefsBrowserOfficePort(QVector<TweakSpec>& c) {
    {
        TweakSpec t;
        t.id = QStringLiteral("browseroffice.edge.legacy_preload_disable");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("Edge");
        t.name = QStringLiteral("Disable legacy Edge prelaunch and tab preloading");
        t.description = QStringLiteral("Blocks the legacy EdgeHTML-based Microsoft Edge from prelaunching at sign-in and preloading tabs in the background, via its old policy keys.");
        t.iconName = QStringLiteral("brand-edge");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Targets the legacy EdgeHTML policy path (MicrosoftEdge\\Main, MicrosoftEdge\\TabPreloader); has no effect on the Chromium-based Edge shipped with current Windows 10/11"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\MicrosoftEdge\\Main"), QStringLiteral("AllowPrelaunch"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\MicrosoftEdge\\TabPreloader"), QStringLiteral("AllowTabPreloading"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("browseroffice.edge.update_and_shortcut_policy");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("Edge");
        t.name = QStringLiteral("Restrict Edge update shortcuts and legacy migration");
        t.description = QStringLiteral("Stops Edge from creating a desktop shortcut by default after updates, and blocks the old Windows 10 auto-migration path that once force-installed Chromium Edge over the legacy EdgeHTML browser.");
        t.iconName = QStringLiteral("brand-edge");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("DoNotUpdateToEdgeWithChromium only matters on older Windows 10 builds that still shipped EdgeHTML; it is a no-op on current Windows 10/11 installs"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\EdgeUpdate"), QStringLiteral("DoNotUpdateToEdgeWithChromium"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\EdgeUpdate"), QStringLiteral("CreateDesktopShortcutDefault"), 0u, std::nullopt),
        };
        c.push_back(t);
    }
}

}
