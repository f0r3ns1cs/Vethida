#include "TweakEngine.h"

namespace vth::tweaks::detail {

    void appendPerformance(QVector<TweakSpec>& c) {
        {
            TweakSpec t;
            t.id = QStringLiteral("perf.game-dvr-off");
            t.category = QStringLiteral("general");
            t.subCategory = QStringLiteral("Game DVR");
            t.name = QStringLiteral("Disable Game DVR");
            t.description = QStringLiteral(
                "Turns off the Xbox Game DVR background recorder, which otherwise hooks every "
                "game's present path to capture replay clips.");
            t.iconName = QStringLiteral("video-off");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.registryOps = {
                dwordOp(QStringLiteral("HKCU\\System\\GameConfigStore"),
                        QStringLiteral("GameDVR_Enabled"), 0, 1),
                dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\GameDVR"),
                        QStringLiteral("AppCaptureEnabled"), 0, 1),
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\GameDVR"),
                        QStringLiteral("AllowGameDVR"), 0, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("perf.vbs-hvci-off");
            t.category = QStringLiteral("advanced");
            t.name = QStringLiteral("Disable VBS / Memory Integrity (HVCI)");
            t.description = QStringLiteral(
                "Turns off Virtualization-Based Security and Hypervisor-Enforced Code Integrity, "
                "removing the hypervisor's per-instruction overhead on CPU and GPU work.");
            t.iconName = QStringLiteral("shield-off");
            t.impact = Impact::High;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("VBS/HVCI is a kernel-memory protection boundary; disabling it "
                            "lowers exploit resistance. Reboot required."),
            };
            t.concern = QStringLiteral(
                "risky: real, measurable gaming/throughput gains on some systems, but at the "
                "cost of a hardware-backed security feature. Enterprise/managed devices may "
                "re-enable it by policy.");
            const QString dg = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\DeviceGuard");
            const QString sc = dg + QStringLiteral("\\Scenarios");
            t.registryOps = {
                dwordOp(dg, QStringLiteral("EnableVirtualizationBasedSecurity"), 0, 1),
                dwordOp(sc + QStringLiteral("\\HypervisorEnforcedCodeIntegrity"),
                        QStringLiteral("Enabled"), 0, 1),
            };
            c.push_back(t);
        }
    }
}
