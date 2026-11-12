#include "TweakEngine.h"

namespace vth::tweaks::detail {

void appendDefsKernelPort(QVector<TweakSpec>& c) {
    {
        TweakSpec t;
        t.id = QStringLiteral("kernel.dpc.rate_control_tuning");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("DPC / Latency");
        t.name = QStringLiteral("Kernel DPC rate control");
        t.description = QStringLiteral("Overrides the kernel's DPC rate-control values (ideal/maximum/minimum DPC rate and per-processor DPC queue depth), which the scheduler otherwise auto-tunes based on measured DPC load.");
        t.iconName = QStringLiteral("clock");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("These are real kernel DPC-throttling controls, but forcing them all to their minimum settings removes the scheduler's own DPC-storm protection; the effect on real workloads is unverified.");
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("IdealDpcRate"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("MaximumDpcQueueDepth"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("MaximumDpcRate"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("MinimumDpcRate"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("DpcQueueDepth"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("kernel.io.workitem_queue_tuning");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("I/O Manager");
        t.name = QStringLiteral("I/O manager work-item queue thresholds");
        t.description = QStringLiteral("Overrides internal I/O manager deferred work-item queue thresholds (buffered I/O init, the ExQueueWorkItem/IoQueueWorkItem family, and IRP enqueue) to a fixed depth.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("These I/O-manager threshold names are not documented by Microsoft; the practical effect, if any, on real workloads is unverified.");
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("DisableBufferedIoInit"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("BufferSize"), 0x00000020u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("IoQueueWorkItem"), 0x00000020u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("IoQueueWorkItemToNode"), 0x00000020u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("IoQueueWorkItemEx"), 0x00000020u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("IoQueueThreadIrp"), 0x00000020u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("ExTryQueueWorkItem"), 0x00000020u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("ExQueueWorkItem"), 0x00000020u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("IoEnqueueIrp"), 0x00000020u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("kernel.cpu.dpc_interrupt_redirection");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Interrupts");
        t.name = QStringLiteral("Kernel DPC redirection and interrupt steering");
        t.description = QStringLiteral("Disables dynamic interrupt steering and the dynamic DPC redirection protocol at the kernel level, pinning interrupt/DPC handling instead of letting Windows rebalance it across cores at runtime.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("Not documented by Microsoft outside internal kernel debugging; may cause uneven interrupt/DPC distribution on multi-core systems, and the latency benefit is unverified.");
        t.warnings = { QStringLiteral("May cause uneven interrupt distribution on multi-core/hybrid CPUs") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("InterruptSteeringDisabled"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("DynamicDpcProtocol"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("kernel.reliability.disable_dpc_watchdog");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Reliability");
        t.name = QStringLiteral("Disable DPC/IO watchdog timeout detection");
        t.description = QStringLiteral("Disables the kernel's DPC watchdog, passive-level IO watchdog, and unexpected-interrupt bugcheck detection, which normally force a recoverable crash (bugcheck) when a driver hangs at high IRQL.");
        t.iconName = QStringLiteral("alert-triangle");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("This disables a real kernel reliability subsystem, not a placebo value: a driver stuck in an infinite loop or hang will freeze the whole system indefinitely instead of triggering a bugcheck that lets Windows recover or at least identify the faulty driver. Only appropriate for advanced users working around a specific watchdog false-positive.");
        t.warnings = { QStringLiteral("A genuinely hung driver will freeze the system instead of producing a recoverable bugcheck") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("DpcTimeout"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("DpcWatchdogPeriod"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("DpcWatchdogProfileBufferSizeBytes"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("DpcWatchdogProfileOffset"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("BugCheckUnexpectedInterrupts"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("WatchdogResumeTimeout"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("PassiveWatchdogTimeout"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("kernel.power.dynamic_tick_idle_tuning");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Idle / Dynamic Tick");
        t.name = QStringLiteral("Dynamic tick and idle timers");
        t.description = QStringLiteral("Overrides the tickless-kernel dynamic-tick duration limit and zeroes several idle-scan and QoS-timer grace periods, aiming for more frequent kernel tick wake-ups while idle.");
        t.iconName = QStringLiteral("clock");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("MaxDynamicTickDurationSize=0xFFFFFFFF and the zeroed idle/grace-period values are undocumented; this likely increases idle power draw (the opposite of what dynamic tick exists to save) with an unverified latency upside.");
        t.warnings = { QStringLiteral("May increase idle power consumption, especially on laptops") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("MaxDynamicTickDuration"), 0x0000000au, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("MaxDynamicTickDurationSize"), 0xffffffffu, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("ForceIdleGracePeriod"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("IdleScanInterval"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("IdleDurationExpirationTimeout"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("CoalescingTimerInterval"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("DisableLowQosTimerResolution"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("scheduler.priority_control_boost_tuning");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("CPU scheduling");
        t.name = QStringLiteral("Foreground boost and DPC scheduling (PriorityControl)");
        t.description = QStringLiteral("Writes a bundle of undocumented scheduler and DPC-coalescing values under Control\\PriorityControl: ForegroundBoost, ThreadBoostType, ThreadSchedulingModel, AdjustDpcThreshold, DeepIoCoalescingEnabled, IdealDpcRate, and SchedulerAssistThreadFlagOverride.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("None of these value names are part of Microsoft's documented foreground-boost mechanism, which lives entirely in the single Win32PrioritySeparation DWORD at this same key; treat as unverified and likely placebo.");
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PriorityControl"), QStringLiteral("ForegroundBoost"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PriorityControl"), QStringLiteral("ThreadBoostType"), 2u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PriorityControl"), QStringLiteral("ThreadSchedulingModel"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PriorityControl"), QStringLiteral("AdjustDpcThreshold"), 0x00000320u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PriorityControl"), QStringLiteral("DeepIoCoalescingEnabled"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PriorityControl"), QStringLiteral("IdealDpcRate"), 0x00000320u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PriorityControl"), QStringLiteral("SchedulerAssistThreadFlagOverride"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("scheduler.irq_priority_tuning");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Interrupts");
        t.name = QStringLiteral("IRQ arbitration priorities");
        t.description = QStringLiteral("Sets Plug-and-Play IRQ arbitration priorities under PriorityControl, including the documented IRQ8Priority value plus zero-padded IRQ00/08/16/23Priority names.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("Windows' documented IRQnPriority mechanism (Microsoft KB314868) uses names without leading zeros, e.g. IRQ8Priority. The zero-padded variants written here (IRQ00Priority, IRQ08Priority, IRQ16Priority, IRQ23Priority) do not match that documented format and are likely inert no-ops that Windows never reads; only IRQ8Priority is believed to have a real effect.");
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PriorityControl"), QStringLiteral("IRQ8Priority"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PriorityControl"), QStringLiteral("IRQ00Priority"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PriorityControl"), QStringLiteral("IRQ08Priority"), 4u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PriorityControl"), QStringLiteral("IRQ16Priority"), 2u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PriorityControl"), QStringLiteral("IRQ23Priority"), 3u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("kernel.scheduler.misc_dpc_tuning");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("CPU scheduling");
        t.name = QStringLiteral("Miscellaneous kernel scheduler and DPC values");
        t.description = QStringLiteral("Writes a bundle of undocumented scheduler/DPC values under the kernel Session Manager key: timer serialization, a DPC soft timeout, foreground-boost decay, a shared ready-queue size, and thread priority/quantum-shaped values that reuse names used elsewhere in Windows for unrelated, real settings.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("None of these value names have public Microsoft documentation at this registry path; PriorityControl/TimeIncrement/QuantumLength here are plain DWORD names under Session Manager\\kernel, not the real, documented Win32PrioritySeparation mechanism (which lives under a different key, Control\\PriorityControl). Treat as low-confidence placebo, similar to other undocumented values already flagged elsewhere in this catalog.");
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("SerializeTimerExpiration"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("DpcCumulativeSoftTimeout"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("ThreadDpcEnable"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("UnlimitDpcQueue"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("ForceForegroundBoostDecay"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("PriorityControl"), 0x00000032u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("TimeIncrement"), 0x0000000fu, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("QuantumLength"), 0x00000014u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("MaximumSharedReadyQueueSize"), 0x00000080u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("kernel.io.misc_worker_stack_tuning");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("I/O Manager");
        t.name = QStringLiteral("Miscellaneous kernel worker and stack sizes");
        t.description = QStringLiteral("Writes a bundle of undocumented cache-splitting, SIMD-zeroing, worker-thread-idle, and stack-size values under the kernel Session Manager key.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("No public Microsoft reference for these value names at this path; StackSubSystemStackSize and the cache/SIMD flags in particular are unverified and could affect stability for some third-party drivers.");
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("DelayCloseSize"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("DelayDerefKCBLimit"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("WorkerFactoryThreadIdleTimeout"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("SplitLargeCaches"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("XMMIZeroingEnable"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("DisableOverlappedExecution"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("UseNormalStack"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("UseNewEaBuffering"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("StackSubSystemStackSize"), 0x00010000u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("advanced.memory.legacy_large_system_cache");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Memory");
        t.name = QStringLiteral("Enable Large System Cache");
        t.description = QStringLiteral("Switches the memory manager's working-set preference to favor the file system cache over running programs - a legacy 'optimize for file sharing' setting inherited from the Windows Server world.");
        t.iconName = QStringLiteral("database");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.reversible = true;
        t.warnings = { QStringLiteral("Has little to no measurable effect on modern client editions of Windows; included for completeness.") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("LargeSystemCache"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("adv.memory.second_level_data_cache_hint");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Memory management");
        t.name = QStringLiteral("Set legacy L2 cache size hint");
        t.description = QStringLiteral(
            "Tells Windows how much second-level processor cache to assume when it "
            "lays out memory. Windows reads the real figure from the processor on "
            "anything modern, so this changes nothing there.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("Windows has auto-detected real L2 cache size since Vista; this pre-Vista hint almost certainly has no measurable effect on any currently supported release. Kept for reference-parity only - recommend not shipping as an 'optimization'.");
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("SecondLevelDataCache"), 256u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("advanced.session_env_latency_placebo_vars");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Environment Variables");
        t.name = QStringLiteral("Set system-wide low-latency environment variables");
        t.description = QStringLiteral("Writes around 57 custom-named system environment variables (e.g. CPU_MAX_PENDING_INTERRUPTS, IO_REQUEST_LIMIT, GPU_MAX_PENDING_INTERRUPTS, POWER_THROTTLE_POLICY) under the session manager's global environment block. The reference tool intends these to signal reduced interrupt/IO pending limits, disabled power throttling, and larger network/audio buffers system-wide.");
        t.iconName = QStringLiteral("gauge");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.reversible = true;
        t.warnings = {
            QStringLiteral("These variable names are not documented Windows, driver, or CPU-vendor environment variables recognized by any known Microsoft component"),
            QStringLiteral("No verified mechanism by which Windows, storage/network/GPU drivers, or the scheduler read these names"),
            QStringLiteral("Environment variables only propagate to new processes after a fresh logon or reboot"),
        };
        t.concern = QStringLiteral("None of these roughly 57 names (CPU_MAX_PENDING_INTERRUPTS, MKL_DEBUG_CPU_TYPE, DISK_MAX_PENDING_IO, GPU_MAX_PENDING_RENDER, etc.) appear in Microsoft's documented environment-variable or registry-tunable references, and none correspond to a known driver or kernel interrupt/IO/power-throttling control. This strongly resembles a placebo/pseudo-tuning tweak rather than a real performance lever. Recommend the curator independently verify any of these before shipping, or drop the tweak entirely rather than present it to users as a proven latency reduction.");
        const QString envKey = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment");
        t.registryOps = {
            stringOp(envKey, QStringLiteral("CPU_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("CPU_MAX_PENDING_IO"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("CPU_IDLE_POLICY"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("CPU_BOOST_POLICY"), QStringLiteral("2"), std::nullopt),
            stringOp(envKey, QStringLiteral("CPU_MAX_FREQUENCY"), QStringLiteral("100"), std::nullopt),
            stringOp(envKey, QStringLiteral("CPU_INTERRUPT_BALANCE_POLICY"), QStringLiteral("1"), std::nullopt),
            stringOp(envKey, QStringLiteral("MKL_DEBUG_CPU_TYPE"), QStringLiteral("10"), std::nullopt),
            stringOp(envKey, QStringLiteral("IO_COMPLETION_POLICY"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("IO_REQUEST_LIMIT"), QStringLiteral("1024"), std::nullopt),
            stringOp(envKey, QStringLiteral("DISK_MAX_PENDING_IO"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("IO_PRIORITY"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("DISK_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("IO_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("POWER_THROTTLE_POLICY"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("POWER_IDLE_TIMEOUT"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("CPU_POWER_POLICY"), QStringLiteral("1"), std::nullopt),
            stringOp(envKey, QStringLiteral("DISABLE_DYNAMIC_TICK"), QStringLiteral("yes"), std::nullopt),
            stringOp(envKey, QStringLiteral("MEMORY_MAX_ALLOCATION"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("MEMORY_LATENCY_POLICY"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("MEMORY_PREFETCH_POLICY"), QStringLiteral("2"), std::nullopt),
            stringOp(envKey, QStringLiteral("MEMORY_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("DWM_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("DWM_COMPOSITOR_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("NETWORK_BUFFER_SIZE"), QStringLiteral("512"), std::nullopt),
            stringOp(envKey, QStringLiteral("NETWORK_INTERRUPT_COALESCING"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("NETWORK_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("TIMER_RESOLUTION"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("THREAD_SCHEDULER_POLICY"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("GPU_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("NETWORK_ADAPTER_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("NETWORK_ADAPTER_MAX_PENDING_IO"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("NETWORK_ADAPTER_INTERRUPT_MODERATION"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("NETWORK_ADAPTER_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("STORAGE_DEVICE_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("STORAGE_DEVICE_MAX_PENDING_IO"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("STORAGE_DEVICE_COMPLETION_POLICY"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("STORAGE_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("STORAGE_DEVICE_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("USB_DEVICE_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("USB_DEVICE_MAX_PENDING_IO"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("USB_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("USB_DEVICE_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("PCIE_DEVICE_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("PCIE_DEVICE_MAX_PENDING_IO"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("PCIE_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("PCIE_DEVICE_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("GPU_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("GPU_MAX_PENDING_COMPUTE"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("GPU_MAX_PENDING_RENDER"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("AUDIO_DEVICE_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("AUDIO_DEVICE_BUFFER_SIZE"), QStringLiteral("512"), std::nullopt),
            stringOp(envKey, QStringLiteral("AUDIO_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("AUDIO_DEVICE_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("DEVICE_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("DEVICE_MAX_PENDING_IO"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("DEVICE_COMPLETION_POLICY"), QStringLiteral("0"), std::nullopt),
            stringOp(envKey, QStringLiteral("DEVICE_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("advanced.driver_class_thread_priority");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Device Drivers");
        t.name = QStringLiteral("Boost device driver thread priority");
        t.description = QStringLiteral("Sets an elevated BasePriority and OverTargetPriority value on the registry class keys for around 100 built-in Windows device-driver categories (display, keyboard, mouse, USB, network, storage, sound, ports, HID, and more), which some driver stacks may read to prioritize their kernel worker threads.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.reversible = true;
        t.warnings = {
            QStringLiteral("Effect is undocumented and driver-specific - most drivers never read these values"),
            QStringLiteral("Touches around 100 registry device-class keys system-wide"),
        };
        t.concern = QStringLiteral("This is a long-circulated 'gaming' registry hack that writes BasePriority/OverTargetPriority under every built-in device-class GUID key. These value names are not part of the documented Windows Driver Model class-registration schema, most drivers never read them, and the real-world performance effect could not be verified - this may be a placebo tweak. A curator should decide whether to ship it, and if so mark it clearly as experimental/unverified.");

        static const QStringList kDriverClassKeySuffixes = {
            QStringLiteral("Video\\{33123269-1807-11EF-B26D-806E6F6E6963}\\0003"),
            QStringLiteral("Video\\{33123269-1807-11EF-B26D-806E6F6E6963}\\0002"),
            QStringLiteral("Video\\{33123269-1807-11EF-B26D-806E6F6E6963}\\0001"),
            QStringLiteral("Video\\{33123269-1807-11EF-B26D-806E6F6E6963}\\0000"),
            QStringLiteral("Class\\{fe8f1572-c67a-48c0-bbac-0b5c6d66cafb}"),
            QStringLiteral("Class\\{f8ecafa6-66d1-41a5-899b-66585d7216b7}"),
            QStringLiteral("Class\\{f75a86c0-10d8-4c3a-b233-ed60e4cdfaac}"),
            QStringLiteral("Class\\{f3586baf-b5aa-49b5-8d6c-0569284c639f}"),
            QStringLiteral("Class\\{f2e7dd72-6468-4e36-b6f1-6488f42c1b52}"),
            QStringLiteral("Class\\{f048e777-b971-404b-bd9c-3802613495c2}"),
            QStringLiteral("Class\\{f01a9d53-3ff6-48d2-9f97-c8a7004be10c}"),
            QStringLiteral("Class\\{eec5ad98-8080-425f-922a-dabf3de3f69a}"),
            QStringLiteral("Class\\{e6f1aa1c-7f3b-4473-b2e8-c97d8ac71d53}"),
            QStringLiteral("Class\\{e55fa6f9-128c-4d04-abab-630c74b1453a}"),
            QStringLiteral("Class\\{e2f84ce7-8efa-411c-aa69-97454ca4cb57}"),
            QStringLiteral("Class\\{e0cbf06c-cd8b-4647-bb8a-263b43f0f974}"),
            QStringLiteral("Class\\{d94ee5d8-d189-4994-83d2-f68d7d41b0e6}"),
            QStringLiteral("Class\\{d546500a-2aeb-45f6-9482-f4b1799c3177}"),
            QStringLiteral("Class\\{d48179be-ec20-11d1-b6b8-00c04fa372a7}"),
            QStringLiteral("Class\\{d421b08e-6d16-41ca-9c4d-9147e5ac98e0}"),
            QStringLiteral("Class\\{d02bc3da-0c8e-4945-9bd5-f1883c226c8c}"),
            QStringLiteral("Class\\{ce5939ae-ebde-11d0-b181-0000f8753ec4}"),
            QStringLiteral("Class\\{cdcf0939-b75b-4630-bf76-80f7ba655884}"),
            QStringLiteral("Class\\{cc342e67-bd5b-4dd2-bb7b-bf23cf9f2a0e}"),
            QStringLiteral("Class\\{ca3e7ab9-b4c3-4ae6-8251-579ef933890f}"),
            QStringLiteral("Class\\{c30ecea0-11ef-4ef9-b02e-6af81e6e65c0}"),
            QStringLiteral("Class\\{c166523c-fe0c-4a94-a586-f1a80cfbbf3e}"),
            QStringLiteral("Class\\{c06ff265-ae09-48f0-812c-16753d7cba83}"),
            QStringLiteral("Class\\{bbbe8734-08fa-4966-b6a6-4e5ad010cdd7}"),
            QStringLiteral("Class\\{b86dff51-a31e-4bac-b3cf-e8cfe75c9fc2}"),
            QStringLiteral("Class\\{b2728d24-ac56-42db-9e02-8edaf5db652f}"),
            QStringLiteral("Class\\{b1d1a169-c54f-4379-81db-bee7d88d7454}"),
            QStringLiteral("Class\\{a73c93f1-9727-4d1d-ace1-0e333ba4e7db}"),
            QStringLiteral("Class\\{a3e32dba-ba89-4f17-8386-2d0127fbd4cc}"),
            QStringLiteral("Class\\{a0a701c0-a511-42ff-aa6c-06dc0395576f}"),
            QStringLiteral("Class\\{a0a588a4-c46f-4b37-b7ea-c82fe89870c6}"),
            QStringLiteral("Class\\{9da2b80f-f89f-4a49-a5c2-511b085b9e8a}"),
            QStringLiteral("Class\\{8ecc055d-047f-11d1-a537-0000f8753ed1}"),
            QStringLiteral("Class\\{89786ff1-9c12-402f-9c9e-17753c7f4375}"),
            QStringLiteral("Class\\{88bae032-5a81-49f0-bc3d-a4ff138216d6}"),
            QStringLiteral("Class\\{88a1c342-4539-11d3-b88d-00c04fad5171}"),
            QStringLiteral("Class\\{87ef9ad1-8f70-49ee-b215-ab1fcadcbe3c}"),
            QStringLiteral("Class\\{8503c911-a6c7-4919-8f79-5028f5866b0c}"),
            QStringLiteral("Class\\{7ebefbc0-3200-11d2-b4c2-00a0c9697d07}"),
            QStringLiteral("Class\\{745a17a0-74d3-11d0-b6fe-00a0c90f57da}"),
            QStringLiteral("Class\\{72631e54-78a4-11d0-bcf7-00aa00b7b32a}"),
            QStringLiteral("Class\\{71aa14f8-6fad-4622-ad77-92bb9d7e6947}"),
            QStringLiteral("Class\\{71a27cdd-812a-11d0-bec7-08002be2092f}"),
            QStringLiteral("Class\\{6fae73b7-b735-4b50-a0da-0dc2484b1f1a}"),
            QStringLiteral("Class\\{6d807884-7d21-11cf-801c-08002be10318}"),
            QStringLiteral("Class\\{6bdd1fc6-810f-11d0-bec7-08002be2092f}"),
            QStringLiteral("Class\\{6bdd1fc5-810f-11d0-bec7-08002be2092f}"),
            QStringLiteral("Class\\{6bdd1fc1-810f-11d0-bec7-08002be2092f}"),
            QStringLiteral("Class\\{6a0a8e78-bba6-4fc4-a709-1e33cd09d67e}"),
            QStringLiteral("Class\\{62f9c741-b25a-46ce-b54c-9bccce08b6f2}"),
            QStringLiteral("Class\\{5d1b9aaa-01e2-46af-849f-272b3f324c46}"),
            QStringLiteral("Class\\{5c4c3332-344d-483c-8739-259e934c9cc8}"),
            QStringLiteral("Class\\{5989fce8-9cd0-467d-8a6a-5419e31529d4}"),
            QStringLiteral("Class\\{5630831c-06c9-4856-b327-f5d32586e060}"),
            QStringLiteral("Class\\{53d29ef7-377c-4d14-864b-eb3a85769359}"),
            QStringLiteral("Class\\{53ccb149-e543-4c84-b6e0-bce4f6b7e806}"),
            QStringLiteral("Class\\{53b3cf03-8f5a-4788-91b6-d19ed9fcccbf}"),
            QStringLiteral("Class\\{53966cb1-4d46-4166-bf23-c522403cd495}"),
            QStringLiteral("Class\\{53487c23-680f-4585-acc3-1f10d6777e82}"),
            QStringLiteral("Class\\{533c5b84-ec70-11d2-9505-00c04f79deaf}"),
            QStringLiteral("Class\\{5175d334-c371-4806-b3ba-71fd53c9258d}"),
            QStringLiteral("Class\\{5099944a-f6b9-4057-a056-8c550228544c}"),
            QStringLiteral("Class\\{50906cb8-ba12-11d1-bf5d-0000f805f530}"),
            QStringLiteral("Class\\{4d36e97d-e325-11ce-bfc1-08002be10318}"),
            QStringLiteral("Class\\{4d36e97b-e325-11ce-bfc1-08002be10318}"),
            QStringLiteral("Class\\{4d36e978-e325-11ce-bfc1-08002be10318}"),
            QStringLiteral("Class\\{4d36e977-e325-11ce-bfc1-08002be10318}"),
            QStringLiteral("Class\\{4d36e975-e325-11ce-bfc1-08002be10318}"),
            QStringLiteral("Class\\{4d36e974-e325-11ce-bfc1-08002be10318}"),
            QStringLiteral("Class\\{4d36e973-e325-11ce-bfc1-08002be10318}"),
            QStringLiteral("Class\\{4d36e972-e325-11ce-bfc1-08002be10318}"),
            QStringLiteral("Class\\{4d36e971-e325-11ce-bfc1-08002be10318}"),
            QStringLiteral("Class\\{4d36e970-e325-11ce-bfc1-08002be10318}"),
            QStringLiteral("Class\\{4d36e96f-e325-11ce-bfc1-08002be10318}"),
            QStringLiteral("Class\\{4d36e96e-e325-11ce-bfc1-08002be10318}"),
            QStringLiteral("Class\\{4d36e96d-e325-11ce-bfc1-08002be10318}"),
            QStringLiteral("Class\\{4d36e96c-e325-11ce-bfc1-08002be10318}"),
            QStringLiteral("Class\\{4d36e96b-e325-11ce-bfc1-08002be10318}"),
            QStringLiteral("Class\\{4d36e96a-e325-11ce-bfc1-08002be10318}"),
            QStringLiteral("Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0001"),
            QStringLiteral("Class\\{4d36e968-e325-11ce-bfc1-08002be10318}"),
            QStringLiteral("Class\\{4d36e967-e325-11ce-bfc1-08002be10318}"),
            QStringLiteral("Class\\{4d36e966-e325-11ce-bfc1-08002be10318}"),
            QStringLiteral("Class\\{4d36e965-e325-11ce-bfc1-08002be10318}"),
            QStringLiteral("Class\\{49ce6ac8-6f86-11d2-b1e5-0080c72e74a2}"),
            QStringLiteral("Class\\{48d3ebc4-4cf8-48ff-b869-9c68ad42eb9f}"),
            QStringLiteral("Class\\{48721b56-6795-11d2-b1a8-0080c72e74a2}"),
            QStringLiteral("Class\\{4658ee7e-f050-11d1-b6bd-00c04fa372a7}"),
            QStringLiteral("Class\\{43675d81-502a-4a82-9f84-b75f418c5dea}"),
            QStringLiteral("Class\\{3e3f0674-c83c-4558-bb26-9820e1eba5c5}"),
            QStringLiteral("Class\\{36fc9e60-c465-11cf-8056-444553540000}"),
            QStringLiteral("Class\\{2db15374-706e-4131-a0c7-d7c78eb0289a}"),
            QStringLiteral("Class\\{268c95a1-edfe-11d3-95c3-0010dc4050a5}"),
            QStringLiteral("Class\\{25dbce51-6c8f-4a72-8a6d-b54c2b4fc835}"),
            QStringLiteral("Class\\{14b62f50-3f15-11dd-ae16-0800200c9a66}"),
            QStringLiteral("Class\\{1264760f-a5c8-4bfe-b314-d56a7b44a362}"),
        };

        QVector<RegistryOp> ops;
        for (const QString& suffix : kDriverClassKeySuffixes) {
            const QString fullPath = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\") + suffix;
            ops.push_back(dwordOp(fullPath, QStringLiteral("BasePriority"), 200u, std::nullopt));
            ops.push_back(dwordOp(fullPath, QStringLiteral("OverTargetPriority"), 80u, std::nullopt));
        }
        t.registryOps = ops;
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("advanced.mouse_data_queue_size_reduce");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Input Latency");
        t.name = QStringLiteral("Reduce mouse data queue size");
        t.description = QStringLiteral("Reduces the mouse class driver's internal input-report queue depth to 20 entries, down from the driver's built-in default, which some users report reduces perceived mouse input latency at the cost of a smaller buffer for input bursts.");
        t.iconName = QStringLiteral("mouse");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.reversible = true;
        t.warnings = {
            QStringLiteral("A very small queue size can drop mouse input reports during brief system stalls"),
            QStringLiteral("Latency benefit is anecdotal and has not been independently benchmarked"),
        };
        t.concern = QStringLiteral("Reducing this below the driver's built-in default is a popular but unverified input-latency tweak; benefit is anecdotal, not independently benchmarked.");
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\mouclass\\Parameters"), QStringLiteral("MouseDataQueueSize"), 20u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("advanced.svc.sens_disable");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable system event notification service");
        t.description = QStringLiteral("Disables SENS, which tracks system events (logon/logoff, network and power state changes) and notifies COM+ Event System subscribers.");
        t.iconName = QStringLiteral("bell-off");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("SENS is Automatic on every stock Windows install and several built-in and third-party apps silently depend on its logon/network/power-state notifications; disabling it can cause subtle misbehavior in unrelated apps.");
        t.warnings = { QStringLiteral("Apps that listen for network or power state changes via COM+ Events may stop receiving notifications.") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\SENS"), QStringLiteral("Start"), 4u, 2u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("advanced.svc.enterprise_mgmt_disable");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Enterprise & domain management");
        t.name = QStringLiteral("Disable Enterprise/Domain management services");
        t.description = QStringLiteral("Disables Certificate Propagation (smart-card certificate sync), the Device Management Enrollment Service, the Enterprise App Management Service, and Netlogon (domain authentication pass-through). All four are only active on domain-joined or MDM-enrolled machines.");
        t.iconName = QStringLiteral("building-bank");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("Do not apply on a domain-joined machine - disabling Netlogon breaks domain sign-in and Group Policy processing. Only safe on a standalone/workgroup PC.");
        t.warnings = {
            QStringLiteral("Smart-card certificates stop propagating to the certificate store."),
            QStringLiteral("MDM enrollment and enterprise app provisioning will not run."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\CertPropSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DmEnrollmentSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\EntAppSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Netlogon"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }
}

}
