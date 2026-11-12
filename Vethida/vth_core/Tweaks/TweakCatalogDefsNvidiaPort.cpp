#include "TweakEngine.h"

namespace vth::tweaks::detail {

void appendDefsNvidiaPort(QVector<TweakSpec>& c) {
    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.nvidia.disable_scheduler_preemption");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("GPU scheduling");
        t.name = QStringLiteral("Disable WDDM GPU scheduler preemption");
        t.description = QStringLiteral("Sets EnablePreemption=0 under the WDDM GraphicsDrivers scheduler key, stopping the OS GPU scheduler from preempting running GPU work to service other tasks.");
        t.iconName = QStringLiteral("cpu-off");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = { QStringLiteral("Any GPU workload (not just NVIDIA) can no longer be preempted mid-task, which risks full system hangs under heavy or mixed GPU workloads until the running task finishes") };
        t.concern = QStringLiteral("This is a global WDDM scheduler override, not NVIDIA-specific, despite living in the reference tool's NVIDIA section (function DisableNvPreemption). Disabling preemption entirely is a materially more aggressive change than the existing per-vendor RMEnablePreemption-style tweaks already in the catalog - needs curator review before shipping.");
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Scheduler"), QStringLiteral("EnablePreemption"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.nvidia.disable_cuda_context_preemption");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("NVIDIA");
        t.name = QStringLiteral("Disable NVIDIA CUDA context preemption");
        t.description = QStringLiteral("Sets DisablePreemption and DisableCudaContextPreemption on the NVIDIA display driver service, preventing the GPU from preempting compute/CUDA contexts mid-execution.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = { QStringLiteral("Long-running CUDA/compute workloads can no longer be interrupted by the GPU scheduler, which may cause brief display hangs or delayed input response under heavy GPU load") };
        t.concern = QStringLiteral("Disabling GPU preemption can hang the desktop under heavy compute load. In the reference tool this was applied inside the telemetry-disable routine (likely a copy/paste artifact), so it was never curated as a deliberate standalone tweak - needs review.");
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\nvlddmkm"), QStringLiteral("DisablePreemption"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\nvlddmkm"), QStringLiteral("DisableCudaContextPreemption"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.nvidia.disable_hdcp");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("NVIDIA");
        t.name = QStringLiteral("Disable HDCP (NVIDIA)");
        t.description = QStringLiteral("Sets RMHdcpKeyglobZero on the NVIDIA display driver to disable HDCP content protection on the video output.");
        t.iconName = QStringLiteral("shield-off");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Content requiring HDCP (Blu-ray players, some 4K/HD streaming) may refuse to play or drop to a lower resolution"),
            QStringLiteral("Only targets the primary NVIDIA adapter's registry subkey (0000); on hybrid/multi-GPU laptops the NVIDIA GPU may live under a different numeric subkey"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("RMHdcpKeyglobZero"), 1u, std::nullopt),
        };
        t.concern = QStringLiteral("Disabling HDCP is commonly used to bypass content-protection restrictions on protected video output (e.g. for capture cards); confirm this aligns with product policy before shipping.");
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.perf.latency_tolerance_extended");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("Latency");
        t.name = QStringLiteral("Extended graphics driver latency tolerance");
        t.description = QStringLiteral("Forces the remaining GraphicsDrivers\\Power latency-tolerance, D3-transition-latency, and memory-refresh-latency-tolerance values (idle buckets, monitor-off states, timer period, Miracast) to the minimum of 1, beyond the small subset already covered by the other GPU/power latency tweaks.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("GPU may not enter deep idle/low-power states; higher idle power draw and heat"),
        };
        t.concern = QStringLiteral("Writes 21 more values on the same GraphicsDrivers\\Power key already touched by 'GPU Low Latency Power Transitions', 'Disable GPU Energy Saving', and 'System Latency Tolerance' - review for consolidation before shipping as a 4th spec on this key.");
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Power"), QStringLiteral("DefaultD3TransitionLatencyIdleMonitorOff"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Power"), QStringLiteral("DefaultD3TransitionLatencyIdleNoContext"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Power"), QStringLiteral("DefaultD3TransitionLatencyIdleShortTime"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Power"), QStringLiteral("DefaultD3TransitionLatencyIdleVeryLongTime"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Power"), QStringLiteral("DefaultLatencyToleranceIdle0"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Power"), QStringLiteral("DefaultLatencyToleranceIdle0MonitorOff"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Power"), QStringLiteral("DefaultLatencyToleranceIdle1"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Power"), QStringLiteral("DefaultLatencyToleranceIdle1MonitorOff"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Power"), QStringLiteral("DefaultLatencyToleranceMemory"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Power"), QStringLiteral("DefaultLatencyToleranceNoContext"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Power"), QStringLiteral("DefaultLatencyToleranceNoContextMonitorOff"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Power"), QStringLiteral("DefaultLatencyToleranceOther"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Power"), QStringLiteral("DefaultLatencyToleranceTimerPeriod"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Power"), QStringLiteral("DefaultMemoryRefreshLatencyToleranceActivelyUsed"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Power"), QStringLiteral("DefaultMemoryRefreshLatencyToleranceMonitorOff"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Power"), QStringLiteral("DefaultMemoryRefreshLatencyToleranceNoContext"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Power"), QStringLiteral("Latency"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Power"), QStringLiteral("MaxIAverageGraphicsLatencyInOneBucket"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Power"), QStringLiteral("MiracastPerfTrackGraphicsLatency"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Power"), QStringLiteral("MonitorRefreshLatencyTolerance"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Power"), QStringLiteral("TransitionLatency"), 1u, std::nullopt),
        };
        c.push_back(t);
    }
}

}
