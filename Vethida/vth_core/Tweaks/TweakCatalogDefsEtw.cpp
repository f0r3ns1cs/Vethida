#include "TweakEngine.h"

namespace vth::tweaks::detail {

void appendDefsEtw(QVector<TweakSpec>& c) {
    Q_UNUSED(c);

    {
        TweakSpec t;
        t.id = QStringLiteral("etw.audiosrv-cpu-percent-max");
        t.category = QStringLiteral("advanced");
        t.name = QStringLiteral("Raise audio driver-graph CPU cap");
        t.description = QStringLiteral(
            "Lifts the processor limit Windows puts on audio processing, so effects "
            "and plugins are not cut short when a session gets heavy.");
        t.iconName = QStringLiteral("volume-2");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Audiosrv\\Parameters"), QStringLiteral("AudioDGCPUPercentMax"), 100u, std::nullopt),
        };
        c.push_back(t);
    }
}

} // namespace vth::tweaks::detail
