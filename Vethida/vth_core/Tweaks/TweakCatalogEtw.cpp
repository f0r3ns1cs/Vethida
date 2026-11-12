#include "TweakEngine.h"

// Each session is a registry key under
//   HKLM\SYSTEM\CurrentControlSet\Control\WMI\Autologger\<session>
// with Start=1 (enabled) / 0 (disabled). Disabling is a registry write.
//
// The DO-NOT-DISABLE set (NT Kernel Logger, Defender*Logger, EventLog-*,
// the CircularKernel loggers) is excluded - disabling those BSODs or breaks the
// event log / security.

namespace vth::tweaks::detail {

    namespace {
        RegistryOp etwOff(const QString& session) {
            return dwordOp(
                QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\%1").arg(session),
                QStringLiteral("Start"), 0, 1);
        }
    }

    void appendEtw(QVector<TweakSpec>& c) {
        {
            TweakSpec t;
            t.id = QStringLiteral("etw.telemetry-off");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Telemetry");
            t.name = QStringLiteral("Disable telemetry trace sessions");
            t.description = QStringLiteral(
                "Disables the boot ETW sessions that feed telemetry: Diagtrack listener, "
                "application-impact telemetry, software-quality metrics and lightweight "
                "network tracing.");
            t.iconName = QStringLiteral("eye-off");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "pair with the telemetry services tweak: the Diagtrack listener re-arms "
                "while the DiagTrack service runs.");
            t.registryOps = {
                etwOff(QStringLiteral("AutoLogger-Diagtrack-Listener")),
                etwOff(QStringLiteral("AITEventLog")),
                etwOff(QStringLiteral("SQMLogger")),
                etwOff(QStringLiteral("LwtNetLog")),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("etw.diagnostics-off");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Diagnostics & tracing");
            t.name = QStringLiteral("Disable diagnostic trace sessions");
            t.description = QStringLiteral("Sets these boot diagnostic ETW trace sessions to not start, reducing background diagnostic/telemetry logging. These are logging sessions, not services, so nothing user-facing stops working and there is no measurable performance gain; it mainly removes diagnostic traces that support staff could otherwise use. Includes ReadyBoot's boot-trace session (harmless to disable on SSDs; may slightly reduce boot prefetch accuracy on slower HDDs).");
            t.iconName = QStringLiteral("activity");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = { QStringLiteral("Removes diagnostic logs support staff might use to investigate issues.") };
            t.registryOps = {
                etwOff(QStringLiteral("DiagLog")),
                etwOff(QStringLiteral("WdiContextLog")),
                etwOff(QStringLiteral("ReadyBoot")),
                etwOff(QStringLiteral("CloudExperienceHostOOBE")),
                etwOff(QStringLiteral("WiFiSession")),
                etwOff(QStringLiteral("NetCore")),
                etwOff(QStringLiteral("RadioMgr")),
                etwOff(QStringLiteral("SpoolerLogger")),
                etwOff(QStringLiteral("UBPM")),
                etwOff(QStringLiteral("AppModel")),
                etwOff(QStringLiteral("FaceRecoTel")),
                etwOff(QStringLiteral("RdrLog")),
                etwOff(QStringLiteral("WFP-IPsec Diagnostics")),
                etwOff(QStringLiteral("Microsoft-Windows-Setup")),
                etwOff(QStringLiteral("SetupPlatform")),
                etwOff(QStringLiteral("DiagPlatformWMIProvider")),
            };
            c.push_back(t);
        }
    }
}
