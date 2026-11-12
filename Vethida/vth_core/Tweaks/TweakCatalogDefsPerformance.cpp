#include "TweakEngine.h"

namespace vth::tweaks::detail {

void appendDefsPerformance(QVector<TweakSpec>& c) {
    {
        TweakSpec t;
        t.id = QStringLiteral("perf.ahci-disable-idle-d3");
        t.category = QStringLiteral("storage");
        t.subCategory = QStringLiteral("Storage latency");
        t.name = QStringLiteral("AHCI: disable idle D3 power state");
        t.description = QStringLiteral("Stops the AHCI storage controller from entering D3 idle power state. Removes wake-up latency on SATA SSD reads.");
        t.iconName = QStringLiteral("database");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\storahci\\Parameters\\Device"), QStringLiteral("NoIdleD3"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.proc.audio_priority");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("MMCSS");
        t.name = QStringLiteral("MMCSS audio scheduling category");
        t.description = QStringLiteral("Sets the MMCSS Audio task scheduling category to High, sets the audiodg.exe CPU priority class, and lowers the Audio task Priority value to 2 from the Windows default of 6. The two pull in opposite directions, so audio scheduling may not improve.");
        t.iconName = QStringLiteral("volume-2");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Under heavy CPU load, elevated audio threads can steal CPU from Pro Audio or DAW plugins"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile\\Tasks\\Audio"), QStringLiteral("Priority"), 2u, std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile\\Tasks\\Audio"), QStringLiteral("Scheduling Category"), QStringLiteral("High"), std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\audiodg.exe\\PerfOptions"), QStringLiteral("CpuPriorityClass"), 2u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.prefetch.boot_optimize");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Prefetch & SysMain");
        t.name = QStringLiteral("Disable Prefetcher and Superfetch");
        t.description = QStringLiteral("Turns off the Prefetcher, Superfetch/SysMain and the automatic disk-layout optimizer. On SSD/NVMe this mainly reduces background disk activity with no launch-speed loss; on mechanical hard drives it makes cold app launches and boot slower.");
        t.iconName = QStringLiteral("activity");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Application cold-start times may increase slightly on hard disk drives"),
        };
        t.registryOps = {
            // Both revert to 3, the Windows default (launch and boot prefetching on).
            // The documented range is 0 to 3.
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management\\PrefetchParameters"), QStringLiteral("EnablePrefetcher"), 0u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management\\PrefetchParameters"), QStringLiteral("EnableSuperfetch"), 0u, 3u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\OptimalLayout"), QStringLiteral("EnableAutoLayout"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.disable-cpu-core-parking");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Core parking");
        t.name = QStringLiteral("Disable CPU core parking");
        t.description = QStringLiteral("Attempts to disable CPU core parking by writing to the power-setting definition key. Note this edits the setting's allowed range rather than the active power scheme's applied value, so it may not actually keep cores unparked; core parking rarely hurts on modern Windows.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\54533251-82be-4824-96c1-47b60b740d00\\0cc5b647-c1df-4637-891a-dec35c318583"), QStringLiteral("ValueMax"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("etw.disable-readyboot");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("ETW / Boot trace");
        t.name = QStringLiteral("Disable ReadyBoot autologger");
        t.description = QStringLiteral("Stops the boot-prefetch trace session. On SSD/NVMe systems boot prefetch is wasted I/O.");
        t.iconName = QStringLiteral("database");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\ReadyBoot"), QStringLiteral("Start"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.wer.disable_dumps");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Error reporting / Telemetry");
        t.name = QStringLiteral("Disable WER crash dumps");
        t.description = QStringLiteral("Prevents Windows Error Reporting from collecting crash dumps and additional diagnostic data.");
        t.iconName = QStringLiteral("alert-triangle");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Crash diagnostic data will not be sent to Microsoft for analysis"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\Windows Error Reporting"), QStringLiteral("DontSendAdditionalData"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\Windows Error Reporting"), QStringLiteral("LoggingDisabled"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\Windows Error Reporting"), QStringLiteral("Disabled"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\Windows Error Reporting"), QStringLiteral("DontShowUI"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\Windows Error Reporting\\LocalDumps"), QStringLiteral("DumpCount"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.disk.write_cache");
        t.category = QStringLiteral("storage");
        t.subCategory = QStringLiteral("Swap file");
        t.name = QStringLiteral("Disable the swap file");
        t.description = QStringLiteral("Disables swapfile.sys, the compressed backing store Windows uses to suspend UWP and Store apps. The classic pagefile (pagefile.sys) is left alone, and disk write caching is not touched.");
        t.iconName = QStringLiteral("usb");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("With the swap file disabled, Store apps can fail under memory pressure"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("SwapfileControl"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.fs.advanced_ntfs");
        t.category = QStringLiteral("storage");
        t.subCategory = QStringLiteral("NTFS");
        t.name = QStringLiteral("Advanced NTFS filesystem optimization");
        t.description = QStringLiteral("Applies several NTFS values. The impactful, documented ones disable 8.3 short-name creation, disable last-access-time updates, enlarge the MFT zone and NTFS cache, and enable long (>260 char) paths. It also disables EFS file encryption and writes two non-standard Services\\Ntfs nonpaged-pool values that Windows does not document. Legacy 16-bit apps may break without 8.3 names.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Legacy 16-bit apps may break without 8.3 names."),
            QStringLiteral("Backup tools relying on last-access timestamps affected."),
            QStringLiteral("Symlink policies may surprise old software."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem"), QStringLiteral("NtfsDisable8dot3NameCreation"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem"), QStringLiteral("NtfsDisableLastAccessUpdate"), 0x80000001u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem"), QStringLiteral("NtfsMftZoneReservation"), 2u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem"), QStringLiteral("NtfsMemoryUsage"), 2u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem"), QStringLiteral("NtfsDisableEncryption"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem"), QStringLiteral("LongPathsEnabled"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem"), QStringLiteral("SymlinkLocalToLocalEvaluation"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem"), QStringLiteral("SymlinkLocalToRemoteEvaluation"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Ntfs"), QStringLiteral("NtfsAllocateFromNonPagedPool"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Ntfs"), QStringLiteral("NtfsForceNonPagedPoolAllocation"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.fth.disable");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Heap / Reliability");
        t.name = QStringLiteral("Disable Fault Tolerant Heap");
        t.description = QStringLiteral("Turns off the Fault Tolerant Heap, which automatically applies mitigation shims to applications that crash repeatedly. FTH engages only for chronically-crashing programs, so the performance impact of disabling it is minimal; the main effect is that such apps crash normally instead of being silently patched.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Applications with heap bugs may crash instead of being silently fixed"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\Software\\Microsoft\\FTH"), QStringLiteral("Enabled"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.game.dvr_policy_disable");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Game DVR / Game Bar");
        t.name = QStringLiteral("GameDVR extended policy disable");
        t.description = QStringLiteral("Disables Game DVR via group policy, PolicyManager, and HKLM policy paths. Complements the per-user GameDVR disable by enforcing the setting system-wide including for the DEFAULT user.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Game DVR background recording and the Win+G game clip function stop working system-wide"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\GameDVR"), QStringLiteral("AllowgameDVR"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\ApplicationManagement\\AllowGameDVR"), QStringLiteral("value"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\GameDVR"), QStringLiteral("HistoricalCaptureEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\GameBar"), QStringLiteral("ShowStartupPanel"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\GameBar"), QStringLiteral("ShowGameModeNotifications"), 0u, 1u),
            dwordOp(QStringLiteral("HKU\\.DEFAULT\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\GameDVR"), QStringLiteral("AppCaptureEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKU\\.DEFAULT\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\GameDVR"), QStringLiteral("HistoricalCaptureEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKU\\.DEFAULT\\Software\\Microsoft\\GameBar"), QStringLiteral("ShowStartupPanel"), 0u, 1u),
            dwordOp(QStringLiteral("HKU\\.DEFAULT\\Software\\Microsoft\\GameBar"), QStringLiteral("ShowGameModeNotifications"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.game.gamebar_redirect");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Game Bar");
        t.name = QStringLiteral("Redirect GameBar URL protocol");
        t.description = QStringLiteral("Redirects ms-gamebar and ms-gamebarservices URL protocols to a harmless process so Game Bar cannot be re-launched via protocol links.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("GameBar overlay cannot be invoked via ms-gamebar:// links."),
        };
        t.registryOps = {
            stringOp(QStringLiteral("HKCR\\ms-gamebar"), QStringLiteral("URL Protocol"), QStringLiteral(""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\ms-gamebar\\shell\\open\\command"), QStringLiteral(""), QStringLiteral("systray.exe"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\ms-gamebarservices"), QStringLiteral("URL Protocol"), QStringLiteral(""), std::nullopt),
            stringOp(QStringLiteral("HKCR\\ms-gamebarservices\\shell\\open\\command"), QStringLiteral(""), QStringLiteral("systray.exe"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.game.xbox_game_monitoring");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Xbox / Game monitoring");
        t.name = QStringLiteral("Disable Xbox game monitoring (XBGM)");
        t.description = QStringLiteral("Writes an EnableXBGM=0 value for the current and default user, intended to stop Xbox background game monitoring. Note the Xbox Game Monitor is a service (xbgm) normally disabled via its service Start value, so this per-user key may have limited or no effect.");
        t.iconName = QStringLiteral("monitor");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Play time tracking and Xbox-linked achievements stop logging for installed games"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Games"), QStringLiteral("EnableXBGM"), 0u, 1u),
            dwordOp(QStringLiteral("HKU\\.DEFAULT\\Software\\Microsoft\\Games"), QStringLiteral("EnableXBGM"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.cpu.hybrid_affinity");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("CPU timers / Latency");
        t.name = QStringLiteral("Optimize hybrid CPU timer distribution");
        t.description = QStringLiteral("Enables cross-core timer distribution (DistributeTimers=1) and removes the CPU reserved for background tasks (SystemResponsiveness=0), both real low-latency knobs. It also writes an undocumented HeterogeneousPolicy value and a ConvertibleSlateMode flag that concerns tablet-mode detection rather than timers. SystemResponsiveness=0 can cause audio or background-service glitches under load.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Sets SystemResponsiveness to 0, which can cause microphone, voice chat, or recording audio to glitch"),
            QStringLiteral("Distributing timers across all cores may slightly raise idle power draw on laptops"),
            QStringLiteral("Benefits mainly hybrid CPUs; on older uniform-core designs the effect is small or neutral"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("DistributeTimers"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile"), QStringLiteral("SystemResponsiveness"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("HeterogeneousPolicy"), 5u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PriorityControl"), QStringLiteral("ConvertibleSlateMode"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.kernel.executive_threads");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Kernel worker threads");
        t.name = QStringLiteral("Kernel executive worker threads");
        t.description = QStringLiteral("Adds a small number of extra critical and delayed kernel worker threads via the documented AdditionalCriticalWorkerThreads / AdditionalDelayedWorkerThreads values. On a typical desktop the throughput benefit is negligible; the main cost is slightly more kernel memory. (The included MaxTimeSeparationBeforeCorrect value is unrelated, concerning time-sync tolerance.)");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Slightly higher memory usage for additional kernel threads"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Executive"), QStringLiteral("AdditionalCriticalWorkerThreads"), 2u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Executive"), QStringLiteral("AdditionalDelayedWorkerThreads"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Executive"), QStringLiteral("MaxTimeSeparationBeforeCorrect"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.mem.interrupt_steering");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Interrupts");
        t.name = QStringLiteral("Disable interrupt steering");
        t.description = QStringLiteral("Writes InterruptSteeringDisabled=1 to the Control root to stop Windows dynamically reassigning interrupts across cores. It is unclear whether Windows consumes this value at this path, so the latency benefit is unverified.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("May cause uneven interrupt distribution on multi-core systems"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control"), QStringLiteral("InterruptSteeringDisabled"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.mem.large_pages");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Large pages");
        t.name = QStringLiteral("Enable large pages");
        t.description = QStringLiteral("Enables large-page support in the memory manager. Large pages can cut TLB misses for a few memory-intensive workloads, but they require the 'Lock pages in memory' privilege and, applied system-wide, can increase committed memory and fragmentation, so the effect is workload-dependent and not universally positive.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Without the 'Lock pages in memory' privilege granted, apps fall back and committed memory grows"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("UseLargePages"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.mmcss.audio_task");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("MMCSS");
        t.name = QStringLiteral("MMCSS audio task priority");
        t.description = QStringLiteral("Sets the MMCSS Audio task SFIO priority to High and lowers its Priority value from the Windows default of 6 to 2. The two pull in opposite directions, so audio scheduling may not improve.");
        t.iconName = QStringLiteral("microphone-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Lowers the Audio task priority index from 6 to 2; audio scheduling may not improve and can get worse"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile\\Tasks\\Audio"), QStringLiteral("Priority"), 2u, 6u),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile\\Tasks\\Audio"), QStringLiteral("SFIO Priority"), QStringLiteral("High"), QStringLiteral("Normal")),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.mmcss.display_post_processing");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("MMCSS");
        t.name = QStringLiteral("MMCSS display post-processing task priority");
        t.description = QStringLiteral("Configures the MMCSS DisplayPostProcessing task for high priority scheduling. Sets GPU priority to 18, CPU priority to 8, and enables high scheduling category with low latency sensitivity.");
        t.iconName = QStringLiteral("monitor");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Could impact other GPU-intensive tasks"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile\\Tasks\\DisplayPostProcessing"), QStringLiteral("BackgroundPriority"), 24u, 8u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile\\Tasks\\DisplayPostProcessing"), QStringLiteral("GPU Priority"), 18u, 8u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile\\Tasks\\DisplayPostProcessing"), QStringLiteral("Priority"), 8u, 4u),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile\\Tasks\\DisplayPostProcessing"), QStringLiteral("Scheduling Category"), QStringLiteral("High"), QStringLiteral("Medium")),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile\\Tasks\\DisplayPostProcessing"), QStringLiteral("SFIO Priority"), QStringLiteral("High"), QStringLiteral("Normal")),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile\\Tasks\\DisplayPostProcessing"), QStringLiteral("Latency Sensitive"), QStringLiteral("True"), QStringLiteral("False")),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.mmcss.latency_tuning");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("MMCSS");
        t.name = QStringLiteral("MMCSS gaming latency");
        t.description = QStringLiteral("Most of the Games-task values it writes already match Windows defaults, so the real effect comes from SystemResponsiveness=0 (removes the CPU reserved for background tasks) and NoLazyMode=1 (keeps MMCSS scheduling active). Those two can lower game latency but may cause microphone, voice-chat or recording glitches under load.");
        t.iconName = QStringLiteral("activity");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Sets SystemResponsiveness to 0, which can starve audio and background services under heavy game load"),
            QStringLiteral("Microphone, voice chat, or recording software may glitch or cut out"),
            QStringLiteral("Major Windows updates can reset the Games profile keys and require reapplying the tweak"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile\\Tasks\\Games"), QStringLiteral("GPU Priority"), 8u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile\\Tasks\\Games"), QStringLiteral("Priority"), 6u, std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile\\Tasks\\Games"), QStringLiteral("Scheduling Category"), QStringLiteral("High"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile\\Tasks\\Games"), QStringLiteral("SFIO Priority"), QStringLiteral("High"), std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile\\Tasks\\Games"), QStringLiteral("Clock Rate"), 10000u, std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile\\Tasks\\Games"), QStringLiteral("Background Only"), QStringLiteral("False"), std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile"), QStringLiteral("SystemResponsiveness"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile"), QStringLiteral("NoLazyMode"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.mmcss.service_start");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("MMCSS");
        t.name = QStringLiteral("Set MMCSS service to automatic");
        t.description = QStringLiteral("Ensures the Multimedia Class Scheduler Service is set to start automatically so media and game threads get MMCSS priority scheduling. On most systems MMCSS is already Automatic, so this typically just guarantees the default state.");
        t.iconName = QStringLiteral("activity");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Starts an extra service at boot; tools that rely on MMCSS being disabled may behave differently"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\MMCSS"), QStringLiteral("Start"), 4u, 2u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.nvme.driver_params");
        t.category = QStringLiteral("storage");
        t.subCategory = QStringLiteral("NVMe");
        t.name = QStringLiteral("Increase NVMe max transfer size");
        t.description = QStringLiteral("Increases the NVMe maximum transfer size per I/O to 1 MB for improved sequential throughput.");
        t.iconName = QStringLiteral("database");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Drives whose firmware does not handle 1 MB transfers can stall or drop commands under heavy load"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\stornvme\\Parameters"), QStringLiteral("MaxTransferSizePerIo"), 0x100000u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.power.reliability");
        t.category = QStringLiteral("power");
        t.subCategory = QStringLiteral("Telemetry");
        t.name = QStringLiteral("Disable reliability timestamp");
        t.description = QStringLiteral("Sets the reliability timestamp interval to 0, disabling periodic dirty-shutdown detection writes.");
        t.iconName = QStringLiteral("clock");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Reliability Monitor loses the periodic heartbeat; dirty-shutdown detection may misreport recent crashes"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Reliability"), QStringLiteral("TimeStampInterval"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.prefetch.disable");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Prefetch & SysMain");
        t.name = QStringLiteral("Disable prefetch and Superfetch");
        t.description = QStringLiteral("Disables the Prefetcher, Superfetch/SysMain and boot tracing. On SSD/NVMe this mainly cuts background disk activity; on mechanical drives it slows cold app launches and boot. Note it also sets a Superfetch tracing-state flag, and it duplicates the 'Disable Prefetcher and Superfetch' tweak.");
        t.iconName = QStringLiteral("rotate");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Without Prefetch and Superfetch, cold-start app launches and boot times become noticeably slower on HDDs"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management\\PrefetchParameters"), QStringLiteral("EnablePrefetcher"), 0u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management\\PrefetchParameters"), QStringLiteral("EnableSuperfetch"), 0u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management\\PrefetchParameters"), QStringLiteral("EnableBoottrace"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management\\PrefetchParameters"), QStringLiteral("SfTracingState"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.mem.page_combining_disable");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Memory (MMAgent)");
        t.name = QStringLiteral("Disable page combining (MMAgent)");
        t.description = QStringLiteral("Disables the Windows Memory Management Agent page combining feature via Disable-MMAgent -PageCombining. Page combining merges identical memory pages to save RAM but adds CPU overhead from periodic page scanning.");
        t.iconName = QStringLiteral("layers");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Higher physical memory usage"),
            QStringLiteral("Reduced memory efficiency with many similar processes"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("Disable-MMAgent -PageCombining") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("Enable-MMAgent -PageCombining") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.kernel.mitigations_disable");
        t.category = QStringLiteral("advanced");
        t.name = QStringLiteral("Disable kernel mitigations audit");
        t.description = QStringLiteral("Disables kernel mitigation audit mode and KVA shadow overhead for reduced syscall latency.");
        t.iconName = QStringLiteral("shield-off");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Turns off the audit channel that security tools and enterprise EDR products use to observe mitigation events"),
            QStringLiteral("A reboot is required for the change to take effect"),
            QStringLiteral("Future Windows updates may add new audit flags that re-enable some overhead"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("MitigationAuditOptions"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.disk.storport_telemetry");
        t.category = QStringLiteral("storage");
        t.name = QStringLiteral("Disable StorAHCI telemetry");
        t.description = QStringLiteral("Disables telemetry collection in the StorAHCI driver, reducing background disk overhead.");
        t.iconName = QStringLiteral("database");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Windows no longer reports SATA drive health counters to the cloud diagnostics pipeline"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\storahci\\Parameters\\Device"), QStringLiteral("TelemetryCollectionEnabled"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.disable_vbs");
        t.category = QStringLiteral("advanced");
        t.name = QStringLiteral("Disable virtualization-based security (VBS)");
        t.description = QStringLiteral("Disables VBS and Credential Guard to reclaim CPU overhead used by hypervisor-enforced memory isolation.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Removes hypervisor-based kernel memory protection"),
            QStringLiteral("Credential Guard will be disabled"),
            QStringLiteral("Reduces resistance to kernel-level exploits"),
            QStringLiteral("vanguard"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\DeviceGuard"), QStringLiteral("EnableVirtualizationBasedSecurity"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Lsa"), QStringLiteral("LsaCfgFlags"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.disk.nvme_advanced");
        t.category = QStringLiteral("storage");
        t.name = QStringLiteral("Advanced NVMe throttling disable (Classpnp)");
        t.description = QStringLiteral("Disables Classpnp-level NVMe performance throttling and idle power modes.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("NVMe may thermal throttle without OS-level pacing."),
            QStringLiteral("Increased power consumption on storage controllers."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Classpnp"), QStringLiteral("NVMeDisablePerfThrottling"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\stornvme\\Parameters\\Device"), QStringLiteral("IdlePowerMode"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.cpu.dpc_latency");
        t.category = QStringLiteral("advanced");
        t.name = QStringLiteral("Reduce DPC latency checks");
        t.description = QStringLiteral("Sets DpcWatchdogProfileOffset to 0, reducing deferred procedure call latency monitoring overhead.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Disables a kernel watchdog profiling hook, reducing data the OS can use to diagnose driver stalls"),
            QStringLiteral("If a misbehaving driver hangs the system, you may have fewer hints in the event log"),
            QStringLiteral("Touches undocumented kernel behavior that future Windows updates could change"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("DpcWatchdogProfileOffset"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.gpu.scheduler_quantum");
        t.category = QStringLiteral("gpu");
        t.name = QStringLiteral("Optimize GPU scheduler quantum");
        t.description = QStringLiteral("Reduces GPU scheduler quantum and sets VsyncIdleTimeout to minimize compositor latency.");
        t.iconName = QStringLiteral("monitor");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Average framerate in GPU-bound games may drop by a small amount in exchange for lower input lag"),
            QStringLiteral("Touches undocumented scheduler keys that future Windows or GPU driver updates could change"),
            QStringLiteral("A reboot is required for the change to take effect"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Scheduler"), QStringLiteral("FlipQueueSize"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Scheduler"), QStringLiteral("VsyncIdleTimeout"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Scheduler"), QStringLiteral("MonitorLatencyTolerance"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.diag.hardware_monitoring");
        t.category = QStringLiteral("advanced");
        t.name = QStringLiteral("Enable performance diagnostics logging");
        t.description = QStringLiteral("Enables Windows performance diagnostics logging for boot time, shutdown, and standby resume analysis.");
        t.iconName = QStringLiteral("activity");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Boot and shutdown tracing add continuous ETW logging that writes a few MB per day"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Reliability Analysis\\WMI"), QStringLiteral("WMIEnable"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.io.environment_latency");
        t.category = QStringLiteral("storage");
        t.name = QStringLiteral("I/O latency environment variables");
        t.description = QStringLiteral("Sets 51 system environment variables under Session Manager\\\\Environment to minimize I/O latency: CPU interrupt/idle/boost policies, I/O completion and request limits, disk/storage/USB/PCIe/GPU/audio/network/memory pending interrupt zeroing, timer resolution, thread scheduling, DWM compositor interrupt reduction, and MKL CPU type hint. All values are REG_SZ strings.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("CPU idle, power throttling and dynamic tick are off; laptop battery drains substantially faster"),
            QStringLiteral("Forced constant timer resolution can raise idle power draw on every CPU core"),
        };
        t.registryOps = {
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("CPU_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("CPU_MAX_PENDING_IO"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("CPU_IDLE_POLICY"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("CPU_BOOST_POLICY"), QStringLiteral("2"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("CPU_MAX_FREQUENCY"), QStringLiteral("100"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("CPU_INTERRUPT_BALANCE_POLICY"), QStringLiteral("1"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("MKL_DEBUG_CPU_TYPE"), QStringLiteral("10"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("IO_COMPLETION_POLICY"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("IO_REQUEST_LIMIT"), QStringLiteral("1024"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("DISK_MAX_PENDING_IO"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("IO_PRIORITY"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("DISK_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("IO_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("POWER_THROTTLE_POLICY"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("POWER_IDLE_TIMEOUT"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("CPU_POWER_POLICY"), QStringLiteral("1"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("DISABLE_DYNAMIC_TICK"), QStringLiteral("yes"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("MEMORY_MAX_ALLOCATION"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("MEMORY_LATENCY_POLICY"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("MEMORY_PREFETCH_POLICY"), QStringLiteral("2"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("MEMORY_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("DWM_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("DWM_COMPOSITOR_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("NETWORK_BUFFER_SIZE"), QStringLiteral("512"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("NETWORK_INTERRUPT_COALESCING"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("NETWORK_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("TIMER_RESOLUTION"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("THREAD_SCHEDULER_POLICY"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("GPU_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("NETWORK_ADAPTER_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("NETWORK_ADAPTER_MAX_PENDING_IO"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("NETWORK_ADAPTER_INTERRUPT_MODERATION"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("NETWORK_ADAPTER_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("STORAGE_DEVICE_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("STORAGE_DEVICE_MAX_PENDING_IO"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("STORAGE_DEVICE_COMPLETION_POLICY"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("STORAGE_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("STORAGE_DEVICE_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("USB_DEVICE_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("USB_DEVICE_MAX_PENDING_IO"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("USB_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("USB_DEVICE_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("PCIE_DEVICE_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("PCIE_DEVICE_MAX_PENDING_IO"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("PCIE_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("PCIE_DEVICE_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("GPU_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("GPU_MAX_PENDING_COMPUTE"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("GPU_MAX_PENDING_RENDER"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("AUDIO_DEVICE_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("AUDIO_DEVICE_BUFFER_SIZE"), QStringLiteral("512"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("AUDIO_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("AUDIO_DEVICE_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("DEVICE_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("DEVICE_MAX_PENDING_IO"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("DEVICE_COMPLETION_POLICY"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), QStringLiteral("DEVICE_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.kernel.advanced_scheduler");
        t.category = QStringLiteral("advanced");
        t.name = QStringLiteral("Advanced kernel scheduler and DPC settings");
        t.description = QStringLiteral("Configures aggressive kernel scheduler, DPC, IRQ, quantum, and watchdog parameters under Session Manager\\\\kernel. Includes DPC queue depth limits, timer serialization, IO work item sizing, watchdog disabling, idle scan intervals, CFG suppression, priority control, quantum length, stack configuration, and XMMI zeroing.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Critical;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Disabling the DPC watchdog means a stuck driver freezes the whole machine instead of bugchecking cleanly"),
            QStringLiteral("Suppressing Control Flow Guard weakens kernel exploit mitigations"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("DynamicDpcProtocol"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("InterruptSteeringDisabled"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("SerializeTimerExpiration"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("DpcTimeout"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("DpcQueueDepth"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("MaxDynamicTickDurationSize"), 0xffffffffu, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("MaximumDpcQueueDepth"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("MaximumDpcRate"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("ForceIdleGracePeriod"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("IdealDpcRate"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("MaximumSharedReadyQueueSize"), 0x80u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("DisableBufferedIoInit"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("BufferSize"), 0x20u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("IoQueueWorkItem"), 0x20u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("IoQueueWorkItemToNode"), 0x20u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("IoQueueWorkItemEx"), 0x20u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("IoQueueThreadIrp"), 0x20u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("ExTryQueueWorkItem"), 0x20u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("ExQueueWorkItem"), 0x20u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("IoEnqueueIrp"), 0x20u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("WatchdogResumeTimeout"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("IdleScanInterval"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("IdleDurationExpirationTimeout"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("DelayCloseSize"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("DelayDerefKCBLimit"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("WorkerFactoryThreadIdleTimeout"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("PassiveWatchdogTimeout"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("BugCheckUnexpectedInterrupts"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("DisableLowQosTimerResolution"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("DisableControlFlowGuardExportSuppression"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("DisableControlFlowGuardCfg"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("DpcCumulativeSoftTimeout"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("DpcWatchdogProfileBufferSizeBytes"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("UnlimitDpcQueue"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("ForceForegroundBoostDecay"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("SplitLargeCaches"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("XMMIZeroingEnable"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("PriorityControl"), 0x32u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("DisableOverlappedExecution"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("TimeIncrement"), 0x0fu, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("QuantumLength"), 0x14u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("UseNormalStack"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("UseNewEaBuffering"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("StackSubSystemStackSize"), 0x10000u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.kernel.coalescing_extended");
        t.category = QStringLiteral("advanced");
        t.name = QStringLiteral("Extended coalescing timer disable");
        t.description = QStringLiteral("Disables coalescing timer intervals across additional registry paths not covered by the base DPC latency tweak: Session Manager Power, Control root, ControlSet001 Control, and Power root.");
        t.iconName = QStringLiteral("clock");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Timers fire individually instead of coalescing; idle CPU wakes more often and battery life drops"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Power"), QStringLiteral("CoalescingTimerInterval"), 0u, 15u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control"), QStringLiteral("CoalescingTimerInterval"), 0u, 15u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ControlSet001\\Control"), QStringLiteral("CoalescingTimerInterval"), 0u, 15u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("CoalescingTimerInterval"), 0u, 15u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.kernel.tick_duration");
        t.category = QStringLiteral("advanced");
        t.name = QStringLiteral("Kernel tick duration optimization");
        t.description = QStringLiteral("Optimizes kernel timer tick parameters: sets debug poll interval and max dynamic tick duration to 1000, enables serialized timer expiration for more deterministic interrupt timing.");
        t.iconName = QStringLiteral("clock");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Serialized timer expiration raises interrupt load on every CPU; battery life suffers on laptops"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("DebugPollInterval"), 1000u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ControlSet001\\Control\\Session Manager\\kernel"), QStringLiteral("DebugPollInterval"), 1000u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ControlSet001\\Control\\Session Manager\\kernel"), QStringLiteral("MaxDynamicTickDuration"), 1000u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ControlSet001\\Control\\Session Manager\\kernel"), QStringLiteral("SerializeTimerExpiration"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.mem.advanced_pool");
        t.category = QStringLiteral("advanced");
        t.name = QStringLiteral("Advanced memory pool configuration");
        t.description = QStringLiteral("Sets nonpaged/paged pool sizes, I/O page lock limits, pool usage maximums, dynamic memory, cache unmap-behind, modified write maximums, session pool/view sizes, and snap unloads for memory throughput.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Kernel pool overcommit can bugcheck the system under sustained heavy load"),
            QStringLiteral("Disabled driver-verifier write protection lowers detection of bad kernel drivers"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("NonPagedPoolSize"), 192u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("PagedPoolSize"), 192u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("IoPageLockLimit"), 4294967295u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("PoolUsageMaximum"), 96u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("DynamicMemory"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("CacheUnmapBehindLengthInMB"), 256u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("ModifiedWriteMaximum"), 32u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("SessionPoolSize"), 100u, 32u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("SessionViewSize"), 256u, 32u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("SystemCacheLimit"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("SessionSpaceLimit"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("SnapUnloads"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("MapAllocationFragment"), 65536u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("MakeLowMemory"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("EnforceWriteProtection"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("DontVerifyRandomDrivers"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("EnableLowVaAccess"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("WriteWatch"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("OverlayTestMode"), 5u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("ProtectionMode"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.mem.processor_power_gating");
        t.category = QStringLiteral("power");
        t.name = QStringLiteral("Disable processor power gating");
        t.description = QStringLiteral("Prevents CPU cores from entering low-power states by setting FeatureSettingsOverride=0x400 and an effectively infinite power state timeout on the Processor service.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("CPU cores stay out of idle C-states; idle power draw, heat and laptop fan noise rise sharply"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Processor"), QStringLiteral("FeatureSettingsOverride"), 1024u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Processor"), QStringLiteral("FeatureSettingsOverrideMask"), 1024u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Processor"), QStringLiteral("PowerStateTimeout"), 2147483647u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.mmcss.games_profile");
        t.category = QStringLiteral("advanced");
        t.name = QStringLiteral("Optimize MMCSS games profile");
        // With Scheduling Category High, MMCSS treats Priority as 2 whatever it holds,
        // and doesn't use SFIO Priority or GPU Priority (Microsoft's MMCSS docs). The
        // category is the part that does something.
        t.description = QStringLiteral("Moves the MMCSS Games task into the High scheduling category, so the multimedia scheduler favors game threads over other multimedia work when the CPU is contended.");
        t.iconName = QStringLiteral("monitor");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Background apps, recording tools, and voice chat can feel slower while a game is running"),
            QStringLiteral("Audio from non-game apps may stutter when CPU is saturated"),
            QStringLiteral("Windows updates can reset the Games profile keys and require reapplying the tweak"),
        };
        t.registryOps = {
            // Stock values for the Games task, from Microsoft's MMCSS reference:
            // Priority REG_DWORD 2, Scheduling Category REG_SZ "Medium",
            // SFIO Priority REG_SZ "Normal", GPU Priority REG_DWORD 8.
            // Priority's documented range is 1 to 8.
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile\\Tasks\\Games"), QStringLiteral("Priority"), 6u, 2u),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile\\Tasks\\Games"), QStringLiteral("Scheduling Category"), QStringLiteral("High"), QStringLiteral("Medium")),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile\\Tasks\\Games"), QStringLiteral("SFIO Priority"), QStringLiteral("High"), QStringLiteral("Normal")),
            // GPU Priority is REG_DWORD on a stock install. Applied and revert are both
            // 8, the default; this op only fixes the type where a string was written.
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile\\Tasks\\Games"), QStringLiteral("GPU Priority"), 8u, 8u),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile\\Tasks\\Games"), QStringLiteral("Latency Sensitive"), QStringLiteral("True"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.irq.msi_mode");
        t.category = QStringLiteral("advanced");
        t.name = QStringLiteral("Enable MSI mode for GPU");
        t.description = QStringLiteral("Enables Message Signaled Interrupts (MSI) for the GPU, reducing interrupt latency and DPC overhead.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("May cause instability on very old GPU hardware that does not properly support MSI"),
            QStringLiteral("Requires reboot to take effect"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PnP\\Pci"), QStringLiteral("MSISupported"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.nvme.control_device");
        t.category = QStringLiteral("storage");
        t.name = QStringLiteral("NVMe control device parameters");
        t.description = QStringLiteral("Configures the StorNVMe control device path with deeper queue depths (100), higher outstanding command limits (598), and expanded host memory buffer at the system level.");
        t.iconName = QStringLiteral("usb");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Non-standard NVMe queue depths can expose controller firmware bugs and trigger storage stalls or data loss under load"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorNVMe\\Parameters\\Device"), QStringLiteral("StorNVMeAllowZeroLatency"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorNVMe\\Parameters\\Device"), QStringLiteral("QueueDepth"), 100u, 32u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorNVMe\\Parameters\\Device"), QStringLiteral("NvmeMaxReadSplit"), 4u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorNVMe\\Parameters\\Device"), QStringLiteral("NvmeMaxWriteSplit"), 4u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorNVMe\\Parameters\\Device"), QStringLiteral("ForceFlush"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorNVMe\\Parameters\\Device"), QStringLiteral("ImmediateData"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorNVMe\\Parameters\\Device"), QStringLiteral("MaxSegmentsPerCommand"), 598u, 32u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorNVMe\\Parameters\\Device"), QStringLiteral("MaxOutstandingCmds"), 598u, 32u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorNVMe\\Parameters\\Device"), QStringLiteral("ForceEagerWrites"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorNVMe\\Parameters\\Device"), QStringLiteral("MaxQueuedCommands"), 598u, 32u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorNVMe\\Parameters\\Device"), QStringLiteral("MaxOutstandingIORequests"), 598u, 32u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorNVMe\\Parameters\\Device"), QStringLiteral("NumberOfRequests"), 5376u, 128u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorNVMe\\Parameters\\Device"), QStringLiteral("IoSubmissionQueueCount"), 3u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorNVMe\\Parameters\\Device"), QStringLiteral("IoQueueDepth"), 100u, 32u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorNVMe\\Parameters\\Device"), QStringLiteral("HostMemoryBufferBytes"), 5376u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorNVMe\\Parameters\\Device"), QStringLiteral("ArbitrationBurst"), 598u, 16u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.nvme-disable-interrupt-coalescing");
        t.category = QStringLiteral("advanced");
        t.name = QStringLiteral("Disable NVMe interrupt coalescing");
        t.description = QStringLiteral("Stops the NVMe driver from batching completion interrupts. Lower latency per I/O.");
        t.iconName = QStringLiteral("disc");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\stornvme\\Parameters\\Device"), QStringLiteral("InterruptCoalescingTime"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.power.latency_tolerance");
        t.category = QStringLiteral("power");
        t.name = QStringLiteral("Disable exit latency tolerance");
        t.description = QStringLiteral("Sets ExitLatency to 0 and disables latency-check gating to prevent power-state transition delays.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("System will skip latency-based gating on hardware low-power transitions, raising idle power draw"),
            QStringLiteral("Laptops will see shorter battery life"),
            QStringLiteral("Touches undocumented keys that future Windows updates could change"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("ExitLatency"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("ExitLatencyCheckEnabled"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.prefetch.advanced_params");
        t.category = QStringLiteral("advanced");
        t.name = QStringLiteral("Advanced prefetch parameters");
        t.description = QStringLiteral("Sets additional prefetch parameters: zero FIFS operation max block time, zero prefetch delay, and enables FI block debug break for more aggressive prefetch behavior.");
        t.iconName = QStringLiteral("refresh-cw");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Altered prefetch timing may cause slower launches on HDDs"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management\\PrefetchParameters"), QStringLiteral("FIFSOpMaxBlockTimeMs"), 0u, 1000u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management\\PrefetchParameters"), QStringLiteral("FIBlockDbgBreak"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management\\PrefetchParameters"), QStringLiteral("FIPrefetchDelayMs"), 0u, 1000u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.proc.ifeo_priorities");
        t.category = QStringLiteral("advanced");
        t.name = QStringLiteral("Optimize process priorities (IFEO)");
        t.description = QStringLiteral("Set CPU and I/O priorities for critical Windows processes via Image File Execution Options. Prioritizes csrss, dwm, and ntoskrnl while deprioritizing search indexing and non-essential services.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Mis-setting priorities on csrss, lsass or ntoskrnl can hang or bluescreen the system at boot"),
            QStringLiteral("Search, logon and service processes are demoted; Windows Search and some background tasks feel sluggish"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\csrss.exe\\PerfOptions"), QStringLiteral("CpuPriorityClass"), 4u, 2u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\csrss.exe\\PerfOptions"), QStringLiteral("IoPriority"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\dwm.exe\\PerfOptions"), QStringLiteral("CpuPriorityClass"), 4u, 2u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\dwm.exe\\PerfOptions"), QStringLiteral("IoPriority"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\ntoskrnl.exe\\PerfOptions"), QStringLiteral("CpuPriorityClass"), 4u, 2u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\ntoskrnl.exe\\PerfOptions"), QStringLiteral("IoPriority"), 3u, 2u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\audiodg.exe\\PerfOptions"), QStringLiteral("CpuPriorityClass"), 2u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\audiodg.exe\\PerfOptions"), QStringLiteral("IoPriority"), 2u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\svchost.exe\\PerfOptions"), QStringLiteral("CpuPriorityClass"), 1u, 2u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\svchost.exe\\PerfOptions"), QStringLiteral("IoPriority"), 1u, 2u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\lsass.exe\\PerfOptions"), QStringLiteral("CpuPriorityClass"), 1u, 2u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\lsass.exe\\PerfOptions"), QStringLiteral("IoPriority"), 0u, 2u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\SearchIndexer.exe\\PerfOptions"), QStringLiteral("CpuPriorityClass"), 0u, 2u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\SearchIndexer.exe\\PerfOptions"), QStringLiteral("IoPriority"), 0u, 2u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.uwp.resource_policy_uncap");
        t.category = QStringLiteral("advanced");
        t.name = QStringLiteral("UWP resource policy store uncap all policy sets");
        t.description = QStringLiteral("Writes aggressive resource policy values across all 100 ResourcePolicyStore PolicySet categories in the Windows kernel resource manager. Sets CPU priority to UnmanagedAboveNormal, marks workloads as ResourceIntensive with Foreground scheduling, Critical importance, and removes IO and Memory caps (NoCap). Affects how the Windows resource manager throttles UWP/modern app background tasks, foreground agents, VoIP workers, and system services.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Removing UWP background-task caps lets Store apps consume CPU, I/O and battery without throttling"),
            QStringLiteral("Hundreds of ResourcePolicyStore values are overwritten; misbehaving apps may no longer be contained by the OS"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ApplicationService"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ApplicationService"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ApplicationService"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ApplicationService"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ApplicationService"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ApplicationService"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ApplicationServiceElastic"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ApplicationServiceElastic"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ApplicationServiceElastic"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ApplicationServiceElastic"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ApplicationServiceElastic"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ApplicationServiceElastic"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ApplicationServiceHighPriElastic"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ApplicationServiceHighPriElastic"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ApplicationServiceHighPriElastic"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ApplicationServiceHighPriElastic"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ApplicationServiceHighPriElastic"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ApplicationServiceHighPriElastic"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ApplicationServiceHighPriority"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ApplicationServiceHighPriority"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ApplicationServiceHighPriority"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ApplicationServiceHighPriority"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ApplicationServiceHighPriority"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ApplicationServiceHighPriority"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ApplicationServiceRemote"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ApplicationServiceRemote"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ApplicationServiceRemote"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ApplicationServiceRemote"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ApplicationServiceRemote"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ApplicationServiceRemote"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\AppToAppTarget"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\AppToAppTarget"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\AppToAppTarget"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\AppToAppTarget"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\AppToAppTarget"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\AppToAppTarget"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\BackgroundAudioPlayer"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\BackgroundAudioPlayer"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\BackgroundAudioPlayer"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\BackgroundAudioPlayer"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\BackgroundAudioPlayer"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\BackgroundAudioPlayer"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\BackgroundCachedFileUpdater"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\BackgroundCachedFileUpdater"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\BackgroundCachedFileUpdater"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\BackgroundCachedFileUpdater"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\BackgroundCachedFileUpdater"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\BackgroundCachedFileUpdater"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\BackgroundTaskCompletion"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\BackgroundTaskCompletion"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\BackgroundTaskCompletion"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\BackgroundTaskCompletion"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\BackgroundTaskCompletion"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\BackgroundTaskCompletion"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\BackgroundTaskDebug"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\BackgroundTaskDebug"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\BackgroundTaskDebug"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\BackgroundTaskDebug"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\BackgroundTaskDebug"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\BackgroundTaskDebug"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\BackgroundTransfer"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\BackgroundTransfer"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\BackgroundTransfer"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\BackgroundTransfer"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\BackgroundTransfer"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\BackgroundTransfer"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\BackgroundTransferNetworkState"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\BackgroundTransferNetworkState"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\BackgroundTransferNetworkState"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\BackgroundTransferNetworkState"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\BackgroundTransferNetworkState"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\BackgroundTransferNetworkState"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\Balloon"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\Balloon"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\Balloon"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\Balloon"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\Balloon"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\Balloon"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\CalendarProviderAsChild"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\CalendarProviderAsChild"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\CalendarProviderAsChild"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\CalendarProviderAsChild"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\CalendarProviderAsChild"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\CalendarProviderAsChild"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\CallingEvent"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\CallingEvent"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\CallingEvent"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\CallingEvent"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\CallingEvent"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\CallingEvent"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\CallingEventHighPriority"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\CallingEventHighPriority"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\CallingEventHighPriority"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\CallingEventHighPriority"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\CallingEventHighPriority"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\CallingEventHighPriority"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ChatMessageNotification"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ChatMessageNotification"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ChatMessageNotification"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ChatMessageNotification"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ChatMessageNotification"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ChatMessageNotification"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ComponentTarget"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ComponentTarget"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ComponentTarget"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ComponentTarget"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ComponentTarget"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ComponentTarget"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ContinuousBackgroundExecution"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ContinuousBackgroundExecution"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ContinuousBackgroundExecution"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ContinuousBackgroundExecution"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ContinuousBackgroundExecution"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ContinuousBackgroundExecution"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\CortanaSpeechBackground"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\CortanaSpeechBackground"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\CortanaSpeechBackground"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\CortanaSpeechBackground"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\CortanaSpeechBackground"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\CortanaSpeechBackground"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\CreateProcess"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\CreateProcess"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\CreateProcess"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\CreateProcess"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\CreateProcess"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\CreateProcess"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\DefaultModernBackgroundTask"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\DefaultModernBackgroundTask"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\DefaultModernBackgroundTask"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\DefaultModernBackgroundTask"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\DefaultModernBackgroundTask"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\DefaultModernBackgroundTask"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\DefaultPPLE"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\DefaultPPLE"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\DefaultPPLE"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\DefaultPPLE"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\DefaultPPLE"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\DefaultPPLE"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\DefaultPPLE2"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\DefaultPPLE2"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\DefaultPPLE2"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\DefaultPPLE2"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\DefaultPPLE2"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\DefaultPPLE2"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\EmCreateProcess"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\EmCreateProcess"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\EmCreateProcess"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\EmCreateProcess"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\EmCreateProcess"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\EmCreateProcess"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\EmCreateProcessNormalPriority"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\EmCreateProcessNormalPriority"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\EmCreateProcessNormalPriority"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\EmCreateProcessNormalPriority"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\EmCreateProcessNormalPriority"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\EmCreateProcessNormalPriority"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\EmptyHost"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\EmptyHost"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\EmptyHost"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\EmptyHost"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\EmptyHost"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\EmptyHost"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\EmptyHostHighPriority"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\EmptyHostHighPriority"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\EmptyHostHighPriority"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\EmptyHostHighPriority"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\EmptyHostHighPriority"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\EmptyHostHighPriority"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\EmptyHostPPLE"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\EmptyHostPPLE"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\EmptyHostPPLE"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\EmptyHostPPLE"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\EmptyHostPPLE"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\EmptyHostPPLE"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\FileProviderTarget"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\FileProviderTarget"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\FileProviderTarget"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\FileProviderTarget"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\FileProviderTarget"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\FileProviderTarget"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ForegroundAgent"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ForegroundAgent"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ForegroundAgent"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ForegroundAgent"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ForegroundAgent"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ForegroundAgent"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ForegroundCachedFileUpdater"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ForegroundCachedFileUpdater"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ForegroundCachedFileUpdater"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ForegroundCachedFileUpdater"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ForegroundCachedFileUpdater"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ForegroundCachedFileUpdater"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ForegroundTaskCompletion"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ForegroundTaskCompletion"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ForegroundTaskCompletion"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ForegroundTaskCompletion"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ForegroundTaskCompletion"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ForegroundTaskCompletion"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\Frozen"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\Frozen"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\Frozen"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\Frozen"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\Frozen"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\Frozen"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\GenericExtendedExecution"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\GenericExtendedExecution"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\GenericExtendedExecution"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\GenericExtendedExecution"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\GenericExtendedExecution"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\GenericExtendedExecution"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\GeofenceTask"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\GeofenceTask"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\GeofenceTask"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\GeofenceTask"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\GeofenceTask"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\GeofenceTask"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\HighPriorityBackgroundAgent"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\HighPriorityBackgroundAgent"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\HighPriorityBackgroundAgent"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\HighPriorityBackgroundAgent"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\HighPriorityBackgroundAgent"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\HighPriorityBackgroundAgent"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\HighPriorityBackgroundDemoted"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\HighPriorityBackgroundDemoted"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\HighPriorityBackgroundDemoted"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\HighPriorityBackgroundDemoted"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\HighPriorityBackgroundDemoted"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\HighPriorityBackgroundDemoted"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\HighPriorityBackgroundTransfer"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\HighPriorityBackgroundTransfer"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\HighPriorityBackgroundTransfer"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\HighPriorityBackgroundTransfer"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\HighPriorityBackgroundTransfer"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\HighPriorityBackgroundTransfer"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\IoTStartupTask"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\IoTStartupTask"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\IoTStartupTask"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\IoTStartupTask"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\IoTStartupTask"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\IoTStartupTask"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\JumboForegroundAgent"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\JumboForegroundAgent"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\JumboForegroundAgent"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\JumboForegroundAgent"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\JumboForegroundAgent"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\JumboForegroundAgent"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaBackgroundTaskCompletion"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaBackgroundTaskCompletion"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaBackgroundTaskCompletion"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaBackgroundTaskCompletion"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaBackgroundTaskCompletion"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaBackgroundTaskCompletion"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaDefaultModernBackgroundTask"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaDefaultModernBackgroundTask"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaDefaultModernBackgroundTask"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaDefaultModernBackgroundTask"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaDefaultModernBackgroundTask"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaDefaultModernBackgroundTask"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaPrelaunchForeground"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaPrelaunchForeground"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaPrelaunchForeground"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaPrelaunchForeground"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaPrelaunchForeground"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaPrelaunchForeground"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiDebugModeForeground"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiDebugModeForeground"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiDebugModeForeground"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiDebugModeForeground"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiDebugModeForeground"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiDebugModeForeground"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiFrozen"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiFrozen"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiFrozen"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiFrozen"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiFrozen"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiFrozen"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiFrozenDNCS"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiFrozenDNCS"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiFrozenDNCS"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiFrozenDNCS"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiFrozenDNCS"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiFrozenDNCS"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiFrozenDNK"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiFrozenDNK"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiFrozenDNK"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiFrozenDNK"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiFrozenDNK"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiFrozenDNK"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiFrozenHighPriority"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiFrozenHighPriority"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiFrozenHighPriority"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiFrozenHighPriority"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiFrozenHighPriority"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiFrozenHighPriority"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiModernForeground"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiModernForeground"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiModernForeground"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiModernForeground"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiModernForeground"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiModernForeground"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiModernForegroundLarge"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiModernForegroundLarge"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiModernForegroundLarge"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiModernForegroundLarge"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiModernForegroundLarge"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiModernForegroundLarge"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiPaused"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiPaused"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiPaused"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiPaused"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiPaused"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiPaused"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiPausedDNK"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiPausedDNK"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiPausedDNK"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiPausedDNK"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiPausedDNK"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiPausedDNK"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiPausedHighPriority"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiPausedHighPriority"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiPausedHighPriority"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiPausedHighPriority"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiPausedHighPriority"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiPausedHighPriority"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiPausing"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiPausing"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiPausing"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiPausing"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiPausing"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LmaUiPausing"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LongRunningBluetooth"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LongRunningBluetooth"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LongRunningBluetooth"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LongRunningBluetooth"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LongRunningBluetooth"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LongRunningBluetooth"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LongRunningControlChannel"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LongRunningControlChannel"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LongRunningControlChannel"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LongRunningControlChannel"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LongRunningControlChannel"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LongRunningControlChannel"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LongRunningSensor"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LongRunningSensor"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LongRunningSensor"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LongRunningSensor"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LongRunningSensor"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\LongRunningSensor"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\MediaProcessing"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\MediaProcessing"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\MediaProcessing"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\MediaProcessing"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\MediaProcessing"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\MediaProcessing"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\OemBackgroundAgent"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\OemBackgroundAgent"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\OemBackgroundAgent"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\OemBackgroundAgent"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\OemBackgroundAgent"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\OemBackgroundAgent"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\OemTask"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\OemTask"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\OemTask"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\OemTask"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\OemTask"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\OemTask"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\PendingDefaultPPLE"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\PendingDefaultPPLE"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\PendingDefaultPPLE"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\PendingDefaultPPLE"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\PendingDefaultPPLE"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\PendingDefaultPPLE"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\PiP"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\PiP"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\PiP"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\PiP"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\PiP"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\PiP"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\PreinstallTask"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\PreinstallTask"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\PreinstallTask"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\PreinstallTask"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\PreinstallTask"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\PreinstallTask"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\PrelaunchForeground"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\PrelaunchForeground"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\PrelaunchForeground"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\PrelaunchForeground"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\PrelaunchForeground"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\PrelaunchForeground"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\PushTriggerTask"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\PushTriggerTask"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\PushTriggerTask"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\PushTriggerTask"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\PushTriggerTask"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\PushTriggerTask"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ResourceIntensive"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ResourceIntensive"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ResourceIntensive"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ResourceIntensive"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ResourceIntensive"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ResourceIntensive"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ShareDataPackageHost"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ShareDataPackageHost"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ShareDataPackageHost"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ShareDataPackageHost"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ShareDataPackageHost"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ShareDataPackageHost"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ShortRunningBluetooth"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ShortRunningBluetooth"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ShortRunningBluetooth"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ShortRunningBluetooth"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ShortRunningBluetooth"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\ShortRunningBluetooth"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\TaskCompletionHighPriority"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\TaskCompletionHighPriority"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\TaskCompletionHighPriority"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\TaskCompletionHighPriority"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\TaskCompletionHighPriority"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\TaskCompletionHighPriority"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiComposer"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiComposer"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiComposer"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiComposer"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiComposer"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiComposer"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiDebugModeForeground"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiDebugModeForeground"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiDebugModeForeground"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiDebugModeForeground"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiDebugModeForeground"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiDebugModeForeground"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiForegroundDNK"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiForegroundDNK"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiForegroundDNK"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiForegroundDNK"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiForegroundDNK"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiForegroundDNK"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiFrozen"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiFrozen"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiFrozen"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiFrozen"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiFrozen"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiFrozen"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiFrozenDNCS"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiFrozenDNCS"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiFrozenDNCS"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiFrozenDNCS"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiFrozenDNCS"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiFrozenDNCS"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiFrozenDNK"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiFrozenDNK"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiFrozenDNK"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiFrozenDNK"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiFrozenDNK"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiFrozenDNK"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiFrozenHighPriority"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiFrozenHighPriority"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiFrozenHighPriority"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiFrozenHighPriority"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiFrozenHighPriority"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiFrozenHighPriority"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiLockScreen"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiLockScreen"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiLockScreen"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiLockScreen"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiLockScreen"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiLockScreen"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiModernForeground"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiModernForeground"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiModernForeground"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiModernForeground"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiModernForeground"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiModernForeground"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiModernForegroundExtended"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiModernForegroundExtended"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiModernForegroundExtended"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiModernForegroundExtended"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiModernForegroundExtended"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiModernForegroundExtended"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiModernForegroundLarge"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiModernForegroundLarge"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiModernForegroundLarge"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiModernForegroundLarge"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiModernForegroundLarge"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiModernForegroundLarge"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiOverlay"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiOverlay"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiOverlay"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiOverlay"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiOverlay"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiOverlay"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiPaused"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiPaused"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiPaused"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiPaused"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiPaused"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiPaused"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiPausedDNK"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiPausedDNK"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiPausedDNK"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiPausedDNK"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiPausedDNK"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiPausedDNK"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiPausedHighPriority"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiPausedHighPriority"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiPausedHighPriority"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiPausedHighPriority"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiPausedHighPriority"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiPausedHighPriority"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiPausing"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiPausing"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiPausing"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiPausing"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiPausing"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiPausing"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiPausingLowPriority"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiPausingLowPriority"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiPausingLowPriority"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiPausingLowPriority"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiPausingLowPriority"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiPausingLowPriority"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiShellCustom1"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiShellCustom1"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiShellCustom1"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiShellCustom1"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiShellCustom1"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiShellCustom1"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiShellCustom2"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiShellCustom2"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiShellCustom2"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiShellCustom2"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiShellCustom2"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiShellCustom2"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiShellCustom3"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiShellCustom3"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiShellCustom3"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiShellCustom3"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiShellCustom3"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiShellCustom3"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiShellCustom4"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiShellCustom4"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiShellCustom4"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiShellCustom4"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiShellCustom4"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\UiShellCustom4"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VideoTranscoding"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VideoTranscoding"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VideoTranscoding"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VideoTranscoding"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VideoTranscoding"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VideoTranscoding"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VoipActiveCallBackground"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VoipActiveCallBackground"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VoipActiveCallBackground"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VoipActiveCallBackground"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VoipActiveCallBackground"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VoipActiveCallBackground"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VoipActiveCallBackgroundPriority"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VoipActiveCallBackgroundPriority"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VoipActiveCallBackgroundPriority"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VoipActiveCallBackgroundPriority"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VoipActiveCallBackgroundPriority"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VoipActiveCallBackgroundPriority"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VoipActiveCallForeground"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VoipActiveCallForeground"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VoipActiveCallForeground"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VoipActiveCallForeground"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VoipActiveCallForeground"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VoipActiveCallForeground"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VoipForegroundWorker"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VoipForegroundWorker"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VoipForegroundWorker"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VoipForegroundWorker"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VoipForegroundWorker"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VoipForegroundWorker"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VoipSuspendedBackground"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VoipSuspendedBackground"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VoipSuspendedBackground"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VoipSuspendedBackground"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VoipSuspendedBackground"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VoipSuspendedBackground"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VoipWorker"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VoipWorker"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VoipWorker"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VoipWorker"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VoipWorker"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\VoipWorker"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\Vpn"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\Vpn"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\Vpn"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\Vpn"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\Vpn"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\Vpn"), QStringLiteral("Memory"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\WebAuthSignIn"), QStringLiteral("CPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\WebAuthSignIn"), QStringLiteral("ExternalResources"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\WebAuthSignIn"), QStringLiteral("Flags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\WebAuthSignIn"), QStringLiteral("Importance"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\WebAuthSignIn"), QStringLiteral("IO"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\WebAuthSignIn"), QStringLiteral("Memory"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.power.cpu_idle_disable");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Disable CPU idle states");
        t.description = QStringLiteral("Disables processor idle states via PowerWriteACValueIndex on the active power scheme. Uses processor idle disable GUID {5d76a2ca-e8c0-402f-a133-2158492d58ad} under the GUID_PROCESSOR_SETTINGS_SUBGROUP. Forces CPU to never enter idle constant 100% power state.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("CPU never enters idle; power draw, heat and laptop fan noise stay at full-load levels constantly"),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("powercfg.exe /SETACVALUEINDEX SCHEME_CURRENT SUB_PROCESSOR 5d76a2ca-e8c0-402f-a133-2158492d58ad 1") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("powercfg.exe /SETACVALUEINDEX SCHEME_CURRENT SUB_PROCESSOR 5d76a2ca-e8c0-402f-a133-2158492d58ad 0") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }
}

} // namespace vth::tweaks::detail
