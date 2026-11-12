#include "TweakEngine.h"

// Graphics drivers, DWM and DXGI, global keys only. Per-adapter NVIDIA values
// are handled by vth::win::nvidia::registryTweaksCatalog.

namespace vth::tweaks::detail {

    namespace {
        const QString kGd  = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers");
        const QString kDwm = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\Dwm");
        const QString kDxgi= QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX\\DXGI");
    }

    void appendGpu(QVector<TweakSpec>& c) {
        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.enable-hags");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("GPU scheduling");
            t.name = QStringLiteral("Hardware-accelerated GPU scheduling");
            t.description = QStringLiteral(
                "Enables HAGS, offloading GPU scheduling from the CPU to the GPU.");
            t.iconName = QStringLiteral("monitor");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "GPU-dependent: a win on RTX 40+/RX 7000+, but adds latency on RTX 20/30 "
                "and RX 5000/6000. Often already on by default on supported GPUs.");
            t.registryOps = { dwordOp(kGd, QStringLiteral("HwSchMode"), 2, 1) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.disable-mpo");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("Frame pacing");
            t.name = QStringLiteral("Disable multi-plane overlay (MPO)");
            t.description = QStringLiteral("Disables Multi-Plane Overlay (MPO), a common cause of multi-monitor and fullscreen flicker/stutter, forcing the classic composition path. Note: on Windows 11 24H2 and newer the OverlayTestMode=5 method is not always sufficient and may need an additional OverlayMinFPS=0 value, so it does not fully cover 24H2+ as-is. Only apply if you actually see stutter; it can slightly raise input latency on single-monitor competitive setups.");
            t.iconName = QStringLiteral("monitor");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            // Context-dependent, NOT universally good. Fixes multi-monitor
            // mismatched-refresh stutter, but on a single-monitor fullscreen game it
            // can ADD latency by dropping the present path off independent flip onto
            // Composed Flip.
            t.concern = QStringLiteral(
                "contested: fixes multi-monitor/fullscreen stutter, but on single-monitor "
                "competitive setups it can raise input latency (forces Composed Flip off "
                "independent flip). Only apply if you actually see stutter; reboot after.");
            t.registryOps = {
                dwordOp(kDwm, QStringLiteral("DisableMPO"), 1, std::nullopt),
                dwordOp(kDwm, QStringLiteral("OverlayTestMode"), 5, std::nullopt),
                dwordOp(kGd,  QStringLiteral("DisableOverlays"), 1, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.disable-tdr");
            t.category = QStringLiteral("gpu");
            t.name = QStringLiteral("Disable GPU timeout recovery (TDR)");
            t.description = QStringLiteral(
                "Disables Timeout Detection and Recovery so the GPU scheduler never "
                "resets a long-running task.");
            t.iconName = QStringLiteral("monitor");
            t.impact = Impact::High;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = { QStringLiteral("A real GPU hang will hard-lock the system instead of recovering.") };
            t.concern = QStringLiteral(
                "risky: removes the only soft-recovery path for a hung GPU; only sensible "
                "on a tuned machine where a hang means a crash anyway.");
            t.registryOps = { dwordOp(kGd, QStringLiteral("TdrLevel"), 0, std::nullopt) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.disable-preemption");
            t.category = QStringLiteral("gpu");
            t.name = QStringLiteral("Disable graphics preemption");
            t.description = QStringLiteral(
                "Stops Windows interrupting GPU work to render the desktop, improving "
                "in-game frame consistency.");
            t.iconName = QStringLiteral("monitor");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = { QStringLiteral("Desktop/DWM responsiveness drops under heavy GPU load.") };
            t.registryOps = { dwordOp(kGd, QStringLiteral("EnablePreemption"), 0, std::nullopt) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.flip-queue-min");
            t.category = QStringLiteral("gpu");
            t.name = QStringLiteral("Minimum flip queue");
            t.description = QStringLiteral(
                "Sets the GPU scheduler pre-rendered flip queue to its minimum for lower "
                "input lag.");
            t.iconName = QStringLiteral("monitor");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.registryOps = { dwordOp(kGd, QStringLiteral("FlipQueueSize"), 1, std::nullopt) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.dxgi-low-latency");
            t.category = QStringLiteral("gpu");
            t.name = QStringLiteral("DXGI low-latency swap chain");
            t.description = QStringLiteral(
                "Requests a minimum frame-latency, double-buffered DXGI swap chain.");
            t.iconName = QStringLiteral("monitor");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "placebo-leaning: swap-chain latency and buffering are chosen by each "
                "application; these global registry hints are largely ignored by modern "
                "DXGI. Harmless.");
            t.registryOps = {
                dwordOp(kDxgi, QStringLiteral("MaxFrameLatency"), 1, std::nullopt),
                dwordOp(kDxgi, QStringLiteral("BufferCount"), 2, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.nvidia-latency-registry");
            t.category = QStringLiteral("gpu");
            t.name = QStringLiteral("NVIDIA driver latency registry set");
            t.description = QStringLiteral(
                "Applies NVIDIA's low-latency/VRR driver flags (Reflex-adjacent GSP timing, "
                "PCI latency timer, idle thresholds) and disables runtime power management "
                "and protected-output negotiation on the GraphicsDrivers key.");
            t.iconName = QStringLiteral("monitor");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "NVIDIA-only; higher idle power. No-op without an NVIDIA GPU. Several names "
                "(RmGspcMaxFtuS, RMLpwrEiIdleThresholdUs, VRDirectFlipDPCDelayUsy, etc.) are "
                "undocumented internal driver values with no published measurement.");
            const QString gd = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers");
            QVector<RegistryOp> o;
            o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\NVIDIA Corporation\\Global\\NvCplApi\\Policies"),
                                QStringLiteral("QualityVsPerformance"), 0, std::nullopt));
            for (const QString& n : {
                QStringLiteral("D3PCLatency"), QStringLiteral("F1TransitionLatency"),
                QStringLiteral("LOWLATENCY"), QStringLiteral("Node3DLowLatency"),
                QStringLiteral("RMDeepL1EntryLatencyUsec"), QStringLiteral("RmGspcMaxFtuS"),
                QStringLiteral("RmGspcMinFtuS"), QStringLiteral("RmGspcPerioduS"),
                QStringLiteral("RMLpwrEiIdleThresholdUs"), QStringLiteral("RMLpwrGrIdleThresholdUs"),
                QStringLiteral("RMLpwrGrRgIdleThresholdUs"), QStringLiteral("RMLpwrMsIdleThresholdUs"),
                QStringLiteral("VRDirectFlipDPCDelayUsy"), QStringLiteral("VRDirectFlipTimingMarginUs"),
                QStringLiteral("VRDirectJITFlipMsHybridFlipDelayUs"), QStringLiteral("vrrCursorMarginUs"),
                QStringLiteral("vrrDeflickerMarginUs"), QStringLiteral("vrrDeflickerMaxUs"),
                QStringLiteral("RmFbsrPagedDMA"), QStringLiteral("UseGpuTimer"),
                QStringLiteral("RmCeUseGen4Mapping") })
                o.push_back(dwordOp(gd, n, 1, std::nullopt));
            o.push_back(dwordOp(gd, QStringLiteral("PciLatencyTimerControl"), 0x00000020u, std::nullopt));
            o.push_back(dwordOp(gd, QStringLiteral("RMOPSB"), 0x00002aa2u, std::nullopt));
            for (const QString& n : {
                QStringLiteral("RmClkPowerOffDramPllWhenUnused"),
                QStringLiteral("SupportRuntimePowerManagement"), QStringLiteral("RuntimePowerManagement"),
                QStringLiteral("Protection"), QStringLiteral("ProtectionLevel"),
                QStringLiteral("OPMSetProtectionLevel"), QStringLiteral("NumberOfIdleStates"),
                QStringLiteral("EnableRuntimePowerManagement"), QStringLiteral("DriverProtection"),
                QStringLiteral("PoFxStartDevicePowerManagement"), QStringLiteral("PoFxPowerControl"),
                QStringLiteral("DxgkWaitForIdle") })
                o.push_back(dwordOp(gd, n, 0, std::nullopt));
            t.registryOps = o;
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.dxgi-force-flip");
            t.category = QStringLiteral("gpu");
            t.name = QStringLiteral("Force flip-discard presentation");
            t.description = QStringLiteral(
                "Forces the DXGI flip model with tearing allowed, for lowest-latency "
                "fullscreen presentation and VRR without V-Sync.");
            t.iconName = QStringLiteral("monitor");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "placebo-leaning: presentation mode is chosen per-application; these global "
                "DXGI hints are largely ignored. AllowTearing only matters with G-Sync/FreeSync.");
            t.registryOps = {
                dwordOp(kDxgi, QStringLiteral("UseFlipModel"), 1, std::nullopt),
                dwordOp(kDxgi, QStringLiteral("ForceFlipDiscard"), 1, std::nullopt),
                dwordOp(kDxgi, QStringLiteral("AllowTearing"), 1, std::nullopt),
            };
            c.push_back(t);
        }
    }
}
