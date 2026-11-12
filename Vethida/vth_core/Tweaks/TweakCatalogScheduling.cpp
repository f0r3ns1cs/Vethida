#include "TweakEngine.h"

// PriorityControl (Win32PrioritySeparation lives in the starter set).
//   HKLM\SYSTEM\CurrentControlSet\Control\PriorityControl

namespace vth::tweaks::detail {

    void appendScheduling(QVector<TweakSpec>& c) {
        Q_UNUSED(c);

        {
            TweakSpec t;
            t.id = QStringLiteral("sched.heterogeneous-policy");
            t.category = QStringLiteral("advanced");
            t.name = QStringLiteral("Hybrid CPU P-core priority");
            t.description = QStringLiteral(
                "Sets the hybrid-CPU scheduling policy to favor performance (P) cores "
                "for high-QoS foreground threads and efficiency (E) cores for background.");
            t.iconName = QStringLiteral("cpu");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "no effect on AMD Ryzen or pre-12th-gen Intel; only meaningful on Intel "
                "hybrid (Alder/Raptor/Meteor Lake) CPUs.");
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PriorityControl"),
                        QStringLiteral("HeterogeneousPolicy"), 5, std::nullopt),
            };
            c.push_back(t);
        }
    }

}
