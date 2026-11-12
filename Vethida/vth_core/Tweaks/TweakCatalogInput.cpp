#include "TweakEngine.h"

// Input device buffer sizes (mouclass / kbdclass) and pointer behavior.

namespace vth::tweaks::detail {

    void appendInput(QVector<TweakSpec>& c) {
        {
            TweakSpec t;
            t.id = QStringLiteral("input.disable-mouse-acceleration");
            t.category = QStringLiteral("general");
            t.subCategory = QStringLiteral("Mouse");
            t.name = QStringLiteral("Disable mouse acceleration");
            t.description = QStringLiteral("Disables Enhance Pointer Precision (mouse acceleration) so cursor movement maps linearly to physical mouse movement. Note this turns off OS acceleration; it does not itself enable raw input, which is a separate per-game setting.");
            t.iconName = QStringLiteral("mouse-pointer");
            t.impact = Impact::Low;
            t.requiresAdmin = false;
            t.registryOps = {
                stringOp(QStringLiteral("HKCU\\Control Panel\\Mouse"),
                         QStringLiteral("MouseSpeed"), QStringLiteral("0"), QStringLiteral("1")),
                stringOp(QStringLiteral("HKCU\\Control Panel\\Mouse"),
                         QStringLiteral("MouseThreshold1"), QStringLiteral("0"), QStringLiteral("6")),
                stringOp(QStringLiteral("HKCU\\Control Panel\\Mouse"),
                         QStringLiteral("MouseThreshold2"), QStringLiteral("0"), QStringLiteral("10")),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("input.touch-prediction");
            t.category = QStringLiteral("general");
            t.subCategory = QStringLiteral("Touch");
            t.name = QStringLiteral("Minimum touch latency");
            t.description = QStringLiteral("Lowers Windows touch-prediction look-ahead and sample time to 2 ms, which can make touchscreen input feel more responsive. Affects touchscreens only and has no effect on a mouse/keyboard desktop.");
            t.iconName = QStringLiteral("hand");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral("niche: only affects touchscreens; no effect on a mouse/keyboard desktop.");
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\TouchPrediction"),
                        QStringLiteral("Latency"), 2, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\TouchPrediction"),
                        QStringLiteral("SampleTime"), 2, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("input.mouse-queue-size");
            t.category = QStringLiteral("general");
            t.name = QStringLiteral("Reduce mouse buffer size");
            t.description = QStringLiteral(
                "Lowers the mouse class driver input queue from 100 to 16 entries.");
            t.iconName = QStringLiteral("mouse-pointer");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "marginal: shrinks a non-paged-pool buffer; no reproducible latency "
                "benefit, but harmless.");
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\mouclass\\Parameters"),
                        QStringLiteral("MouseDataQueueSize"), 16, 100),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("input.keyboard-queue-size");
            t.category = QStringLiteral("general");
            t.name = QStringLiteral("Reduce keyboard buffer size");
            t.description = QStringLiteral(
                "Lowers the keyboard class driver input queue from 100 to 16 entries.");
            t.iconName = QStringLiteral("keyboard");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "marginal: shrinks a non-paged-pool buffer; no reproducible latency "
                "benefit, but harmless.");
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\kbdclass\\Parameters"),
                        QStringLiteral("KeyboardDataQueueSize"), 16, 100),
            };
            c.push_back(t);
        }
    }

}
