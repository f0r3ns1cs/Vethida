#include "TweakEngine.h"

namespace vth::tweaks::detail {

    void appendStarter(QVector<TweakSpec>& c) {
        {
            TweakSpec t;
            t.id = QStringLiteral("perf.power-throttling-off");
            t.category = QStringLiteral("general");
            t.subCategory = QStringLiteral("Power throttling");
            t.name = QStringLiteral("Disable power throttling");
            t.description = QStringLiteral("Sets PowerThrottlingOff=1 to globally disable Windows Power Throttling, which normally parks idle background threads in low-power CPU states to save energy. Foreground apps are already exempt, so in practice this keeps background processes at full clocks at the cost of higher idle power and heat. Effect is most noticeable on laptops; on a plugged-in desktop it changes little.");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.warnings = { QStringLiteral("Increases idle power draw and temperature.") };
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerThrottling"),
                        QStringLiteral("PowerThrottlingOff"), 1, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("net.throttling-index-off");
            t.category = QStringLiteral("networking");
            t.subCategory = QStringLiteral("Network throttling");
            t.name = QStringLiteral("Relax network throttling");
            t.description = QStringLiteral(
                "Raises the multimedia network throttle from 10 to 20 inbound packets/ms, "
                "giving more headroom without removing the cap.");
            t.iconName = QStringLiteral("network");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            // The popular 0xFFFFFFFF "disable" value INCREASES NDIS DPC latency and
            // starves the MMCSS audio path, because the index doubles as an inbound,
            // audio-protective rate limit. 20 lifts the outbound budget while keeping
            // that protection; never set 0xFFFFFFFF.
            t.concern = QStringLiteral(
                "operator-corrected: uses 20, not the popular 0xFFFFFFFF which degrades audio "
                "DPC scheduling. Benefit is modest and NIC-dependent.");
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"
                                       "\\Multimedia\\SystemProfile"),
                        QStringLiteral("NetworkThrottlingIndex"), 20, 0xAu),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("mm.system-responsiveness");
            t.category = QStringLiteral("general");
            t.subCategory = QStringLiteral("Multimedia (MMCSS)");
            t.name = QStringLiteral("Multimedia system responsiveness");
            t.description = QStringLiteral(
                "Lowers the CPU share the multimedia scheduler reserves for background "
                "tasks from 20% to 10%.");
            t.iconName = QStringLiteral("gauge");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"
                                       "\\Multimedia\\SystemProfile"),
                        QStringLiteral("SystemResponsiveness"), 10, 20),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("usb.selective-suspend-off");
            t.category = QStringLiteral("power");
            t.subCategory = QStringLiteral("USB power");
            t.name = QStringLiteral("Disable USB selective suspend");
            t.description = QStringLiteral("Sets DisableSelectiveSuspend=1 as a global request that the USB hub driver stop suspending idle USB devices, reducing wake latency on input peripherals at the cost of slightly higher idle power. On modern Windows 10/11 this is only a hint: per-device settings and the active power plan often take precedence, so the effect is partial and controller-dependent.");
            t.iconName = QStringLiteral("usb");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.warnings = { QStringLiteral("Slightly higher idle power on USB peripherals.") };
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\USB"),
                        QStringLiteral("DisableSelectiveSuspend"), 1, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("perf.win32priorityseparation");
            t.category = QStringLiteral("advanced");
            t.name = QStringLiteral("Foreground quantum boost");
            t.description = QStringLiteral(
                "Gives the window you are working in a bigger share of the processor "
                "than the ones behind it, and switches between them more often.");
            t.iconName = QStringLiteral("cpu");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PriorityControl"),
                        QStringLiteral("Win32PrioritySeparation"), 0x26, 0x2),
            };
            c.push_back(t);
        }
    }

}
