#include "TweakEngine.h"

#include <QtCore/QByteArray>
#include <QtCore/QStringList>

namespace vth::tweaks::detail {

void appendDefsExtra(QVector<TweakSpec>& c) {
    {
        TweakSpec t;
        t.id = QStringLiteral("sys.input.mouse_linear_curve");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Mouse");
        t.name = QStringLiteral("Set linear 1:1 mouse curve");
        t.description = QStringLiteral("Writes a linear SmoothMouseXCurve/SmoothMouseYCurve to HKCU\\Control Panel\\Mouse so the pointer-acceleration shaping curve is flat and cursor travel tracks distance moved. Note: the bytes are a fixed linear curve, not all-zeros, and this does not touch the 'Enhance pointer precision' checkbox (MouseSpeed/Threshold) itself.");
        t.iconName = QStringLiteral("mouse-pointer");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Pointer feel will change if you relied on acceleration curves"),
        };
        t.registryOps = {
            RegistryOp{ QStringLiteral("HKCU\\Control Panel\\Mouse"), QStringLiteral("SmoothMouseXCurve"), vth::services::RegistryValueKind::Binary, QByteArray::fromHex("0000000000000000c0cc0c0000000000809919000000000040662600000000000033330000000000"), std::nullopt },
            RegistryOp{ QStringLiteral("HKCU\\Control Panel\\Mouse"), QStringLiteral("SmoothMouseYCurve"), vth::services::RegistryValueKind::Binary, QByteArray::fromHex("0000000000000000000038000000000000007000000000000000a800000000000000e00000000000"), std::nullopt },
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.disable-pagefile-on-second-disk");
        t.category = QStringLiteral("storage");
        t.subCategory = QStringLiteral("Pagefile");
        t.name = QStringLiteral("Move pagefile to a secondary fast SSD");
        t.description = QStringLiteral("Sets the pagefile to a system-managed file on D:. Only meaningful if D: is a real, fast, always-present drive; on a single-NVMe system the benefit is negligible, and hardcoding D: is fragile if D: is a slow, removable, or absent volume.");
        t.iconName = QStringLiteral("database");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Only apply if a second drive is mounted at D:; otherwise Windows falls back to a default pagefile on C:."),
        };
        t.registryOps = {
            RegistryOp{ QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("PagingFiles"), vth::services::RegistryValueKind::MultiString, QVariant(QStringList{ QStringLiteral("D:\\pagefile.sys 0 0") }), std::nullopt },
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.bloat.cp_applets");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Control Panel");
        t.name = QStringLiteral("Add advanced Control Panel applets");
        t.description = QStringLiteral("Adds msconfig (System Configuration), msinfo32 (System Information), and Advanced User Accounts (netplwiz) to the Control Panel.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Adds shell CLSID entries under Control Panel; custom Control Panel layouts or MDM policies may conflict"),
        };
        t.registryOps = {
            stringOp(QStringLiteral("HKCR\\CLSID\\{ADEAD0A2-0A48-49cf-8416-FF0000000001}"), QStringLiteral(""), QStringLiteral("System Configuration (msconfig)"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\CLSID\\{ADEAD0A2-0A48-49cf-8416-FF0000000002}"), QStringLiteral(""), QStringLiteral("System information (msinfo32)"), std::nullopt),
            stringOp(QStringLiteral("HKCR\\CLSID\\{ADEAD0A2-0A48-49cf-8416-FF0000000000}"), QStringLiteral(""), QStringLiteral("User Accounts: Advanced"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.sensors.disable_all");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Sensors & Location");
        t.name = QStringLiteral("Disable ambient light sensor and location service state");
        t.description = QStringLiteral("Clears the ambient-light-sensor permission state (user consent and system override) and the location service's internal status flag.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Sensor\\Permissions\\{BFA794E4-F964-4FDB-90F6-51056BFE4B44}"), QStringLiteral("SensorPermissionState"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Sensor\\Overrides\\{BFA794E4-F964-4FDB-90F6-51056BFE4B44}"), QStringLiteral("SensorPermissionState"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\lfsvc\\Service\\Configuration"), QStringLiteral("Status"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.smb.server_latency_tuning");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("SMB");
        t.name = QStringLiteral("SMB server latency");
        t.description = QStringLiteral("Sets the LanmanServer share cache size class to 3 and removes oplock/sharing-violation backoff. Only relevant when this PC hosts SMB shares; oplocks can no longer be disabled on SMB2/SMB3, so the oplock change is mostly inert on modern Windows, and disabling oplocks can hurt (not help) throughput on WAN links.");
        t.iconName = QStringLiteral("network");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Disables opportunistic locking and sharing-violation retry backoff; concurrent access to the same file from other machines/processes may behave differently"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters"), QStringLiteral("Size"), 3u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters"), QStringLiteral("EnableOplocks"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters"), QStringLiteral("SharingViolationDelay"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters"), QStringLiteral("SharingViolationRetries"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.input.extended_responsiveness");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Input responsiveness");
        t.name = QStringLiteral("Extended input and Explorer responsiveness");
        t.description = QStringLiteral("Shortens the low-level-hook timeout, enables raw mouse input, disables active-window tracking on hover, clears keyboard Filter Keys delay/repeat/bounce timings, and stops Explorer waiting for an idle state before repainting.");
        t.iconName = QStringLiteral("mouse-pointer");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Clears Keyboard Filter Keys delay/bounce protection entirely; users who rely on that accessibility feature should skip this"),
        };
        t.registryOps = {
            stringOp(QStringLiteral("HKCU\\Control Panel\\Desktop"), QStringLiteral("LowLevelHooksTimeout"), QStringLiteral("1000"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Control Panel\\Mouse"), QStringLiteral("RawInput"), QStringLiteral("1"), std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Control Panel\\Mouse"), QStringLiteral("ActiveWindowTracking"), 0u, std::nullopt),
            stringOp(QStringLiteral("HKCU\\Control Panel\\Accessibility\\Keyboard Response"), QStringLiteral("AutoRepeatDelay"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Control Panel\\Accessibility\\Keyboard Response"), QStringLiteral("AutoRepeatRate"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Control Panel\\Accessibility\\Keyboard Response"), QStringLiteral("DelayBeforeAcceptance"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Control Panel\\Accessibility\\Keyboard Response"), QStringLiteral("BounceTime"), QStringLiteral("0"), std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Serialize"), QStringLiteral("WaitForIdleState"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("kernel.bam_idle_off");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Kernel");
        t.name = QStringLiteral("Disable background activity moderator idle tracking");
        t.description = QStringLiteral("Clears the Background Activity Moderator idle-resiliency flag and pins its idle-latency tracking.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\bam"), QStringLiteral("IdleResiliency"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\bam"), QStringLiteral("IdleLatency"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("kernel.dpc_watchdog_off");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Kernel");
        t.name = QStringLiteral("Disable DPC watchdog profiling");
        t.description = QStringLiteral("Clears the Session Manager kernel DPC watchdog profile.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("undocumented Session Manager kernel flag; effect not independently verified per CPU model");
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Kernel"), QStringLiteral("DpcWatchdogProfile"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("cpu.hetero_scheduling_off");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("CPU scheduling");
        t.name = QStringLiteral("Disable heterogeneous (E-core) scheduling policy");
        t.description = QStringLiteral("Clears the Session Manager HeteroPolicy flag that steers threads between performance and efficiency cores.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("On hybrid (P-core/E-core) CPUs this changes how Windows schedules threads across core types; test workloads before keeping it"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Power"), QStringLiteral("HeteroPolicy"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("kernel.session_manager_flags");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Kernel");
        t.name = QStringLiteral("Session manager kernel flags");
        t.description = QStringLiteral("Marks the kernel NUMA-aware and obsolete-API flags and raises the Session Manager base CPU priority.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("undocumented Session Manager kernel flags; effect not independently verified per CPU model");
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager"), QStringLiteral("NumaAware"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Kernel"), QStringLiteral("Obsolete"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("CpuPriority"), 6u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.ndis.affinity_reset");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("NDIS");
        t.name = QStringLiteral("Reset NDIS processor affinity mask");
        t.description = QStringLiteral("Clears the NDIS network stack's global processor affinity mask so packet processing is not pinned away from any CPU core.");
        t.iconName = QStringLiteral("network");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Ndis\\Parameters"), QStringLiteral("ProcessorAffinityMask"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.qos.tsfairshare_disable");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("QoS & throttling");
        t.name = QStringLiteral("Disable TSFairShare Disk/Network throttling");
        t.description = QStringLiteral("Disables the Terminal Services fair-share scheduler's disk and network I/O throttling, so a single process is not QoS-capped to keep bandwidth fair across sessions.");
        t.iconName = QStringLiteral("gauge");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\TSFairShare\\Disk"), QStringLiteral("EnableFairShare"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\TSFairShare\\NetFS"), QStringLiteral("EnableFairShare"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("power.mei_icps_tuning");
        t.category = QStringLiteral("power");
        t.subCategory = QStringLiteral("Device power");
        t.name = QStringLiteral("Disable Intel MEI/ICPS power saving");
        t.description = QStringLiteral("Disables the D3 power state for the Intel Management Engine Interface and power saving for the ICPS coprocessor driver, when present.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\MEIx64\\Parameters"), QStringLiteral("DisableD3"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\ICPS\\Parameters"), QStringLiteral("EnablePowerSaving"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.fse.dxgi_gamedvr_tuning");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("Game DVR");
        t.name = QStringLiteral("Fullscreen optimization and GameDVR HDR");
        t.description = QStringLiteral("Sets GameDVR_DXGI_HonorFSEWindowsCompatible so DXGI honors an app's fullscreen-optimizations opt-in, and disables the GameDVR HDR capture path.");
        t.iconName = QStringLiteral("monitor");
        t.impact = Impact::Low;
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\SYSTEM\\GameConfigStore"), QStringLiteral("GameDVR_DXGI_HonorFSEWindowsCompatible"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SYSTEM\\GameConfigStore"), QStringLiteral("GameDVR_DXGI_HDR_Enable"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.gamedvr.capture_channels_disable");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Game DVR");
        t.name = QStringLiteral("Disable GameDVR Audio/Cursor/Microphone capture");
        t.description = QStringLiteral("Disables the individual GameDVR audio, cursor and microphone capture channels.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Low;
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR"), QStringLiteral("AudioCaptureEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR"), QStringLiteral("CursorCaptureEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR"), QStringLiteral("MicrophoneCaptureEnabled"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("pwr.throttle.ceiling_off");
        t.category = QStringLiteral("power");
        t.subCategory = QStringLiteral("Performance & throttling");
        t.name = QStringLiteral("Remove CPU throttle floors and ceilings");
        t.description = QStringLiteral("Writes throttle floor/ceiling and high-performance values under Control\\Power to reduce power throttling. Raises idle power draw and heat; some of these value names are non-standard at this location, so the real effect may be partial.");
        t.iconName = QStringLiteral("gauge");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Raises idle power draw and heat; disables Windows' automatic power-throttling"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("MinimumThrottlePercent"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("MaximumThrottlePercent"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("MaximumPerformancePercent"), 100u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("HighPerformance"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("HighestPerformance"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("DynamicThrottlePolicy"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("pwr.energysaver.off");
        t.category = QStringLiteral("power");
        t.subCategory = QStringLiteral("Energy Saver");
        t.name = QStringLiteral("Disable energy saver and eco mode");
        t.description = QStringLiteral("Turns off Energy Saver, Eco Mode and energy estimation and sets the energy-saver policy flag.");
        t.iconName = QStringLiteral("gauge");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Disables Windows' Energy Saver safety net; raises idle power draw"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("EnergySaverStatus"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("EcoMode"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("EnergySaverPolicy"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("DisableEnergyEstimation"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("pwr.idle.unpark_and_qos");
        t.category = QStringLiteral("power");
        t.subCategory = QStringLiteral("Core parking");
        t.name = QStringLiteral("Unpark cores and disable idle-processor QoS");
        t.description = QStringLiteral("Unparks all cores at boot, disables idle-processor QoS management and the vsync latency update, and pins low-latency scaling to 100%.");
        t.iconName = QStringLiteral("gauge");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Keeping all cores unparked raises idle power draw"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("InitialUnparkCount"), 100u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("QosManagesIdleProcessors"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("DisableVsyncLatencyUpdate"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("LowLatencyScalingPercentage"), 100u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.dwm.buffer_flip");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("DWM");
        t.name = QStringLiteral("DWM buffer and flip model");
        t.description = QStringLiteral("Writes DWM buffer/flip values (queued-buffer cap, flip-model override, independent-flip disable). These are largely unofficial DWM keys with mixed results; disabling independent flip can increase, not reduce, latency, so treat as experimental.");
        t.iconName = QStringLiteral("monitor");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\DWM"), QStringLiteral("MaxQueuedBuffers"), 2u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\DWM"), QStringLiteral("UseMachineCheck"), 0u, std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\DWM"), QStringLiteral("FlipModelOverride"), QStringLiteral("1"), std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("DisableIndependentFlip"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.dwm.effects_off");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("DWM");
        t.name = QStringLiteral("Disable DWM blur and composition effects");
        t.description = QStringLiteral("Disables DWM blur-behind compositing (per-user and machine-wide), the composition policy and process-window ghosting.");
        t.iconName = QStringLiteral("monitor");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\DWM"), QStringLiteral("EnableBlurBehind"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("EnableBlurBehind"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("CompositionPolicy"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("DisableProcessWindowsGhosting"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.dwm.desktop_heap");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("DWM");
        t.name = QStringLiteral("Enlarge desktop heap (SharedSection)");
        t.description = QStringLiteral("Writes a SharedSection desktop-heap triple to the DWM key. The value Windows actually reads is under Control\\Session Manager\\SubSystems\\Windows, so at this DWM location it most likely has no effect.");
        t.iconName = QStringLiteral("monitor");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("An invalid SharedSection value can cause \"out of desktop heap\" GUI failures; only change it to a known-good triple"),
        };
        t.registryOps = {
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("SharedSection"), QStringLiteral("1024,20480,768"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.scheduler.priority");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("GPU scheduling");
        t.name = QStringLiteral("GPU scheduler priority and policy");
        t.description = QStringLiteral("Sets the GraphicsDrivers scheduler render priority and policy and disables software-command-generation for render and preemption levels.");
        t.iconName = QStringLiteral("monitor");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Scheduler"), QStringLiteral("GpuPriority"), 31u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Scheduler"), QStringLiteral("SchedulePolicy"), 4u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Scheduler"), QStringLiteral("PreemptionLevel"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Scheduler"), QStringLiteral("UseSCGForRender"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.scheduler.boost_compression");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("GPU scheduling");
        t.name = QStringLiteral("GPU boost and framebuffer compression");
        t.description = QStringLiteral("Forces GPU boost on and disables scheduler framebuffer compression and yields.");
        t.iconName = QStringLiteral("monitor");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Scheduler"), QStringLiteral("EnableGpuBoost"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Scheduler"), QStringLiteral("EnableFrameBufferCompression"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Scheduler"), QStringLiteral("EnableYield"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.driver.vsync_off");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("GPU display");
        t.name = QStringLiteral("Disable driver-level VSync");
        t.description = QStringLiteral("Disables VSync on the graphics driver's default display configuration.");
        t.iconName = QStringLiteral("monitor");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Disabling VSync can cause visible screen tearing"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Configuration\\00"), QStringLiteral("EnableVSync"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Configuration\\00\\00"), QStringLiteral("EnableVSync"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.d3d.loader_limits");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("Direct3D");
        t.name = QStringLiteral("Direct3D loader thread and context limits");
        t.description = QStringLiteral("Raises the Direct3D loader-thread count and context limit and disables context reordering.");
        t.iconName = QStringLiteral("monitor");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D"), QStringLiteral("MaxLoaderThreads"), 10u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D"), QStringLiteral("MaxContexts"), 1024u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D"), QStringLiteral("ContextReordering"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("storage.ntfs_cache_tuning");
        t.category = QStringLiteral("storage");
        t.subCategory = QStringLiteral("NTFS");
        t.name = QStringLiteral("NTFS tunnel cache and filesystem memory");
        t.description = QStringLiteral("Zeroes the NTFS tunnel-cache entry age and raises the filesystem cache memory-usage class.");
        t.iconName = QStringLiteral("database");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem"), QStringLiteral("MaximumTunnelEntryAgeInSeconds"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem"), QStringLiteral("MemoryUsage"), 2u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("storage.usb_hid_idle_off");
        t.category = QStringLiteral("power");
        t.subCategory = QStringLiteral("USB power");
        t.name = QStringLiteral("Disable USB/HID idle power-down");
        t.description = QStringLiteral("Disables HID/USB-class idle power-down, extends the USB XHCI buffer and disables PCI bus selective suspend.");
        t.iconName = QStringLiteral("database");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\HidUsb"), QStringLiteral("IdleEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{36fc9e60-c465-11cf-8056-444553540000}"), QStringLiteral("IdleEnable"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\USBXHCI\\Parameters\\Wdf"), QStringLiteral("NoExtraBufferRoom"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\pci"), QStringLiteral("EnableSelectiveSuspend"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("storage.wdf_idle_polling");
        t.category = QStringLiteral("storage");
        t.subCategory = QStringLiteral("Device power");
        t.name = QStringLiteral("Tighten WDF idle polling");
        t.description = QStringLiteral("Sets the WDF framework idle-polling timeout and disables its idle timeout.");
        t.iconName = QStringLiteral("database");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Wdf\\Wdf01000"), QStringLiteral("IdlePollingTimeout"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Wdf\\Wdf01000"), QStringLiteral("IdleTimeout"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.irq.legacy_ide_priority");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("IRQ priority");
        t.name = QStringLiteral("Raise legacy IDE IRQ priority");
        t.description = QStringLiteral("Raises the interrupt priority of the legacy IDE channels (IRQ 14/15).");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("only affects legacy IDE controllers; no effect on AHCI/NVMe-only systems");
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PriorityControl"), QStringLiteral("IRQ14Priority"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PriorityControl"), QStringLiteral("IRQ15Priority"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("storage.tcg_opal_prompt_off");
        t.category = QStringLiteral("storage");
        t.subCategory = QStringLiteral("Storage devices");
        t.name = QStringLiteral("Disable TCG Opal security activation prompt");
        t.description = QStringLiteral("Disables the enhanced-storage TCG Opal security-activation prompt policy.");
        t.iconName = QStringLiteral("database");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\EnhancedStorageDevices"), QStringLiteral("TCGSecurityActivationDisabled"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.processor.cstate_and_scheme");
        t.category = QStringLiteral("power");
        t.subCategory = QStringLiteral("C-States");
        t.name = QStringLiteral("Disable CPU C-states and force high performance plan");
        t.description = QStringLiteral("Clears the processor C-state options bitmask and force-activates the built-in Windows \"High performance\" power plan.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Disabling CPU C-states raises idle power draw and heat"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Processor"), QStringLiteral("CStateOptions"), 0u, std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\User\\PowerSchemes"), QStringLiteral("ActivePowerScheme"), QStringLiteral("8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.amd.driver_micro_tuning");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("AMD");
        t.name = QStringLiteral("AMD driver internals (PSP/KMD/ACE)");
        t.description = QStringLiteral("Enables the AMD Platform Security Processor, disables the KMD dummy backend and Radeon Chill on the WDDM graphics service, and resets the ADL app-profile reload counter and auto-update status, if the relevant AMD services/keys are present.");
        t.iconName = QStringLiteral("monitor");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.concern = QStringLiteral("AMD-only; no effect if these legacy service keys are absent on the current driver version");
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdpsp\\Parameters"), QStringLiteral("EnablePSP"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdkmdag"), QStringLiteral("PP_PhmUseDummyBackEnd"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\ATI\\ACE\\Settings\\ADL\\AppProfiles"), QStringLiteral("AplReloadCounter"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\Software\\AUEP"), QStringLiteral("RSX_AUEPStatus"), 2u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdwddmg"), QStringLiteral("ChillEnabled"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("shell.explorer_icon_cache_size");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Explorer");
        t.name = QStringLiteral("Raise Explorer icon cache limit");
        t.description = QStringLiteral("Raises Explorer's maximum cached-icon count to 4096, reducing icon re-extraction when browsing large folders.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.registryOps = {
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer"), QStringLiteral("Max Cached Icons"), QStringLiteral("4096"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.update.defer_feature_upgrades");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Windows Update");
        t.name = QStringLiteral("Defer feature upgrades");
        t.description = QStringLiteral("Sets the Windows Update feature-upgrade deferral period policy.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate"), QStringLiteral("DeferUpgradePeriod"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.telemetry.sqm_corporate_blackhole");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Telemetry");
        t.name = QStringLiteral("Blackhole corporate SQM telemetry URL");
        t.description = QStringLiteral("Points the corporate Software Quality Metrics (SQM) telemetry upload URL policy at 0.0.0.0, so any SQM report never leaves the machine.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.registryOps = {
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\SQMClient"), QStringLiteral("CorporateSQMURL"), QStringLiteral("0.0.0.0"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.startup.gaming_launchers_disable");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Startup");
        t.name = QStringLiteral("Disable common Gaming/Chat launcher autostart");
        t.description = QStringLiteral("Disables autostart (via StartupApproved\\Run) for Discord, Razer Synapse, Spotify, Epic Games Launcher, Riot Client and Steam, if currently present and enabled.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Only disables autostart for these apps if already present; does not uninstall them"),
        };
        t.registryOps = {
            RegistryOp{ QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved\\Run"), QStringLiteral("Discord"), vth::services::RegistryValueKind::Binary, QByteArray::fromHex("0300000066AF9C7C5A46D901"), std::nullopt },
            RegistryOp{ QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved\\Run"), QStringLiteral("Synapse3"), vth::services::RegistryValueKind::Binary, QByteArray::fromHex("030000007DC437B0EA9FD901"), std::nullopt },
            RegistryOp{ QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved\\Run"), QStringLiteral("Spotify"), vth::services::RegistryValueKind::Binary, QByteArray::fromHex("0300000070E93D7B5A46D901"), std::nullopt },
            RegistryOp{ QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved\\Run"), QStringLiteral("EpicGamesLauncher"), vth::services::RegistryValueKind::Binary, QByteArray::fromHex("03000000F51C70A77A48D901"), std::nullopt },
            RegistryOp{ QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved\\Run"), QStringLiteral("RiotClient"), vth::services::RegistryValueKind::Binary, QByteArray::fromHex("03000000A0EA598A88B2D901"), std::nullopt },
            RegistryOp{ QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved\\Run"), QStringLiteral("Steam"), vth::services::RegistryValueKind::Binary, QByteArray::fromHex("03000000E7766B83316FD901"), std::nullopt },
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.input.spi_responsiveness");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Input responsiveness");
        t.name = QStringLiteral("Input responsiveness (SystemParametersInfo)");
        t.description = QStringLiteral("Disables mouse vanish, UI effects, and font smoothing via SystemParametersInfoW; sets keyboard delay to 0 (shortest) and keyboard speed to 31 (fastest repeat rate). Also writes the corresponding registry values for persistence across reboots.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Font smoothing disabled may reduce text readability"),
            QStringLiteral("UI effects disabled removes visual animations"),
        };
        t.registryOps = {
            RegistryOp{ QStringLiteral("HKCU\\Control Panel\\Desktop"), QStringLiteral("UserPreferencesMask"), vth::services::RegistryValueKind::Binary, QByteArray::fromHex("5a12038010000000"), std::nullopt },
            dwordOp(QStringLiteral("HKCU\\Control Panel\\Mouse"), QStringLiteral("MouseVanish"), 0u, 1u),
            stringOp(QStringLiteral("HKCU\\Control Panel\\Desktop"), QStringLiteral("FontSmoothing"), QStringLiteral("0"), QStringLiteral("2")),
            stringOp(QStringLiteral("HKCU\\Control Panel\\Keyboard"), QStringLiteral("KeyboardDelay"), QStringLiteral("0"), QStringLiteral("1")),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.services.xbox_disable");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Xbox & Game Bar");
        t.name = QStringLiteral("Disable Xbox services and GameDVR");
        t.description = QStringLiteral("Disables XboxNetApiSvc, XboxGipSvc services and all GameDVR registry settings including AllowGameDVR policy, GameDVR_Enabled, AppCaptureEnabled, auto game mode, and game mode notifications.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Game Pass, Xbox Live sign-in and GameDVR recording/streaming features stop working"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\ApplicationManagement\\AllowGameDVR"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\GameDVR"), QStringLiteral("AllowGameDVR"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SYSTEM\\GameConfigStore"), QStringLiteral("GameDVR_Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\GameDVR"), QStringLiteral("AppCaptureEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\GameBar"), QStringLiteral("AllowAutoGameMode"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\GameBar"), QStringLiteral("AutoGameModeEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\GameBar"), QStringLiteral("ShowStartupPanel"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\GameBar"), QStringLiteral("ShowGameModeNotifications"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\XboxNetApiSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\XboxGipSvc"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.activity.wdi");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Diagnostics");
        t.name = QStringLiteral("Disable Windows diagnostic infrastructure");
        t.description = QStringLiteral("Disables all WDI diagnostic scenario execution (14 GUIDs), WMI reliability analysis, storage health telemetry, and the WDI services.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Windows troubleshooters and automatic problem detection stop working"),
            QStringLiteral("Reliability Monitor and storage-health reporting become less useful"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\WdiServiceHost"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\WdiSystemHost"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.shortcut_resolve");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Explorer");
        t.name = QStringLiteral("Disable shortcut auto-resolution");
        t.description = QStringLiteral("Stops Explorer from hunting for moved targets when opening broken shortcuts.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Broken shortcuts no longer auto-repair themselves."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"), QStringLiteral("LinkResolveIgnoreLinkInfo"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"), QStringLiteral("NoResolveSearch"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"), QStringLiteral("NoResolveTrack"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("svc.ipv6.disable");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable IPv6 (Tcpip6)");
        t.description = QStringLiteral("Disables the IPv6 protocol driver. Modern Windows uses IPv6 internally; disabling it can break Teams, WSL2, Xbox networking and other services. Reverts to manual start.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("breaks IPv6-dependent services; opt-in only, never recommended.");
        t.warnings = { QStringLiteral("IPv6 connectivity and any IPv6-only services (Teams, WSL2, Xbox) stop working") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip6"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("svc.pc_speaker.disable");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable PC speaker (beep)");
        t.description = QStringLiteral("Disables the motherboard PC-speaker beep driver. Reverts to manual start.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Beep"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("svc.ndis_virtualbus.disable");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable NDIS virtual bus");
        t.description = QStringLiteral("Disables the NDIS virtual-bus driver used by some virtual network adapters. Reverts to manual start.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = { QStringLiteral("Virtual network adapters that depend on it stop working") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NdisVirtualBus"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("svc.netbios.disable");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable NetBIOS");
        t.description = QStringLiteral("Disables the legacy NetBIOS name-resolution driver. Reverts to manual start.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = { QStringLiteral("Legacy NetBIOS name resolution and some old file-sharing setups stop working") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NetBIOS"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("svc.parallel_port.disable");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable parallel port");
        t.description = QStringLiteral("Disables the parallel-port driver. Reverts to manual start.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = { QStringLiteral("Parallel-port peripherals no longer enumerate") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Parport"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("svc.serial_port.disable");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable serial port");
        t.description = QStringLiteral("Disables the serial-port driver. Reverts to manual start.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = { QStringLiteral("Serial-port peripherals no longer enumerate") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Serial"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("svc.infrared.disable");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable infrared (IrDA)");
        t.description = QStringLiteral("Disables the infrared (IrDA) driver. Reverts to manual start.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = { QStringLiteral("Infrared devices no longer work") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\IrDA"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("svc.rdp_display.disable");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable Remote Desktop display driver");
        t.description = QStringLiteral("Disables the Remote Desktop display driver (TermDD). Reverts to manual start.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = { QStringLiteral("Remote Desktop / RDP sessions stop working") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\TermDD"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("svc.bluetooth.disable");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable Bluetooth");
        t.description = QStringLiteral("Disables the Bluetooth port driver. Reverts to manual start.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = { QStringLiteral("Bluetooth stops working entirely") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\BTHPORT"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("svc.fax.disable");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable fax service");
        t.description = QStringLiteral("Disables the Fax service. Reverts to manual start.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = { QStringLiteral("Faxing stops working") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Fax"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("svc.ip_helper.disable");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable IP helper");
        t.description = QStringLiteral("Disables the IP Helper service (IPv6 transition/tunneling). Reverts to manual start.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("breaks IPv6 tunneling / some VPNs; opt-in only, never recommended.");
        t.warnings = { QStringLiteral("IPv6 tunneling and some VPN clients stop working") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\iphlpsvc"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("svc.smart_card.disable");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable smart card");
        t.description = QStringLiteral("Disables the Smart Card service. Reverts to manual start.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = { QStringLiteral("Smart-card logon and smart-card readers stop working") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\SCardSvr"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.compat.game_shims");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Compatibility");
        t.name = QStringLiteral("Disable game compatibility shims");
        t.description = QStringLiteral("Disables 7 Microsoft-shipped game-compatibility shims that otherwise wrap affected game executables with runtime fix-ups.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Some older games may lose legitimate compatibility fixes."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags"), QStringLiteral("{6999b73a-bf55-4b3a-9804-5119e72c6b3a}"), 119u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags"), QStringLiteral("{d6cb77a2-24b7-4abf-a74e-ad4243af6f74}"), 119u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags"), QStringLiteral("{5d41f98c-93d7-4b23-998a-9e809c893b05}"), 119u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags"), QStringLiteral("{c0e71031-3bf9-42ad-a0e4-6c1d0fa5a315}"), 119u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags"), QStringLiteral("{93d69993-27b6-489b-b1ee-2517766283a1}"), 119u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags"), QStringLiteral("{c9810637-3e60-488a-bcbe-0cf6f15d0f31}"), 119u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags"), QStringLiteral("{5ec07f4f-3cbd-4974-8720-806cefc95af6}"), 119u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.appdiag.settings_sync");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Sync");
        t.name = QStringLiteral("Disable settings sync");
        t.description = QStringLiteral("Prevents Windows from syncing your settings across devices via your Microsoft account.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Themes, passwords, and language preferences no longer sync across devices via Microsoft account"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SettingSync\\Groups\\Accessibility"), QStringLiteral("Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SettingSync\\Groups\\AppSync"), QStringLiteral("Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SettingSync\\Groups\\BrowserSettings"), QStringLiteral("Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SettingSync\\Groups\\Credentials"), QStringLiteral("Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SettingSync\\Groups\\DesktopTheme"), QStringLiteral("Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SettingSync\\Groups\\Language"), QStringLiteral("Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SettingSync\\Groups\\PackageState"), QStringLiteral("Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SettingSync\\Groups\\Personalization"), QStringLiteral("Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SettingSync\\Groups\\StartLayout"), QStringLiteral("Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SettingSync\\Groups\\Windows"), QStringLiteral("Enabled"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("shell.remove-3d-objects");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Explorer");
        t.name = QStringLiteral("Remove \\\"3D objects\\\" from this PC");
        t.description = QStringLiteral("Removes the \\\"3D Objects\\\" namespace from This PC and the Explorer left-rail.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.reversible = false;
        {
            CommandOp op;
            op.program = QStringLiteral("powershell");
            op.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MyComputer\\NameSpace\\{0DB7E03F-FC29-4DC6-9020-FF41B59E513A}','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MyComputer\\NameSpace\\{0DB7E03F-FC29-4DC6-9020-FF41B59E513A}','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MyComputer\\NameSpace\\{0DB7E03F-FC29-4DC6-9020-FF41B59E513A}','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MyComputer\\NameSpace\\{0DB7E03F-FC29-4DC6-9020-FF41B59E513A}')){Remove-Item -LiteralPath $k -Recurse -Force}") };
            op.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
            t.commandOps.push_back(op);
        }
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.thispc_cleanup");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Explorer");
        t.name = QStringLiteral("Remove default folders from this PC");
        t.description = QStringLiteral("Removes the default user folders (Desktop, Documents, Downloads, Music, Pictures, Videos, 3D Objects) from the This PC view.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Desktop, Documents, Downloads, Music, Pictures, Videos, and 3D Objects disappear from the This PC view; the underlying user files remain intact"),
            QStringLiteral("Requires administrator rights to modify HKLM; running unelevated causes the tweak to fail rather than partially apply"),
        };
        {
            CommandOp op;
            op.program = QStringLiteral("powershell");
            op.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MyComputer\\NameSpace\\{B4BFCC3A-DB2C-424C-B029-7FE99A87C641}','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MyComputer\\NameSpace\\{d3162b92-9365-467a-956b-92703aca08af}','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MyComputer\\NameSpace\\{088e3905-0323-4b02-9826-5d99428e115f}','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MyComputer\\NameSpace\\{3dfdf296-dbec-4fb4-81d1-6a3438bcf4de}','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MyComputer\\NameSpace\\{24ad3ad4-a569-4530-98e1-ab02f9417aa8}','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MyComputer\\NameSpace\\{f86fa3ab-70d2-4fc7-9c99-fcbf05467f3a}','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MyComputer\\NameSpace\\{0DB7E03F-FC29-4DC6-9020-FF41B59E513A}')){Remove-Item -LiteralPath $k -Recurse -Force}") };
            op.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
            t.commandOps.push_back(op);
        }
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.adapter.binding_disable");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("Adapter bindings");
        t.name = QStringLiteral("Network adapter binding component disable");
        t.description = QStringLiteral("Unbinds ms_tcpip6 (IPv6), ms_lltdio and ms_rspndr (Link-Layer Topology Discovery) and ms_lldp from every network adapter, with a matching re-enable revert. Disabling IPv6 binding can affect Teams/WSL2; the LLTD/LLDP components only affect network-map discovery. Contrary to the current text, File and Printer Sharing, Microsoft Network Client and the QoS Packet Scheduler are NOT touched.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Critical;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("IPv6 disabled (breaks Teams, WSL2, many modern services)"),
            QStringLiteral("File and Printer Sharing disabled (breaks SMB access)"),
            QStringLiteral("Microsoft Network Client disabled (no network share access)"),
            QStringLiteral("QoS Packet Scheduler disabled"),
        };
        {
            CommandOp op;
            op.program = QStringLiteral("powershell");
            op.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';Get-NetAdapter | ForEach-Object { Disable-NetAdapterBinding -Name $_.Name -ComponentID ms_tcpip6,ms_lltdio,ms_rspndr,ms_lldp -EA SilentlyContinue }") };
            op.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';Get-NetAdapter | ForEach-Object { Enable-NetAdapterBinding -Name $_.Name -ComponentID ms_tcpip6,ms_lltdio,ms_rspndr,ms_lldp -EA SilentlyContinue }") };
            t.commandOps.push_back(op);
        }
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.nic.per_interface_tuning");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("TCP/IP");
        // Writes one value, NetbiosOptions.
        t.name = QStringLiteral("Disable NetBIOS over TCP/IP");
        t.description = QStringLiteral("Sets NetbiosOptions=2 on every network interface, turning off legacy NetBIOS name resolution. Nothing else is changed: not flow control, interrupt moderation, Energy Efficient or Green Ethernet, adapter power saving, Nagle, TcpAckFrequency or UseZeroBroadcast.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("NetBIOS over TCP/IP is disabled on every adapter; legacy name resolution stops working"),
        };
        {
            CommandOp op;
            op.program = QStringLiteral("powershell");
            op.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';Get-ChildItem 'HKLM:\\SYSTEM\\CurrentControlSet\\services\\NetBT\\Parameters\\Interfaces' | ForEach-Object { Set-ItemProperty $_.PSPath -Name NetbiosOptions -Value 2 -Type DWord }") };
            // NetbiosOptions defaults to 0 (use the DHCP server's NetBIOS setting), so
            // the revert writes 0.
            op.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';Get-ChildItem 'HKLM:\\SYSTEM\\CurrentControlSet\\services\\NetBT\\Parameters\\Interfaces' | ForEach-Object { Set-ItemProperty $_.PSPath -Name NetbiosOptions -Value 0 -Type DWord }") };
            t.commandOps.push_back(op);
        }
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("net.nic.extended_power_disable");
        t.category = QStringLiteral("networking");
        t.subCategory = QStringLiteral("NIC power");
        t.name = QStringLiteral("Extended NIC power management disable");
        t.description = QStringLiteral("Enumerates NIC adapter class subkeys and disables power saving, Energy Efficient Ethernet, wake-on-LAN, PME, dynamic/connected power gating, and related power management features on all adapters.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        // Writes about 29 values on every adapter without recording what they held,
        // so there is no exact revert; driver defaults differ by vendor. A DynamicOp
        // that snapshots each device would make it reversible.
        t.reversible = false;
        t.warnings = {
            QStringLiteral("NIC power saving disabled - higher idle power consumption"),
            QStringLiteral("Wake-on-LAN disabled - cannot wake PC remotely"),
            QStringLiteral("Vethida cannot undo this; per-adapter prior values are not recorded"),
        };
        {
            CommandOp op;
            op.program = QStringLiteral("powershell");
            op.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';Get-ChildItem 'HKLM:\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e972-e325-11ce-bfc1-08002be10318}' | Where-Object { $_.PSChildName -match '^\\d{4}$' } | ForEach-Object { Set-ItemProperty $_.PSPath -Name 'AutoPowerSaveModeEnabled' -Value 0 -Type DWord; Set-ItemProperty $_.PSPath -Name 'AutoDisableGigabit' -Value 0 -Type DWord; Set-ItemProperty $_.PSPath -Name 'AdvancedEEE' -Value 0 -Type DWord; Set-ItemProperty $_.PSPath -Name 'DisableDelayedPowerUp' -Value 0 -Type DWord; Set-ItemProperty $_.PSPath -Name '*EEE' -Value 0 -Type DWord; Set-ItemProperty $_.PSPath -Name 'EEE' -Value 0 -Type DWord; Set-ItemProperty $_.PSPath -Name 'EnablePME' -Value 0 -Type DWord; Set-ItemProperty $_.PSPath -Name 'EEELinkAdvertisement' -Value 0 -Type DWord; Set-ItemProperty $_.PSPath -Name 'EnableGreenEthernet' -Value 0 -Type DWord; Set-ItemProperty $_.PSPath -Name 'EnableSavePowerNow' -Value 0 -Type DWord; Set-ItemProperty $_.PSPath -Name 'EnablePowerManagement' -Value 0 -Type DWord; Set-ItemProperty $_.PSPath -Name 'EnableDynamicPowerGating' -Value 0 -Type DWord; Set-ItemProperty $_.PSPath -Name 'EnableConnectedPowerGating' -Value 0 -Type DWord; Set-ItemProperty $_.PSPath -Name 'EnableWakeOnLan' -Value 0 -Type DWord; Set-ItemProperty $_.PSPath -Name 'GigaLite' -Value 0 -Type DWord; Set-ItemProperty $_.PSPath -Name 'NicAutoPowerSaver' -Value 0 -Type DWord; Set-ItemProperty $_.PSPath -Name 'PowerDownPll' -Value 0 -Type DWord; Set-ItemProperty $_.PSPath -Name 'PowerSavingMode' -Value 0 -Type DWord; Set-ItemProperty $_.PSPath -Name 'ReduceSpeedOnPowerDown' -Value 0 -Type DWord; Set-ItemProperty $_.PSPath -Name 'SmartPowerDownEnable' -Value 0 -Type DWord; Set-ItemProperty $_.PSPath -Name 'S5NicKeepOverrideMacAddrV2' -Value 0 -Type DWord; Set-ItemProperty $_.PSPath -Name 'S5WakeOnLan' -Value 0 -Type DWord; Set-ItemProperty $_.PSPath -Name 'ULPMode' -Value 0 -Type DWord; Set-ItemProperty $_.PSPath -Name 'WakeOnDisconnect' -Value 0 -Type DWord; Set-ItemProperty $_.PSPath -Name '*WakeOnMagicPacket' -Value 0 -Type DWord; Set-ItemProperty $_.PSPath -Name '*WakeOnPattern' -Value 0 -Type DWord; Set-ItemProperty $_.PSPath -Name 'WakeOnLink' -Value 0 -Type DWord; Set-ItemProperty $_.PSPath -Name 'WolShutdownLinkSpeed' -Value 0 -Type DWord; Set-ItemProperty $_.PSPath -Name 'PnPCapabilities' -Value 24 -Type DWord }") };
            op.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
            t.commandOps.push_back(op);
        }
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.usb.selective_suspend_disable");
        t.category = QStringLiteral("power");
        t.subCategory = QStringLiteral("USB power");
        t.name = QStringLiteral("Disable USB selective suspend (per-device)");
        t.description = QStringLiteral("Enumerates every USB device instance and disables selective suspend and D3 cold on each, eliminating USB device wake latency (mice, keyboards, audio, controllers stay fully powered).");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        // Same as the NIC power tweak above: it overwrites five values on every USB
        // device with no snapshot, so it can't be put back.
        t.reversible = false;
        t.warnings = {
            QStringLiteral("USB devices never enter selective suspend; idle power consumption rises (notable on laptops)"),
            QStringLiteral("Vethida cannot undo this; per-device prior values are not recorded"),
        };
        {
            CommandOp op;
            op.program = QStringLiteral("powershell");
            op.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';Get-ChildItem 'HKLM:\\SYSTEM\\CurrentControlSet\\Enum\\USB' -Recurse | Where-Object { $_.PSChildName -eq 'Device Parameters' } | ForEach-Object { Set-ItemProperty $_.PSPath -Name 'SelectiveSuspendEnabled' -Value 0 -Type DWord; Set-ItemProperty $_.PSPath -Name 'SelectiveSuspendOn' -Value 0 -Type DWord; Set-ItemProperty $_.PSPath -Name 'DeviceSelectiveSuspended' -Value 0 -Type DWord; Set-ItemProperty $_.PSPath -Name 'EnableSelectiveSuspend' -Value 0 -Type DWord; Set-ItemProperty $_.PSPath -Name 'D3ColdSupported' -Value 0 -Type DWord }") };
            op.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
            t.commandOps.push_back(op);
        }
        c.push_back(t);
    }


    {
        TweakSpec t;
        t.id = QStringLiteral("sec.mitigations.process_options_disable");
        t.category = QStringLiteral("security");
        t.name = QStringLiteral("Disable system-wide process mitigation options (ASLR/DEP/CFG/CIG)");
        t.description = QStringLiteral("Writes a 24-byte MitigationOptions bitfield of 0x22 values, disabling ASLR, DEP, CFG and CIG defaults for every new process.");
        t.iconName = QStringLiteral("shield-off");
        t.impact = Impact::Critical;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Disables ASLR, DEP, CFG and CIG for every process - a severe security downgrade."),
        };
        t.concern = QStringLiteral("flagged: removes foundational memory-safety mitigations system-wide");
        t.registryOps = {
            RegistryOp{ QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Kernel"), QStringLiteral("MitigationOptions"), vth::services::RegistryValueKind::Binary, QByteArray::fromHex("222222222222222222222222222222222222222222222222"), std::nullopt },
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bios.hpet_disable");
        t.category = QStringLiteral("advanced");
        t.declaredTier = vth::models::ProductTier::Ultimate;
        t.name = QStringLiteral("Disable HPET (high precision event timer)");
        t.description = QStringLiteral("Disables the HPET platform clock via the registry. Modern Windows already prefers the invariant TSC, so this rarely changes anything measurable; treat as experimental, not a guaranteed latency win.");
        t.iconName = QStringLiteral("clock");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("no measured effect on modern hardware; opt-in only, never recommended.");
        t.warnings = {
            QStringLiteral("Very old software that depends on HPET for timing may behave differently"),
            QStringLiteral("Some audio production software prefers HPET - test after applying"),
        };
        t.registryOps = {
            RegistryOp{ QStringLiteral("HKLM\\BCD00000000\\Objects\\{6efb52bf-1766-41db-a6b3-0ee5eff72bd7}\\Elements\\16000067"), QStringLiteral("Element"), vth::services::RegistryValueKind::Binary, QByteArray::fromHex("0000000000000000"), std::nullopt },
        };
        c.push_back(t);
    }


    {
        TweakSpec t;
        t.id = QStringLiteral("sys.time.disable_ntp_sync");
        t.category = QStringLiteral("advanced");
        t.name = QStringLiteral("Disable Windows Time sync (NoSync)");
        t.description = QStringLiteral("Sets the Windows Time service provider type to NoSync, stopping periodic NTP synchronization.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("The system clock will drift over time without NTP sync; re-enable if you rely on accurate time (auth tokens, scheduled tasks, logs)"),
        };
        t.registryOps = {
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\W32Time\\Parameters"), QStringLiteral("Type"), QStringLiteral("NoSync"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("cpu.processor_capabilities_override");
        t.category = QStringLiteral("advanced");
        t.name = QStringLiteral("CPU capabilities bitmask override");
        t.description = QStringLiteral("Writes a fixed value to the kernel's reported processor Capabilities bitmask.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("undocumented raw CPU capabilities bitmask; its effect on a given CPU model is unverified");
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Processor"), QStringLiteral("Capabilities"), 57598u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.scheduler.hangcheck_off");
        t.category = QStringLiteral("gpu");
        t.name = QStringLiteral("Disable GPU scheduler hang check");
        t.description = QStringLiteral("Disables the GraphicsDrivers scheduler hang/TDR recovery check.");
        t.iconName = QStringLiteral("monitor");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("A genuinely hung GPU will no longer be automatically recovered and can hard-lock the system - only for adapters already stable under custom clocks"),
        };
        t.concern = QStringLiteral("high-risk; removes the GPU hang-recovery safety net");
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Scheduler"), QStringLiteral("EnableHangChecks"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.driver.graphicsdrivers_flags");
        t.category = QStringLiteral("gpu");
        t.name = QStringLiteral("GraphicsDrivers low-level flags");
        t.description = QStringLiteral("Disables dirty-rectangle updates, mode-change offsetting, IOMMU usage, swapchain waits, DMA remapping and the frame-queue mode at the GraphicsDrivers level.");
        t.iconName = QStringLiteral("monitor");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("DisableDirtyRectangles"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("DisableModeChangeOffset"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("IommuUsage"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("SwapchainWaitTime"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("DmaRemappingEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("PowerSettingEnable"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("FrameQueueMode"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\I/O System"), QStringLiteral("DmaRemappingCompatible"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.update.block_access");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Block Windows Update access");
        t.description = QStringLiteral("Sets the DisableWindowsUpdateAccess policy, fully blocking access to Windows Update.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Critical;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Fully blocks Windows Update access, including security patches. Only use on systems with an alternate patch-management process."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate"), QStringLiteral("DisableWindowsUpdateAccess"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.telemetry.autologger_disable");
        t.category = QStringLiteral("privacy");
        t.name = QStringLiteral("Disable WMI autologger sessions");
        t.description = QStringLiteral("Disables 35 WMI autologger sessions used for diagnostics, telemetry, and logging by setting Start=0 on each session under CurrentControlSet\\\\Control\\\\WMI\\\\Autologger.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("WMI autologger sessions off; Wi-Fi, TPM, Defender, and setup diagnostic traces unavailable to support tools"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\AppModel"), QStringLiteral("Start"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\Cellcore"), QStringLiteral("Start"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\Circular Kernel Context Logger"), QStringLiteral("Start"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\CloudExperienceHostOobe"), QStringLiteral("Start"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\DataMarket"), QStringLiteral("Start"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\DefenderApiLogger"), QStringLiteral("Start"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\DefenderAuditLogger"), QStringLiteral("Start"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\DiagLog"), QStringLiteral("Start"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\HolographicDevice"), QStringLiteral("Start"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\iclsClient"), QStringLiteral("Start"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\iclsProxy"), QStringLiteral("Start"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\LwtNetLog"), QStringLiteral("Start"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\Mellanox-Kernel"), QStringLiteral("Start"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\Microsoft-Windows-AssignedAccess-Trace"), QStringLiteral("Start"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\Microsoft-Windows-Setup"), QStringLiteral("Start"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\NBSMBLOGGER"), QStringLiteral("Start"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\PEAuthLog"), QStringLiteral("Start"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\RdrLog"), QStringLiteral("Start"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\ReadyBoot"), QStringLiteral("Start"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\SetupPlatform"), QStringLiteral("Start"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\SetupPlatformTel"), QStringLiteral("Start"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\SocketHeciServer"), QStringLiteral("Start"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\SpoolerLogger"), QStringLiteral("Start"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\SQMLogger"), QStringLiteral("Start"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\TCPIPLOGGER"), QStringLiteral("Start"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\TileStore"), QStringLiteral("Start"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\Tpm"), QStringLiteral("Start"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\TPMProvisioningService"), QStringLiteral("Start"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\UBPM"), QStringLiteral("Start"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\WdiContextLog"), QStringLiteral("Start"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\WFP-IPsec Trace"), QStringLiteral("Start"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\WiFiDriverIHVSession"), QStringLiteral("Start"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\WiFiDriverIHVSessionRepro"), QStringLiteral("Start"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\WiFiSession"), QStringLiteral("Start"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\WinPhoneCritical"), QStringLiteral("Start"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.perf.device_class_priorities");
        t.category = QStringLiteral("gpu");
        t.name = QStringLiteral("Device class thread priority optimization");
        t.description = QStringLiteral("Raises IRQ-thread priorities across every hardware device class for snappier driver callbacks.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("All hardware classes contend at the same elevated priority; background tasks may starve."),
            QStringLiteral("Aggressive priority on every driver can destabilise hybrid laptops with many device classes."),
        };
        {
            CommandOp op;
            op.program = QStringLiteral("powershell");
            op.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';Get-ChildItem 'HKLM:\\SYSTEM\\CurrentControlSet\\Control\\Class' | ForEach-Object { Set-ItemProperty $_.PSPath -Name BasePriority -Value 200 -Type DWord; Set-ItemProperty $_.PSPath -Name OverTargetPriority -Value 80 -Type DWord }") };
            op.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';Get-ChildItem 'HKLM:\\SYSTEM\\CurrentControlSet\\Control\\Class' | ForEach-Object { Remove-ItemProperty $_.PSPath -Name BasePriority; Remove-ItemProperty $_.PSPath -Name OverTargetPriority }") };
            t.commandOps.push_back(op);
        }
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.startup.clear_run_keys");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Clear startup Run/RunOnce registry keys");
        t.description = QStringLiteral("Deletes all entries from HKCU and HKLM Run, RunOnce, RunOnceEx, RunServices, RunServicesOnce keys, plus Wow6432Node Run/RunOnce variants to remove all auto-start programs.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Removes every Run and RunOnce entry, including drivers, security tools and legitimate app auto-starts"),
            QStringLiteral("Apps that rely on RunOnce to finish setup after reboot may fail to complete installation"),
        };
        {
            CommandOp op;
            op.program = QStringLiteral("powershell");
            op.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run','Registry::HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnceEx','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunServices','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunServicesOnce','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Run','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\RunOnce')){Remove-Item -LiteralPath $k -Recurse -Force}") };
            op.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
            t.commandOps.push_back(op);
        }
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.cleanup.vendor_registry");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Vendor bloat and telemetry registry cleanup");
        t.description = QStringLiteral("Removes vendor startup/telemetry registry entries and stale driver-profile values from the source cleanup batch: NVIDIA/AMD Run-key launchers and helpers, Epic Games registration, AMD Radeon Settings caches, DWM overlay/test values, and GPU PowerMizer/PerfLevel overrides. Irreversible (deletes registry data).");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Irreversible: deleted registry keys/values cannot be restored by reverting"),
            QStringLiteral("Removes Epic Games launcher registration and NVIDIA/AMD helper entries; reinstall the vendor software to restore them"),
        };
        {
            CommandOp op;
            op.program = QStringLiteral("powershell");
            op.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';foreach($k in @('Registry::HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Serialize','Registry::HKEY_CURRENT_USER\\SOFTWARE\\Policies\\Microsoft\\Edge','Registry::HKEY_CURRENT_USER\\SOFTWARE\\Policies\\Microsoft\\EdgeUpdate','Registry::HKEY_LOCAL_MACHINE\\Software\\Policies\\Microsoft\\Edge','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Policies\\Microsoft\\EdgeUpdate','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Epic Games Launcher','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Epic Games Launcher','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\Epic Games','Registry::HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Epic Games','Registry::HKEY_CURRENT_USER\\Software\\Epic Games','Registry::HKEY_CLASSES_ROOT\\com.epicgames.launcher','Registry::HKEY_CLASSES_ROOT\\com.epicgames.launcher://','Registry::HKEY_CURRENT_USER\\Software\\Classes\\com.epicgames.launcher')){Remove-Item -LiteralPath $k -Recurse -Force}") };
            op.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
            t.commandOps.push_back(op);
        }
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.devices.global_power_off");
        t.category = QStringLiteral("power");
        t.name = QStringLiteral("Disable device power management globally");
        t.description = QStringLiteral("Clears the 'Allow the computer to turn off this device' and 'Allow this device to wake the computer' flags across every PnP device via WMI.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Disables device power-management and wake on all devices; may affect USB/NIC wake behavior."),
        };
        {
            CommandOp op;
            op.program = QStringLiteral("powershell");
            op.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';Get-PnpDevice | ForEach-Object { try { $n=($_.InstanceId); Get-PnpDeviceProperty -InstanceId $n -KeyName 'DEVPKEY_Device_PowerData' | Out-Null } catch {} }; powercfg /devicedisablewake *") };
            op.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
            t.commandOps.push_back(op);
        }
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.ifeo-priority-preset");
        t.category = QStringLiteral("advanced");
        t.name = QStringLiteral("Apply per-process priority preset (IFEO)");
        t.description = QStringLiteral("Elevates input/audio/desktop-compositor processes to Realtime/Critical and deprioritizes background services to Idle.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Misuse (elevating a wrong process to Realtime) can starve the kernel and freeze input."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\csrss.exe\\PerfOptions"), QStringLiteral("CpuPriorityClass"), 4u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\csrss.exe\\PerfOptions"), QStringLiteral("IoPriority"), 3u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\dwm.exe\\PerfOptions"), QStringLiteral("CpuPriorityClass"), 4u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\dwm.exe\\PerfOptions"), QStringLiteral("IoPriority"), 3u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\ntoskrnl.exe\\PerfOptions"), QStringLiteral("CpuPriorityClass"), 4u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\ntoskrnl.exe\\PerfOptions"), QStringLiteral("IoPriority"), 3u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\audiodg.exe\\PerfOptions"), QStringLiteral("CpuPriorityClass"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\audiodg.exe\\PerfOptions"), QStringLiteral("IoPriority"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\audiodg.exe\\PerfOptions"), QStringLiteral("PagePriority"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\lsass.exe\\PerfOptions"), QStringLiteral("CpuPriorityClass"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\lsass.exe\\PerfOptions"), QStringLiteral("IoPriority"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\lsass.exe\\PerfOptions"), QStringLiteral("PagePriority"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\SearchIndexer.exe\\PerfOptions"), QStringLiteral("CpuPriorityClass"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\SearchIndexer.exe\\PerfOptions"), QStringLiteral("IoPriority"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\SearchIndexer.exe\\PerfOptions"), QStringLiteral("PagePriority"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\svchost.exe\\PerfOptions"), QStringLiteral("CpuPriorityClass"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\svchost.exe\\PerfOptions"), QStringLiteral("IoPriority"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\svchost.exe\\PerfOptions"), QStringLiteral("PagePriority"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\TrustedInstaller.exe\\PerfOptions"), QStringLiteral("CpuPriorityClass"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\TrustedInstaller.exe\\PerfOptions"), QStringLiteral("IoPriority"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\TrustedInstaller.exe\\PerfOptions"), QStringLiteral("PagePriority"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\wuauclt.exe\\PerfOptions"), QStringLiteral("CpuPriorityClass"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\wuauclt.exe\\PerfOptions"), QStringLiteral("IoPriority"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\wuauclt.exe\\PerfOptions"), QStringLiteral("PagePriority"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.maintenance.driver_cleanup");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove old driver packages");
        t.description = QStringLiteral("Removes old and superseded driver packages from the Windows driver store using pnputil. Reclaims disk space occupied by outdated driver backups. One-shot action.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Rolling back a current driver is no longer possible; only the latest installed version is kept in the driver store"),
        };
        {
            CommandOp op;
            op.program = QStringLiteral("powershell");
            op.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("Dism.exe /Online /Cleanup-Image /StartComponentCleanup /ResetBase") };
            op.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
            t.commandOps.push_back(op);
        }
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("deb.features.capabilities_remove");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Remove unused Windows capabilities");
        t.description = QStringLiteral("Removes Features on Demand (FoD) matching vendor drivers, Internet Explorer, Steps Recorder, Windows Media Player, wallpapers, printing, Math Recognizer, OpenSSH, PowerShell ISE, Quick Assist, OneSync, Ralink drivers, virtual device drivers, TCP features, and Windows Hello components.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.reversible = false;
        t.warnings = {
            QStringLiteral("Removed capabilities cannot be restored without Windows source media"),
            QStringLiteral("Removes OpenSSH, PowerShell ISE, Quick Assist and Windows Hello components"),
        };
        {
            CommandOp op;
            op.program = QStringLiteral("powershell");
            op.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$ErrorActionPreference='SilentlyContinue';$pats=@('*Realtek*','*Intel*','*Broadcom*','*Qualcomm*','*Marvel*','*Vmware*','*Ethernet*','*InternetExplorer*','*StepsRecorder*','*DirectX.Configuration.Database*','*WindowsMediaPlayer*','*Wallpapers*','*Print*','*MathRecognizer*','*OpenSSH*','*PowerShell*','*QuickAssist*','*OneSync*','*LA57*','*Ralink*','*Virtual*','*TCP*','*Hello*');Get-WindowsCapability -Online|Where-Object{$_.State -eq 'Installed'}|ForEach-Object{$n=$_.Name;foreach($p in $pats){if($n -like $p){Remove-WindowsCapability -Online -Name $n;break}}}") };
            op.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("$null=$null") };
            t.commandOps.push_back(op);
        }
        c.push_back(t);
    }
}
}
