#include "TweakEngine.h"

// Session Manager / kernel / IFEO / AppCompat / WHEA.
// IFEO and AppCompat keys are fixed paths (known exe names / GUIDs) so they ship
// directly. The csrss ResourcePolicyStore values are deep undocumented placebo
// and are not shipped.

namespace vth::tweaks::detail {

    namespace {
        const QString kSM   = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager");
        const QString kKern = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel");
        const QString kWhea = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WHEA\\Policy");
        const QString kLayers = QStringLiteral(
            "HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers");

        QString ifeoPerf(const QString& exe) {
            return QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"
                                  "\\Image File Execution Options\\%1\\PerfOptions").arg(exe);
        }
    }

    void appendKernel(QVector<TweakSpec>& c) {
        {
            TweakSpec t;
            t.id = QStringLiteral("kernel.global-timer-resolution");
            t.category = QStringLiteral("general");
            t.subCategory = QStringLiteral("Timer resolution");
            t.name = QStringLiteral("Global timer resolution requests");
            t.description = QStringLiteral(
                "Windows 11+ opt-in that makes the highest timer-resolution request apply "
                "system-wide rather than per-process. It does nothing on its own - only if "
                "some app is already requesting a high-resolution timer - and forcing a low "
                "global timer can raise DPC/idle power with no latency benefit. No effect on "
                "Windows 10.");
            t.iconName = QStringLiteral("clock");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral("Windows 11 / Server 2022+ only; no effect on Windows 10. No measured benefit on its own; never recommended.");
            t.registryOps = { dwordOp(kKern, QStringLiteral("GlobalTimerResolutionRequests"), 1, std::nullopt) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("kernel.distribute-timers");
            t.category = QStringLiteral("advanced");
            t.subCategory = QStringLiteral("Latency");
            t.name = QStringLiteral("Distribute timers across cores");
            t.description = QStringLiteral("Spreads kernel timer expiration across all cores.");
            t.iconName = QStringLiteral("clock");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral("placebo: an undocumented value with no published measurement.");
            t.registryOps = { dwordOp(kKern, QStringLiteral("DistributeTimers"), 1, std::nullopt) };
            c.push_back(t);
        }


        {
            TweakSpec t;
            t.id = QStringLiteral("kernel.disable-threaded-dpc");
            t.category = QStringLiteral("advanced");
            t.name = QStringLiteral("Disable threaded DPCs");
            t.description = QStringLiteral("Forces deferred procedure calls to run inline instead of on DPC threads.");
            t.iconName = QStringLiteral("cpu");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "risky/placebo: can increase DPC latency on drivers that rely on threaded "
                "DPCs; measure, don't assume.");
            t.registryOps = { dwordOp(kKern, QStringLiteral("ThreadDpcEnable"), 0, std::nullopt) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("kernel.disable-tsx");
            t.category = QStringLiteral("advanced");
            t.name = QStringLiteral("Disable TSX");
            t.description = QStringLiteral("Disables Transactional Synchronization Extensions.");
            t.iconName = QStringLiteral("cpu");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral("niche: the default (enabled) is fine for general use; no gaming benefit.");
            t.registryOps = { dwordOp(kKern, QStringLiteral("DisableTsx"), 1, 0) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("kernel.protection-mode-off");
            t.category = QStringLiteral("advanced");
            t.name = QStringLiteral("Reduce object protection mode");
            t.description = QStringLiteral("Lowers Session Manager protection on system objects.");
            t.iconName = QStringLiteral("shield-off");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = { QStringLiteral("Security trade-off: weakens system object hardening.") };
            t.concern = QStringLiteral("risky: small security trade-off for no measured performance benefit.");
            t.registryOps = { dwordOp(kSM, QStringLiteral("ProtectionMode"), 0, 1) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("kernel.disable-sehop");
            t.category = QStringLiteral("advanced");
            t.name = QStringLiteral("Disable SEHOP");
            t.description = QStringLiteral(
                "Disables Structured Exception Handler Overwrite Protection and its "
                "per-exception validation.");
            t.iconName = QStringLiteral("shield-off");
            t.impact = Impact::High;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = { QStringLiteral("Security trade-off: removes an exploit-mitigation layer.") };
            t.concern = QStringLiteral("risky: security trade-off for a negligible, unmeasured gain.");
            t.registryOps = {
                dwordOp(kKern, QStringLiteral("DisableExceptionChainValidation"), 1, std::nullopt),
                dwordOp(kKern, QStringLiteral("KernelSEHOPEnabled"), 0, 1),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("kernel.disable-mitigations");
            t.category = QStringLiteral("advanced");
            t.name = QStringLiteral("Disable kernel process mitigations");
            t.description = QStringLiteral(
                "Writes the system-wide MitigationOptions blob that disables 24 kernel "
                "process mitigations.");
            t.iconName = QStringLiteral("shield-off");
            t.impact = Impact::Critical;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = { QStringLiteral("Major security trade-off: disables a large set of exploit mitigations.") };
            t.concern = QStringLiteral(
                "risky: only for a dedicated machine that runs no untrusted code; do not "
                "apply on a daily driver.");
            RegistryOp op;
            op.path = kKern;
            op.name = QStringLiteral("MitigationOptions");
            op.kind = vth::services::RegistryValueKind::Binary;
            op.appliedValue = QByteArray(24, '\x22');
            // revert == delete restores Windows' managed default
            t.registryOps = { op };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("kernel.disable-whea");
            t.category = QStringLiteral("advanced");
            t.name = QStringLiteral("Disable hardware error reporting (WHEA)");
            t.description = QStringLiteral("Disables the Windows Hardware Error Architecture checker.");
            t.iconName = QStringLiteral("shield-off");
            t.impact = Impact::High;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = { QStringLiteral("Uncorrectable hardware errors will silently corrupt data instead of being caught.") };
            t.concern = QStringLiteral(
                "risky: data-integrity trade-off; masks real, failing hardware. Rarely worth it.");
            t.registryOps = { dwordOp(kWhea, QStringLiteral("DisableWHEA"), 1, std::nullopt) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("ifeo.elevate-system");
            t.category = QStringLiteral("advanced");
            t.name = QStringLiteral("Elevate system processes (realtime)");
            t.description = QStringLiteral(
                "Raises csrss.exe (raw input), dwm.exe (compositor) and the kernel image "
                "to Realtime CPU priority with Critical I/O.");
            t.iconName = QStringLiteral("gauge");
            t.impact = Impact::High;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = { QStringLiteral("Realtime priority on system processes can starve other threads and cause stutter or instability.") };
            t.concern = QStringLiteral(
                "risky: an aggressive tuning pattern; Realtime is rarely the right class "
                "and can hurt more than it helps. Test carefully.");
            QVector<RegistryOp> ops;
            for (const QString& exe : { QStringLiteral("csrss.exe"),
                                        QStringLiteral("dwm.exe"),
                                        QStringLiteral("ntoskrnl.exe") }) {
                ops.push_back(dwordOp(ifeoPerf(exe), QStringLiteral("CpuPriorityClass"), 4, std::nullopt));
                ops.push_back(dwordOp(ifeoPerf(exe), QStringLiteral("IoPriority"), 3, std::nullopt));
            }
            t.registryOps = ops;
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("ifeo.deprioritize-background");
            t.category = QStringLiteral("advanced");
            t.name = QStringLiteral("Deprioritize background processes");
            t.description = QStringLiteral(
                "Drops svchost, Search Indexer, TrustedInstaller, Windows Update and LSASS "
                "to Idle CPU/I/O priority.");
            t.iconName = QStringLiteral("gauge");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = { QStringLiteral("Idle-priority svchost/LSASS can cause sign-in, service and security delays.") };
            t.concern = QStringLiteral(
                "risky: generic svchost hosts critical services and LSASS handles auth; "
                "Idle priority can produce odd hangs. audiodg is intentionally excluded.");
            QVector<RegistryOp> ops;
            for (const QString& exe : { QStringLiteral("svchost.exe"),
                                        QStringLiteral("SearchIndexer.exe"),
                                        QStringLiteral("TrustedInstaller.exe"),
                                        QStringLiteral("wuauclt.exe"),
                                        QStringLiteral("lsass.exe") }) {
                ops.push_back(dwordOp(ifeoPerf(exe), QStringLiteral("CpuPriorityClass"), 1, std::nullopt));
                ops.push_back(dwordOp(ifeoPerf(exe), QStringLiteral("IoPriority"), 0, std::nullopt));
                ops.push_back(dwordOp(ifeoPerf(exe), QStringLiteral("PagePriority"), 1, std::nullopt));
            }
            t.registryOps = ops;
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("compat.game-shims");
            t.category = QStringLiteral("advanced");
            t.name = QStringLiteral("Game compatibility shims");
            t.description = QStringLiteral(
                "Turns on the seven Windows compatibility layers that let older "
                "games run the way they expect on a modern build.");
            t.iconName = QStringLiteral("gamepad-2");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "snake-oil/risky: AppCompat shims have unpredictable interactions and can "
                "make some games worse; no reproducible benefit.");
            QVector<RegistryOp> ops;
            for (const QString& guid : { QStringLiteral("{4f5c5f15-2091-4068-8e0f-4f0b58565801}"),
                                         QStringLiteral("{c7f73e24-5828-42b0-b121-75b32f46e5e3}"),
                                         QStringLiteral("{1d3e7ad4-0d73-4ef9-84b8-c8b0e8a85201}"),
                                         QStringLiteral("{6a0b70a6-7c0e-4438-a1ec-8b5d1d87a93f}"),
                                         QStringLiteral("{ed463575-b7af-443f-ab28-6b97ade88e7e}"),
                                         QStringLiteral("{cf4403b9-3a71-48fd-b3a1-aabd8b5b7f55}"),
                                         QStringLiteral("{ddb6d0d4-5fa9-4d33-b0b7-e5f49fc5efd6}") })
                ops.push_back(dwordOp(kLayers, guid, 119, std::nullopt));
            t.registryOps = ops;
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("kernel.session-manager-tuning");
            t.category = QStringLiteral("advanced");
            t.name = QStringLiteral("Session manager kernel values");
            t.description = QStringLiteral(
                "Writes a broad set of Session Manager\\kernel values covering DPC queue "
                "depth/rate, dynamic-tick duration, I/O work-item queueing, watchdog timeouts, "
                "Control Flow Guard export suppression, and thread quantum/stack sizing.");
            t.iconName = QStringLiteral("cpu");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "low-level scheduler/kernel changes; review before applying. "
                "DisableControlFlowGuardCfg/Export Suppression weaken an exploit mitigation; "
                "several other names (IoQueueWorkItemToNode, ExTryQueueWorkItem, PriorityControl "
                "as a kernel value distinct from Control\\PriorityControl) are undocumented with "
                "no published measurement.");
            // The warnings are load-bearing: nothing in this tweak's name,
            // filed under advanced at Low impact, tells the reader it switches
            // off an exploit mitigation, and concern is maintainer-only.
            t.warnings = {
                QStringLiteral("Weakens Control Flow Guard, a mitigation that makes memory-corruption bugs harder to exploit"),
                QStringLiteral("Several of these values are undocumented and have no published measurement behind them"),
            };
            t.registryOps = {
                dwordOp(kKern, QStringLiteral("DynamicDpcProtocol"), 1, std::nullopt),
                dwordOp(kKern, QStringLiteral("CoalescingTimerInterval"), 0, std::nullopt),
                dwordOp(kKern, QStringLiteral("DpcQueueDepth"), 1, std::nullopt),
                dwordOp(kKern, QStringLiteral("MaxDynamicTickDuration"), 0x0000000au, std::nullopt),
                dwordOp(kKern, QStringLiteral("MaxDynamicTickDurationSize"), 0xffffffffu, std::nullopt),
                dwordOp(kKern, QStringLiteral("MaximumDpcRate"), 1, std::nullopt),
                dwordOp(kKern, QStringLiteral("ForceIdleGracePeriod"), 0, std::nullopt),
                dwordOp(kKern, QStringLiteral("DisableBufferedIoInit"), 1, std::nullopt),
                dwordOp(kKern, QStringLiteral("BufferSize"), 0x00000020u, std::nullopt),
                dwordOp(kKern, QStringLiteral("IoQueueWorkItem"), 0x00000020u, std::nullopt),
                dwordOp(kKern, QStringLiteral("IoQueueWorkItemToNode"), 0x00000020u, std::nullopt),
                dwordOp(kKern, QStringLiteral("IoQueueWorkItemEx"), 0x00000020u, std::nullopt),
                dwordOp(kKern, QStringLiteral("IoQueueThreadIrp"), 0x00000020u, std::nullopt),
                dwordOp(kKern, QStringLiteral("ExTryQueueWorkItem"), 0x00000020u, std::nullopt),
                dwordOp(kKern, QStringLiteral("ExQueueWorkItem"), 0x00000020u, std::nullopt),
                dwordOp(kKern, QStringLiteral("IoEnqueueIrp"), 0x00000020u, std::nullopt),
                dwordOp(kKern, QStringLiteral("WatchdogResumeTimeout"), 0, std::nullopt),
                dwordOp(kKern, QStringLiteral("IdleScanInterval"), 0, std::nullopt),
                dwordOp(kKern, QStringLiteral("IdleDurationExpirationTimeout"), 0, std::nullopt),
                dwordOp(kKern, QStringLiteral("DelayCloseSize"), 0, std::nullopt),
                dwordOp(kKern, QStringLiteral("DelayDerefKCBLimit"), 0, std::nullopt),
                dwordOp(kKern, QStringLiteral("WorkerFactoryThreadIdleTimeout"), 0, std::nullopt),
                dwordOp(kKern, QStringLiteral("PassiveWatchdogTimeout"), 0, std::nullopt),
                dwordOp(kKern, QStringLiteral("BugCheckUnexpectedInterrupts"), 0, std::nullopt),
                dwordOp(kKern, QStringLiteral("DisableLowQosTimerResolution"), 1, std::nullopt),
                dwordOp(kKern, QStringLiteral("DisableControlFlowGuardExportSuppression"), 1, std::nullopt),
                dwordOp(kKern, QStringLiteral("DisableControlFlowGuardCfg"), 1, std::nullopt),
                dwordOp(kKern, QStringLiteral("DpcCumulativeSoftTimeout"), 0, std::nullopt),
                dwordOp(kKern, QStringLiteral("DpcWatchdogProfileBufferSizeBytes"), 1, std::nullopt),
                dwordOp(kKern, QStringLiteral("UnlimitDpcQueue"), 1, std::nullopt),
                dwordOp(kKern, QStringLiteral("ForceForegroundBoostDecay"), 0, std::nullopt),
                dwordOp(kKern, QStringLiteral("XMMIZeroingEnable"), 0, std::nullopt),
                dwordOp(kKern, QStringLiteral("PriorityControl"), 0x00000032u, std::nullopt),
                dwordOp(kKern, QStringLiteral("DisableOverlappedExecution"), 0, std::nullopt),
                dwordOp(kKern, QStringLiteral("TimeIncrement"), 0x0000000fu, std::nullopt),
                dwordOp(kKern, QStringLiteral("QuantumLength"), 0x00000014u, std::nullopt),
                dwordOp(kKern, QStringLiteral("UseNormalStack"), 1, std::nullopt),
                dwordOp(kKern, QStringLiteral("UseNewEaBuffering"), 1, std::nullopt),
                dwordOp(kKern, QStringLiteral("StackSubSystemStackSize"), 0x00010000u, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("kernel.disable-timer-coalescing");
            t.category = QStringLiteral("advanced");
            t.name = QStringLiteral("Disable timer coalescing (system-wide)");
            t.description = QStringLiteral(
                "Sets CoalescingTimerInterval to 0 across every Session Manager subkey "
                "(kernel, Power, Memory Management, Executive, ModernSleep and Control) so "
                "no timer batching is applied anywhere in the tree.");
            t.iconName = QStringLiteral("timer");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "raises timer-interrupt frequency system-wide, which increases power draw; "
                "overlaps with kernel.session-manager-tuning's own CoalescingTimerInterval "
                "write on the same kernel key.");
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Power"),
                        QStringLiteral("CoalescingTimerInterval"), 0, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"),
                        QStringLiteral("CoalescingTimerInterval"), 0, std::nullopt),
                dwordOp(kKern, QStringLiteral("CoalescingTimerInterval"), 0, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Executive"),
                        QStringLiteral("CoalescingTimerInterval"), 0, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager"),
                        QStringLiteral("CoalescingTimerInterval"), 0, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\ModernSleep"),
                        QStringLiteral("CoalescingTimerInterval"), 0, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"),
                        QStringLiteral("CoalescingTimerInterval"), 0, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control"),
                        QStringLiteral("CoalescingTimerInterval"), 0, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("kernel.scheduler-quantum-tuning");
            t.category = QStringLiteral("advanced");
            t.name = QStringLiteral("PriorityControl scheduler and IRQ values");
            t.description = QStringLiteral(
                "Sets per-IRQ priority hints, forces desktop (non-tablet) posture, and switches "
                "foreground boost, thread-boost type and the thread scheduling model on the "
                "Control\\PriorityControl key.");
            t.iconName = QStringLiteral("cpu");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "IRQ priority values and SchedulerAssistThreadFlagOverride are undocumented "
                "kernel scheduler internals with no published measurement; overlaps with the "
                "Win32PrioritySeparation tweak already shipped in the starter catalog.");
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PriorityControl"),
                        QStringLiteral("ConvertibleSlateMode"), 0, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PriorityControl"),
                        QStringLiteral("IRQ00Priority"), 1, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PriorityControl"),
                        QStringLiteral("IRQ08Priority"), 4, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PriorityControl"),
                        QStringLiteral("IRQ16Priority"), 2, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PriorityControl"),
                        QStringLiteral("IRQ23Priority"), 3, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PriorityControl"),
                        QStringLiteral("IRQ8Priority"), 1, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PriorityControl"),
                        QStringLiteral("ForegroundBoost"), 1, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PriorityControl"),
                        QStringLiteral("ThreadBoostType"), 2, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PriorityControl"),
                        QStringLiteral("ThreadSchedulingModel"), 1, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PriorityControl"),
                        QStringLiteral("AdjustDpcThreshold"), 0x00000320u, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PriorityControl"),
                        QStringLiteral("DeepIoCoalescingEnabled"), 1, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PriorityControl"),
                        QStringLiteral("SchedulerAssistThreadFlagOverride"), 1, std::nullopt),
            };
            c.push_back(t);
        }
    }

}
