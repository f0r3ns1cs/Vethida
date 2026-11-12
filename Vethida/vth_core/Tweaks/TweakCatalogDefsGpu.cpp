#include "TweakEngine.h"

namespace vth::tweaks::detail {

void appendDefsGpu(QVector<TweakSpec>& c) {
    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.amd.clock_gating_disable");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("AMD");
        t.name = QStringLiteral("Disable AMD clock gating");
        t.description = QStringLiteral("Disables AMD GPU clock/power gating for stable clocks and consistent frametimes.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Higher idle power draw."),
            QStringLiteral("Higher GPU temperature."),
            QStringLiteral("No effect on non-AMD GPUs."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("DisableClockGating"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("DisablePowerGating"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("PP_DisablePowerContainment"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.amd-disable-ulps");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("AMD");
        t.name = QStringLiteral("AMD: disable ultra low power state (ULPS)");
        t.description = QStringLiteral("Sets EnableUlps=0 on the AMD GPU driver service. Stops latency spikes when waking from ULPS.");
        t.iconName = QStringLiteral("cpu-2");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdkmdag"), QStringLiteral("EnableUlps"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.amd.logging_disable");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable AMD logging drivers");
        t.description = QStringLiteral("Disables AMD's background logging and telemetry drivers that run continuously and consume CPU resources.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("AMD Radeon Software crash logs and Fendr diagnostics will not be collected"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdlog"), QStringLiteral("Start"), 4u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdfendr"), QStringLiteral("Start"), 4u, 2u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.amd.performance_mode");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("AMD");
        t.name = QStringLiteral("AMD GPU performance mode");
        t.description = QStringLiteral("Forces AMD GPU into maximum performance: high SCLK/MCLK/DPM/UVD, disables Wattman auto-tune and ASPM.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("GPU runs at max clocks always."),
            QStringLiteral("Increased power consumption and heat."),
            QStringLiteral("ASPM disabled."),
            QStringLiteral("No effect on non-AMD systems."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdkmpfd"), QStringLiteral("PP_DisableAutoWattman"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdwddmg"), QStringLiteral("PP_DisableAutoWattman"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdgpuv"), QStringLiteral("PP_DisableAutoWattman"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\atikmdag"), QStringLiteral("PP_DisableAutoWattman"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdkmpfd"), QStringLiteral("PP_Force3DPerformanceMode"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdwddmg"), QStringLiteral("PP_Force3DPerformanceMode"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdgpuv"), QStringLiteral("PP_Force3DPerformanceMode"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\atikmdag"), QStringLiteral("PP_Force3DPerformanceMode"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdkmpfd"), QStringLiteral("PP_ForceHighDPMLevel"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdwddmg"), QStringLiteral("PP_ForceHighDPMLevel"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdgpuv"), QStringLiteral("PP_ForceHighDPMLevel"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\atikmdag"), QStringLiteral("PP_ForceHighDPMLevel"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdkmpfd"), QStringLiteral("PP_ForceHighUVDState"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdwddmg"), QStringLiteral("PP_ForceHighUVDState"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdgpuv"), QStringLiteral("PP_ForceHighUVDState"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\atikmdag"), QStringLiteral("PP_ForceHighUVDState"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdkmpfd"), QStringLiteral("PP_ForceMCLKHigh"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdwddmg"), QStringLiteral("PP_ForceMCLKHigh"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdgpuv"), QStringLiteral("PP_ForceMCLKHigh"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\atikmdag"), QStringLiteral("PP_ForceMCLKHigh"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdkmpfd"), QStringLiteral("PP_ForceSCLKHigh"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdwddmg"), QStringLiteral("PP_ForceSCLKHigh"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdgpuv"), QStringLiteral("PP_ForceSCLKHigh"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\atikmdag"), QStringLiteral("PP_ForceSCLKHigh"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdkmpfd"), QStringLiteral("DisableAspmL0s"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdwddmg"), QStringLiteral("DisableAspmL0s"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdgpuv"), QStringLiteral("DisableAspmL0s"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\atikmdag"), QStringLiteral("DisableAspmL0s"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdkmpfd"), QStringLiteral("DisableAspmL1"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdwddmg"), QStringLiteral("DisableAspmL1"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdgpuv"), QStringLiteral("DisableAspmL1"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\atikmdag"), QStringLiteral("DisableAspmL1"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdkmpfd"), QStringLiteral("EnableAspmL0s"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdwddmg"), QStringLiteral("EnableAspmL0s"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdgpuv"), QStringLiteral("EnableAspmL0s"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\atikmdag"), QStringLiteral("EnableAspmL0s"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdkmpfd"), QStringLiteral("EnableAspmL1"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdwddmg"), QStringLiteral("EnableAspmL1"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdgpuv"), QStringLiteral("EnableAspmL1"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\atikmdag"), QStringLiteral("EnableAspmL1"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdkmpfd"), QStringLiteral("KMD_UseBestGPUPowerOption"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdwddmg"), QStringLiteral("KMD_UseBestGPUPowerOption"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdgpuv"), QStringLiteral("KMD_UseBestGPUPowerOption"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\atikmdag"), QStringLiteral("KMD_UseBestGPUPowerOption"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.dwm.directflip");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("Latency");
        t.name = QStringLiteral("Force DirectFlip and hardware cursor");
        t.description = QStringLiteral("Writes undocumented Force* values under the DWM key intended to pin DirectFlip presentation and hardware cursor. DWM normally negotiates these automatically; the real effect is unverified and may be inert on current builds.");
        t.iconName = QStringLiteral("monitor");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Some games and capture or streaming tools rely on DWM composition and may show a black window or fail to record"),
            QStringLiteral("Hardware cursor may flicker or disappear on specific GPU and driver combinations"),
            QStringLiteral("Future Windows or GPU driver updates could change how DirectFlip behaves"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\Dwm"), QStringLiteral("ForceDirectFlipMode"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\Dwm"), QStringLiteral("ForceHardwareCursor"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\Dwm"), QStringLiteral("ForceHardwareModeChange"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.disable-aero-shake");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Window management");
        t.name = QStringLiteral("Disable Aero Shake");
        t.description = QStringLiteral("Stops the gesture that minimizes every other window when one is shaken. Removes a frequent source of accidental window minimization.");
        t.iconName = QStringLiteral("adjustments");
        t.impact = Impact::Low;
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\Explorer"), QStringLiteral("NoWindowMinimizingShortcuts"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.hwaccel.energy_disable");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("Power");
        t.name = QStringLiteral("Disable GPU energy saving");
        t.description = QStringLiteral("Sets D3 transition latency to zero, preventing the GPU from entering power-saving states during active use.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("GPU will skip low-power D3 transitions while in use, raising idle power draw and temperature"),
            QStringLiteral("Laptop battery life will be noticeably shorter"),
            QStringLiteral("A reboot is required for the change to take effect"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Power"), QStringLiteral("DefaultD3TransitionLatencyActivelyUsed"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\GpuEnergyDrv"), QStringLiteral("Start"), 4u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\GpuEnergyDr"), QStringLiteral("Start"), 4u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.display.audio_bg_policy");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("Audio");
        t.name = QStringLiteral("Audio background latency policy");
        t.description = QStringLiteral("Adjusts UWP background-audio task policy (headless execution, no inactivity timeout, multiple background tasks). This affects UWP background-audio scheduling; a general reduction in game audio stalls is unverified but the change is harmless.");
        t.iconName = QStringLiteral("microphone-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Background audio tasks may consume more resources"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\BackgroundModel\\BackgroundAudioPolicy"), QStringLiteral("AllowHeadlessExecution"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\BackgroundModel\\BackgroundAudioPolicy"), QStringLiteral("AllowMultipleBackgroundTasks"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\BackgroundModel\\BackgroundAudioPolicy"), QStringLiteral("InactivityTimeoutMs"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.display.dxgi_settings");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("Latency");
        t.name = QStringLiteral("DXGI display settings");
        t.description = QStringLiteral("Configures DXGI (DirectX Graphics Infrastructure) behavior: disables delayed flips for lower frame latency and sets GDI process idle timeout for faster GPU resource release.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("May cause visual tearing if VSync is also disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows"), QStringLiteral("AllowDelayedFlips"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows"), QStringLiteral("GDIProcessIdleTimeout"), 30000u, 10000u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows"), QStringLiteral("DesktopHeapLogging"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.dwm.input_processing");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("Aero Peek");
        t.name = QStringLiteral("Disable DWM Aero Peek");
        t.description = QStringLiteral("Disables Aero Peek (taskbar/desktop hover preview) via EnableAeroPeek=0. The additional 'input processing' values it writes are undocumented and have no verified effect; the real result is simply turning off Aero Peek.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Taskbar live thumbnails and desktop peek-at-hover no longer work"),
        };
        t.registryOps = {
            // 1 is the Windows default (Aero Peek on).
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("EnableAeroPeek"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows"), QStringLiteral("DesktopHeapLogging"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows"), QStringLiteral("DwmInputUsesIoCompletionPort"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows"), QStringLiteral("EnableDwmInputProcessing"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.dwm.visual_optimize");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("Aero Peek");
        t.name = QStringLiteral("DWM visual optimizations");
        t.description = QStringLiteral("Disables Aero Peek desktop preview and hibernate thumbnails for reduced DWM overhead.");
        t.iconName = QStringLiteral("palette");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Aero Peek preview on taskbar hover disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("EnableAeroPeek"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("AlwaysHibernateThumbnails"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.dxgi-use-flip-model");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("Latency");
        t.name = QStringLiteral("Force DXGI flip-model presentation");
        t.description = QStringLiteral("Writes a UseFlipModel value under the DXGI key. Flip-model presentation is selected per swap chain by the application/DWM and is not globally forced by this key, so the effect is unverified; it is not required for variable refresh rate.");
        t.iconName = QStringLiteral("monitor");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX\\DXGI"), QStringLiteral("UseFlipModel"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.dx.windowed_swap_override");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("Latency");
        t.name = QStringLiteral("Disable windowed game optimization");
        t.description = QStringLiteral("Disables the automatic swap effect upgrade for windowed games. Prevents Windows from overriding the application's chosen swap chain model. Can reduce input lag in windowed/borderless modes.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Borderless-windowed games may tear or run at lower effective frame rate without the swap-effect upgrade"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\DirectX\\GraphicsSettings"), QStringLiteral("SwapEffectUpgradeCache"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.gaming.disable_gamedvr");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("Game DVR");
        t.name = QStringLiteral("Disable Game DVR");
        t.description = QStringLiteral("Disables Windows Game DVR recording, fullscreen exclusive behavior overrides, app capture, historical capture, and the Game DVR policy. Eliminates background recording overhead for improved gaming performance.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Background game recording disabled"),
            QStringLiteral("Game clip capture unavailable"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\System\\GameConfigStore"), QStringLiteral("GameDVR_Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\System\\GameConfigStore"), QStringLiteral("GameDVR_FSEBehaviorMode"), 2u, 0u),
            dwordOp(QStringLiteral("HKCU\\System\\GameConfigStore"), QStringLiteral("GameDVR_HonorUserFSEBehaviorMode"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\System\\GameConfigStore"), QStringLiteral("GameDVR_DXGIHonorFSEWindowsCompatible"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\System\\GameConfigStore"), QStringLiteral("GameDVR_FSEBehavior"), 2u, 0u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\GameDVR"), QStringLiteral("AppCaptureEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\GameDVR"), QStringLiteral("HistoricalCaptureEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\GameDVR"), QStringLiteral("AllowgameDVR"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\ApplicationManagement\\AllowGameDVR"), QStringLiteral("value"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.gaming.disable_xbgm");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("Game DVR");
        t.name = QStringLiteral("Disable Xbox game monitoring");
        t.description = QStringLiteral("Disables the Xbox Game Monitoring background service (EnableXBGM) and forces fullscreen exclusive behavior mode to reduce gaming overhead from Xbox integration.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Xbox game activity tracking disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Games"), QStringLiteral("EnableXBGM"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\System\\GameConfigStore"), QStringLiteral("GameDVR_FSEBehavior"), 2u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.dx.thread_priority");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("Latency");
        t.name = QStringLiteral("Maximize GPU thread priority");
        t.description = QStringLiteral("Writes MonitorLatencyTolerance=1, DisableWriteCombining=0, and a legacy Direct3D FlipPresentToVSync value. It does not actually change thread priority; the effect is a minor latency-tolerance hint and is largely near-default.");
        t.iconName = QStringLiteral("arrow-up");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Graphics threads run at higher priority, which can starve other background work on low-core-count CPUs"),
            QStringLiteral("Touches Direct3D and graphics kernel behavior that future Windows updates may change"),
            QStringLiteral("If games crash or stutter after applying, reverting this tweak is the first thing to try"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D"), QStringLiteral("FlipPresentToVSync"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("MonitorLatencyTolerance"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("DisableWriteCombining"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.hwaccel.hags");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("GPU scheduling");
        t.name = QStringLiteral("Hardware-accelerated GPU scheduling (HAGS)");
        t.description = QStringLiteral("Enables hardware-accelerated GPU scheduling to reduce CPU overhead and input latency.");
        t.iconName = QStringLiteral("monitor");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Requires a recent GPU and driver; on older hardware the system may fall back or misbehave"),
            QStringLiteral("Some games and capture or overlay tools show regressions with HAGS enabled"),
            QStringLiteral("A reboot is required for the change to take effect"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("HwSchMode"), 2u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("DpiMapIommuContiguous"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("RmGpsPsEnablePerCpuCoreDpc"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Power"), QStringLiteral("RmGpsPsEnablePerCpuCoreDpc"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0001"), QStringLiteral("RMPowerFeature"), 4u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.intel.dedicated_segment");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("Intel");
        t.name = QStringLiteral("Intel IGPU dedicated segment size");
        t.description = QStringLiteral("Sets the Intel integrated GPU dedicated video memory segment to 512 MB. Only effective on systems with Intel integrated graphics (HD/UHD/Iris).");
        t.iconName = QStringLiteral("monitor");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("512 MB of system RAM is reserved for Intel graphics and unavailable to other apps"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Intel\\GMM"), QStringLiteral("DedicatedSegmentSize"), 512u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.nvidia.ansel_disable");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("NVIDIA");
        t.name = QStringLiteral("Disable NVIDIA Ansel");
        t.description = QStringLiteral("Disables the NVIDIA Ansel in-game screenshot overlay.");
        t.iconName = QStringLiteral("monitor");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("NVIDIA Ansel screenshot overlay will not appear."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\NVIDIA Corporation\\Global\\Ansel"), QStringLiteral("EnableAnsel"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.nvidia.clock_gating_disable");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("NVIDIA");
        t.name = QStringLiteral("Disable NVIDIA clock gating");
        t.description = QStringLiteral("Disables dynamic P-state transitions and forces maximum performance clock levels on NVIDIA GPUs.");
        t.iconName = QStringLiteral("bolt-off");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("GPU stays at high clocks even on the desktop, increasing power draw, heat, and fan noise"),
            QStringLiteral("Laptops will see noticeably shorter battery life"),
            QStringLiteral("A reboot is required for the change to take effect"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("DisableDynamicPstate"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("PerfLevelSrc"), 0x2222u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("DisableClockGating"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.nvidia.coproc_tray_disable");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("NVIDIA");
        t.name = QStringLiteral("NVIDIA disable CoProcManager tray icon");
        t.description = QStringLiteral("Hides the NVIDIA CoProcManager system tray icon to reduce visual clutter and minor background overhead.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("NVIDIA tray-icon shortcuts to Control Panel and GeForce Experience disappear"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\NVIDIA Corporation\\Global\\CoProcManager"), QStringLiteral("ShowTrayIcon"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.nvidia-disable-telemetry-registry");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("Telemetry");
        t.name = QStringLiteral("NVIDIA: disable driver telemetry (registry layer)");
        t.description = QStringLiteral(
            "Opts this PC out of the usage data the NVIDIA driver collects.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\NVIDIA Corporation\\NvControlPanel2\\Client"), QStringLiteral("OptInOrOutPreference"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.nvidia-disable-telemetry-startup");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("Telemetry");
        t.name = QStringLiteral("NVIDIA: disable SendTelemetryData on driver startup");
        t.description = QStringLiteral(
            "Stops the NVIDIA driver sending a usage report every time Windows starts.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\nvlddmkm\\Global\\Startup"), QStringLiteral("SendTelemetryData"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.nvidia.display_container");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("NVIDIA disable display container plugin");
        t.description = QStringLiteral("Disables NvcDispCorePlugin to reduce background CPU usage from the NVIDIA Display Container service.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("NVIDIA display container features may stop working"),
            QStringLiteral("Multi-monitor management may be affected"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ControlSet001\\Services\\NVDisplay.ContainerLocalSystem\\LocalSystem\\NvcDispCorePlugin"), QStringLiteral("DisableLoad"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ControlSet001\\Services\\NVDisplay.ContainerLocalSystem\\LocalSystem\\NvcDispCorePlugin"), QStringLiteral("LogLevel"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.nvidia.extended_perf");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("NVIDIA");
        t.name = QStringLiteral("NVIDIA extended performance");
        t.description = QStringLiteral("Applies a bundle of advanced NVIDIA driver performance flags on the primary adapter.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Benchmark before/after - effect varies by GPU model."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("Acceleration.Level"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("FeatureControl"), 4u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("NVDeviceSupportKFilter"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("RmCacheLoc"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("RmDisableInst2Sys"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("RmFbsrPagedDMA"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("RmProfilingAdminOnly"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("TrackResetEngine"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("ValidateBlitSubRects"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("PreferSystemMemoryContiguous"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("TCCSupported"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.nvidia.framebuffer_capture");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("NVIDIA");
        t.name = QStringLiteral("Enable NVIDIA frame buffer capture");
        t.description = QStringLiteral("Enables NVFBCEnable for frame buffer capture support and disables tiled display to ensure single-buffer rendering. Useful for streaming and recording applications.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Tiled-display configurations (Surround/Mosaic) stop working with EnableTiledDisplay off"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\nvlddmkm"), QStringLiteral("NVFBCEnable"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\nvlddmkm"), QStringLiteral("EnableTiledDisplay"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.nvidia.fts_rids");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("NVIDIA");
        t.name = QStringLiteral("NVIDIA FTS performance RIDs");
        t.description = QStringLiteral("Enables undocumented NVIDIA Feature Trial Service RIDs. These are internal flags with no public documentation; any performance benefit is unproven and they may stop working or regress after a driver update.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Undocumented feature IDs may stop working or cause regressions after a driver update"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\nvlddmkm\\FTS"), QStringLiteral("EnableRID61684"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\nvlddmkm\\FTS"), QStringLiteral("EnableRID73779"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\nvlddmkm\\FTS"), QStringLiteral("EnableRID73780"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\nvlddmkm\\FTS"), QStringLiteral("EnableRID74361"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.nvidia.latency_tuning");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("Latency");
        t.name = QStringLiteral("Disable NVIDIA deep L1 link state");
        t.description = QStringLiteral("Disables PCIe deep L1 link state on NVIDIA GPUs to reduce wake-up latency.");
        t.iconName = QStringLiteral("activity");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("GPU idle power draw increases slightly because the PCIe link no longer enters deep sleep"),
            QStringLiteral("Laptops will see modestly reduced battery life"),
            QStringLiteral("A reboot is required for the change to take effect"),
        };
        t.registryOps = {
            // Absent on a stock NVIDIA install, the driver's own default applies, so
            // revert removes it.
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("RMDeepL1LinkState"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("D3PCLatency"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0001"), QStringLiteral("D3PCLatency"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0002"), QStringLiteral("D3PCLatency"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0003"), QStringLiteral("D3PCLatency"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("F1TransitionLatency"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0001"), QStringLiteral("F1TransitionLatency"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0002"), QStringLiteral("F1TransitionLatency"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0003"), QStringLiteral("F1TransitionLatency"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("LOWLATENCY"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0001"), QStringLiteral("LOWLATENCY"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0002"), QStringLiteral("LOWLATENCY"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0003"), QStringLiteral("LOWLATENCY"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("Node3DLowLatency"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0001"), QStringLiteral("Node3DLowLatency"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0002"), QStringLiteral("Node3DLowLatency"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0003"), QStringLiteral("Node3DLowLatency"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("PciLatencyTimerControl"), 0x32u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0001"), QStringLiteral("PciLatencyTimerControl"), 0x32u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0002"), QStringLiteral("PciLatencyTimerControl"), 0x32u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0003"), QStringLiteral("PciLatencyTimerControl"), 0x32u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("RMDeepL1EntryLatencyUsec"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0001"), QStringLiteral("RMDeepL1EntryLatencyUsec"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0002"), QStringLiteral("RMDeepL1EntryLatencyUsec"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0003"), QStringLiteral("RMDeepL1EntryLatencyUsec"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("RmGspcMaxFtuS"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0001"), QStringLiteral("RmGspcMaxFtuS"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0002"), QStringLiteral("RmGspcMaxFtuS"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0003"), QStringLiteral("RmGspcMaxFtuS"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("RmGspcMinFtuS"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0001"), QStringLiteral("RmGspcMinFtuS"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0002"), QStringLiteral("RmGspcMinFtuS"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0003"), QStringLiteral("RmGspcMinFtuS"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("RmGspcPerioduS"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0001"), QStringLiteral("RmGspcPerioduS"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0002"), QStringLiteral("RmGspcPerioduS"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0003"), QStringLiteral("RmGspcPerioduS"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("RMLpwrEiIdleThresholdUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0001"), QStringLiteral("RMLpwrEiIdleThresholdUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0002"), QStringLiteral("RMLpwrEiIdleThresholdUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0003"), QStringLiteral("RMLpwrEiIdleThresholdUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("RMLpwrGrIdleThresholdUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0001"), QStringLiteral("RMLpwrGrIdleThresholdUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0002"), QStringLiteral("RMLpwrGrIdleThresholdUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0003"), QStringLiteral("RMLpwrGrIdleThresholdUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("RMLpwrGrRgIdleThresholdUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0001"), QStringLiteral("RMLpwrGrRgIdleThresholdUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0002"), QStringLiteral("RMLpwrGrRgIdleThresholdUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0003"), QStringLiteral("RMLpwrGrRgIdleThresholdUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("RMLpwrMsIdleThresholdUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0001"), QStringLiteral("RMLpwrMsIdleThresholdUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0002"), QStringLiteral("RMLpwrMsIdleThresholdUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0003"), QStringLiteral("RMLpwrMsIdleThresholdUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("VRDirectFlipDPCDelayUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0001"), QStringLiteral("VRDirectFlipDPCDelayUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0002"), QStringLiteral("VRDirectFlipDPCDelayUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0003"), QStringLiteral("VRDirectFlipDPCDelayUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("VRDirectFlipTimingMarginUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0001"), QStringLiteral("VRDirectFlipTimingMarginUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0002"), QStringLiteral("VRDirectFlipTimingMarginUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0003"), QStringLiteral("VRDirectFlipTimingMarginUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("VRDirectJITFlipMsHybridFlipDelayUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0001"), QStringLiteral("VRDirectJITFlipMsHybridFlipDelayUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0002"), QStringLiteral("VRDirectJITFlipMsHybridFlipDelayUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0003"), QStringLiteral("VRDirectJITFlipMsHybridFlipDelayUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("vrrCursorMarginUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0001"), QStringLiteral("vrrCursorMarginUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0002"), QStringLiteral("vrrCursorMarginUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0003"), QStringLiteral("vrrCursorMarginUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("vrrDeflickerMarginUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0001"), QStringLiteral("vrrDeflickerMarginUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0002"), QStringLiteral("vrrDeflickerMarginUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0003"), QStringLiteral("vrrDeflickerMarginUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("vrrDeflickerMaxUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0001"), QStringLiteral("vrrDeflickerMaxUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0002"), QStringLiteral("vrrDeflickerMaxUs"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0003"), QStringLiteral("vrrDeflickerMaxUs"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.nvidia.legacy_sharpening");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("NVIDIA");
        t.name = QStringLiteral("NVIDIA legacy scaling and sharpening");
        t.description = QStringLiteral("Re-enables the pre-NIS legacy image-scaling and sharpening filter in the NVIDIA driver.");
        t.iconName = QStringLiteral("monitor");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Visual preference - benchmark your games."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("RMEnableOldScalingAndSharpening"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.nvidia.low_latency_profile");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("Latency");
        t.name = QStringLiteral("NVIDIA low latency driver profile");
        t.description = QStringLiteral("Applies a curated set of hidden NVIDIA driver registry settings optimized for minimum input latency. Includes maximum pre-rendered frames=1, shader cache=on, threaded optimization=on, texture filtering quality=high performance, and power management=prefer maximum performance.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Texture filtering drops to high-performance quality; image sharpness reduces in some games"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("RMDisablePostL2Compression"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("RMGpuOverclockHighPerfState"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("RMEnableWMI"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\NVIDIA Corporation\\Global\\NVTweak"), QStringLiteral("RMOverclockLatencyOptimize"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.nvidia.msi_mode");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("Latency");
        t.name = QStringLiteral("Enable GPU MSI mode");
        t.description = QStringLiteral("Enables Message-Signaled Interrupts (MSI) on the GPU and lowers DevicePriority for faster IRQ service.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("BSOD on reboot if GPU doesn't support MSI - boot Safe Mode to revert."),
            QStringLiteral("Rarely conflicts with other devices sharing interrupt lines."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000\\Device Parameters\\Interrupt Management\\MessageSignaledInterruptProperties"), QStringLiteral("MSISupported"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000\\Device Parameters\\Interrupt Management\\Affinity Policy"), QStringLiteral("DevicePriority"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.nvidia.quality_performance");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("NVIDIA");
        t.name = QStringLiteral("NVIDIA prefer maximum performance");
        t.description = QStringLiteral("Forces NVIDIA GPU to source performance levels from the maximum performance table.");
        t.iconName = QStringLiteral("trending-up");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("GPU will run at maximum clocks at all times, drawing more power and producing more heat"),
            QStringLiteral("Laptop battery life and fan noise will noticeably increase"),
            QStringLiteral("A reboot is required for the change to take effect"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("PerfLevelSrc"), 0x2222u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\NVIDIA Corporation\\Global\\Anisotropic"), QStringLiteral("Anisotropic Value"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\NVIDIA Corporation\\Global\\NvCplApi\\Policies\\QualityVsPerformance"), QStringLiteral("Quality"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\nvlddmkm\\NVAPI"), QStringLiteral("RmGpsPsEnablePerCpuCoreDpc"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.nvidia.telemetry_disable");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("Telemetry");
        t.name = QStringLiteral("Disable NVIDIA telemetry service");
        t.description = QStringLiteral("Disables the NVIDIA Telemetry Container service that collects and transmits usage data.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Crash diagnostics and driver-update telemetry no longer reach NVIDIA"),
        };
        t.registryOps = {
            // Written by the NVIDIA installer's opt-in prompt, not shipped with a
            // default, so revert removes it.
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\NVIDIA Corporation\\NvControlPanel2\\Client"), QStringLiteral("OptInOrOutPreference"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\nvlddmkm\\Global\\Startup"), QStringLiteral("SendTelemetryData"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NvTelemetryContainer"), QStringLiteral("Start"), 4u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\NVIDIA Corporation\\Global\\FTS"), QStringLiteral("EnableRID44231"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\NVIDIA Corporation\\Global\\FTS"), QStringLiteral("EnableRID64640"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\NVIDIA Corporation\\Global\\FTS"), QStringLiteral("EnableRID66610"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ControlSet001\\Services\\nvlddmkm\\Global\\NVTweak"), QStringLiteral("DisplayPowerSaving"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ControlSet001\\Services\\nvlddmkm\\Global\\NVTweak"), QStringLiteral("RmGpsPsEnablePerCpuCoreDpc"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\NVIDIA Corporation\\Global\\NVTweak"), QStringLiteral("DisplayPowerSaving"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.nvidia.vram_selfrefresh_disable");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("NVIDIA");
        t.name = QStringLiteral("Disable NVIDIA VRAM self-refresh");
        t.description = QStringLiteral("Disables VRAM self-refresh on NVIDIA GPUs for lower memory latency at the cost of idle power.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Increased VRAM power consumption."),
            QStringLiteral("May cause instability on some GPUs."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("RMDisableVRAMSelfRefresh"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.perf.fso_config");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("Fullscreen optimizations");
        t.name = QStringLiteral("Full screen optimization configuration");
        t.description = QStringLiteral("Configures GameConfigStore fullscreen optimization behavior: resets DSE behavior, FSE behavior mode, EFSE feature flags, DXGI FSE compatibility, and honors user FSE behavior mode.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Alt-Tab out of fullscreen games takes longer and Game DVR capture stops working"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\SYSTEM\\GameConfigStore"), QStringLiteral("GameDVR_DSEBehavior"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SYSTEM\\GameConfigStore"), QStringLiteral("GameDVR_FSEBehaviorMode"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SYSTEM\\GameConfigStore"), QStringLiteral("GameDVR_EFSEFeatureFlags"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SYSTEM\\GameConfigStore"), QStringLiteral("GameDVR_DXGIHonorFSEWindowsCompatible"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SYSTEM\\GameConfigStore"), QStringLiteral("GameDVR_HonorUserFSEBehaviorMode"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.perf.kernel_priorities");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("Priorities");
        t.name = QStringLiteral("Kernel worker and foreground boost values");
        t.description = QStringLiteral("Sets two kernel values: PassiveIntRealTimeWorkerPriority=18 under the I/O System key, and DisableFGBoostDecay=1 under KernelVelocity. The performance benefit is unproven.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Windows Update operations may run slower."),
            QStringLiteral("Background services deprioritized - some may feel unresponsive."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\I/O System"), QStringLiteral("PassiveIntRealTimeWorkerPriority"), 18u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\KernelVelocity"), QStringLiteral("DisableFGBoostDecay"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.perf.latency_tolerance");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("Latency");
        t.name = QStringLiteral("System latency tolerance");
        t.description = QStringLiteral("Tightens GraphicsDrivers\\\\Power and DXGKrnl latency-tolerance values to 1 microsecond.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Increased idle power."),
            QStringLiteral("GPU may not enter deep low-power states."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Power"), QStringLiteral("D3PCLatency"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Power"), QStringLiteral("MonitorLatencyTolerance"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Power"), QStringLiteral("IdleLatencyTolerance"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Power"), QStringLiteral("RMDeepL1EntryLatencyUsec"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DXGKrnl"), QStringLiteral("MonitorLatencyTolerance"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.wddm.queue_limit");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("GPU scheduling");
        t.name = QStringLiteral("WDDM queue and latency optimization");
        t.description = QStringLiteral("Reduces WDDM command queue depth and idle timeouts for minimum GPU scheduling latency.");
        t.iconName = QStringLiteral("gauge");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Average framerate in GPU-bound games can drop by a small amount in exchange for lower input lag"),
            QStringLiteral("Frame pacing may feel less smooth in titles that prefer deeper queuing"),
            QStringLiteral("A reboot is required for the change to take effect"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Scheduler"), QStringLiteral("EnableVidMmIommuDMA"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Scheduler"), QStringLiteral("DisableHwSchProcessing"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Scheduler"), QStringLiteral("GpuIdleTimeout"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.nvidia.telemetry_tasks");
        t.category = QStringLiteral("gpu");
        t.subCategory = QStringLiteral("Telemetry");
        t.name = QStringLiteral("Disable NVIDIA telemetry scheduled tasks");
        t.description = QStringLiteral("Disables NVIDIA telemetry scheduled tasks that periodically collect and report usage data.");
        t.iconName = QStringLiteral("clock");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        CommandOp to0;
        to0.program = QStringLiteral("schtasks");
        to0.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\NvTmMon_{B2FE1952-0186-46C3-BAEC-A80AA35AC5B8}"), QStringLiteral("/Disable") };
        to0.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\NvTmMon_{B2FE1952-0186-46C3-BAEC-A80AA35AC5B8}"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to0);
        CommandOp to1;
        to1.program = QStringLiteral("schtasks");
        to1.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\NvTmRep_{B2FE1952-0186-46C3-BAEC-A80AA35AC5B8}"), QStringLiteral("/Disable") };
        to1.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\NvTmRep_{B2FE1952-0186-46C3-BAEC-A80AA35AC5B8}"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to1);
        CommandOp to2;
        to2.program = QStringLiteral("schtasks");
        to2.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\NvTmRepOnLogon_{B2FE1952-0186-46C3-BAEC-A80AA35AC5B8}"), QStringLiteral("/Disable") };
        to2.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\NvTmRepOnLogon_{B2FE1952-0186-46C3-BAEC-A80AA35AC5B8}"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to2);
        CommandOp to3;
        to3.program = QStringLiteral("schtasks");
        to3.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\NvTmRep_CrashReport1_{B2FE1952-0186-46C3-BAEC-A80AA35AC5B8}"), QStringLiteral("/Disable") };
        to3.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\NvTmRep_CrashReport1_{B2FE1952-0186-46C3-BAEC-A80AA35AC5B8}"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to3);
        CommandOp to4;
        to4.program = QStringLiteral("schtasks");
        to4.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\NvTmRep_CrashReport2_{B2FE1952-0186-46C3-BAEC-A80AA35AC5B8}"), QStringLiteral("/Disable") };
        to4.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\NvTmRep_CrashReport2_{B2FE1952-0186-46C3-BAEC-A80AA35AC5B8}"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to4);
        CommandOp to5;
        to5.program = QStringLiteral("schtasks");
        to5.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\NvTmRep_CrashReport3_{B2FE1952-0186-46C3-BAEC-A80AA35AC5B8}"), QStringLiteral("/Disable") };
        to5.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\NvTmRep_CrashReport3_{B2FE1952-0186-46C3-BAEC-A80AA35AC5B8}"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to5);
        CommandOp to6;
        to6.program = QStringLiteral("schtasks");
        to6.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\NvTmRep_CrashReport4_{B2FE1952-0186-46C3-BAEC-A80AA35AC5B8}"), QStringLiteral("/Disable") };
        to6.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\NvTmRep_CrashReport4_{B2FE1952-0186-46C3-BAEC-A80AA35AC5B8}"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to6);
        CommandOp to7;
        to7.program = QStringLiteral("schtasks");
        to7.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\NvDriverUpdateCheckDaily_{B2FE1952-0186-46C3-BAEC-A80AA35AC5B8}"), QStringLiteral("/Disable") };
        to7.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\NvDriverUpdateCheckDaily_{B2FE1952-0186-46C3-BAEC-A80AA35AC5B8}"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to7);
        CommandOp to8;
        to8.program = QStringLiteral("schtasks");
        to8.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\NVIDIA GeForce Experience SelfUpdate_{B2FE1952-0186-46C3-BAEC-A80AA35AC5B8}"), QStringLiteral("/Disable") };
        to8.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\NVIDIA GeForce Experience SelfUpdate_{B2FE1952-0186-46C3-BAEC-A80AA35AC5B8}"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to8);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.amd.anti_lag_enable");
        t.category = QStringLiteral("gpu");
        t.name = QStringLiteral("Enable AMD Anti-Lag");
        t.description = QStringLiteral("Enables AMD Anti-Lag for reduced input-to-display latency on AMD GPUs.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("No effect on non-AMD hardware."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000\\UMD"), QStringLiteral("AntiLag"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.amd.extended_perf");
        t.category = QStringLiteral("gpu");
        t.name = QStringLiteral("AMD extended performance and latency");
        t.description = QStringLiteral("Comprehensive AMD GPU settings: disables power/clock gating, ULPS, thermal throttling, enables de-lag, and resizes the UMD shader cache and flip queue.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Increased GPU temp and power draw."),
            QStringLiteral("Thermal auto-throttling disabled - monitor temps."),
            QStringLiteral("No effect on non-AMD systems."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdkmpfd"), QStringLiteral("DisableSAMUPowerGating"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdwddmg"), QStringLiteral("DisableSAMUPowerGating"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdgpuv"), QStringLiteral("DisableSAMUPowerGating"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\atikmdag"), QStringLiteral("DisableSAMUPowerGating"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdkmpfd"), QStringLiteral("DisableUVDPowerGatingDynamic"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdwddmg"), QStringLiteral("DisableUVDPowerGatingDynamic"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdgpuv"), QStringLiteral("DisableUVDPowerGatingDynamic"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\atikmdag"), QStringLiteral("DisableUVDPowerGatingDynamic"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdkmpfd"), QStringLiteral("DisableVCEPowerGating"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdwddmg"), QStringLiteral("DisableVCEPowerGating"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdgpuv"), QStringLiteral("DisableVCEPowerGating"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\atikmdag"), QStringLiteral("DisableVCEPowerGating"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdkmpfd"), QStringLiteral("DisablePowerGating"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdwddmg"), QStringLiteral("DisablePowerGating"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdgpuv"), QStringLiteral("DisablePowerGating"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\atikmdag"), QStringLiteral("DisablePowerGating"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdkmpfd"), QStringLiteral("DisableDrmdmaPowerGating"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdwddmg"), QStringLiteral("DisableDrmdmaPowerGating"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdgpuv"), QStringLiteral("DisableDrmdmaPowerGating"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\atikmdag"), QStringLiteral("DisableDrmdmaPowerGating"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdkmpfd"), QStringLiteral("EnableVceSwClockGating"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdwddmg"), QStringLiteral("EnableVceSwClockGating"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdgpuv"), QStringLiteral("EnableVceSwClockGating"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\atikmdag"), QStringLiteral("EnableVceSwClockGating"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdkmpfd"), QStringLiteral("EnableUvdClockGating"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdwddmg"), QStringLiteral("EnableUvdClockGating"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdgpuv"), QStringLiteral("EnableUvdClockGating"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\atikmdag"), QStringLiteral("EnableUvdClockGating"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdkmpfd"), QStringLiteral("EnableUlps"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdwddmg"), QStringLiteral("EnableUlps"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdgpuv"), QStringLiteral("EnableUlps"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\atikmdag"), QStringLiteral("EnableUlps"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdkmpfd"), QStringLiteral("EnableUlps_NA"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdwddmg"), QStringLiteral("EnableUlps_NA"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdgpuv"), QStringLiteral("EnableUlps_NA"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\atikmdag"), QStringLiteral("EnableUlps_NA"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdkmpfd"), QStringLiteral("PP_SclkDeepSleepDisable"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdwddmg"), QStringLiteral("PP_SclkDeepSleepDisable"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdgpuv"), QStringLiteral("PP_SclkDeepSleepDisable"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\atikmdag"), QStringLiteral("PP_SclkDeepSleepDisable"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdkmpfd"), QStringLiteral("PP_ThermalAutoThrottlingEnable"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdwddmg"), QStringLiteral("PP_ThermalAutoThrottlingEnable"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdgpuv"), QStringLiteral("PP_ThermalAutoThrottlingEnable"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\atikmdag"), QStringLiteral("PP_ThermalAutoThrottlingEnable"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdkmpfd"), QStringLiteral("PP_GPUPowerDownEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdwddmg"), QStringLiteral("PP_GPUPowerDownEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdgpuv"), QStringLiteral("PP_GPUPowerDownEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\atikmdag"), QStringLiteral("PP_GPUPowerDownEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdkmpfd"), QStringLiteral("KMD_DeLagEnabled"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdwddmg"), QStringLiteral("KMD_DeLagEnabled"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdgpuv"), QStringLiteral("KMD_DeLagEnabled"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\atikmdag"), QStringLiteral("KMD_DeLagEnabled"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdkmpfd"), QStringLiteral("KMD_FRTEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdwddmg"), QStringLiteral("KMD_FRTEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdgpuv"), QStringLiteral("KMD_FRTEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\atikmdag"), QStringLiteral("KMD_FRTEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdkmpfd"), QStringLiteral("DisableDMACopy"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdwddmg"), QStringLiteral("DisableDMACopy"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdgpuv"), QStringLiteral("DisableDMACopy"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\atikmdag"), QStringLiteral("DisableDMACopy"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdkmpfd"), QStringLiteral("DisableBlockWrite"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdwddmg"), QStringLiteral("DisableBlockWrite"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdgpuv"), QStringLiteral("DisableBlockWrite"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\atikmdag"), QStringLiteral("DisableBlockWrite"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdkmpfd"), QStringLiteral("StutterMode"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdwddmg"), QStringLiteral("StutterMode"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdgpuv"), QStringLiteral("StutterMode"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\atikmdag"), QStringLiteral("StutterMode"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000\\UMD"), QStringLiteral("Main3D_DEF"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000\\UMD"), QStringLiteral("Main3D"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000\\UMD"), QStringLiteral("FlipQueueSize"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000\\UMD"), QStringLiteral("ShaderCache"), 32u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000\\UMD"), QStringLiteral("TFQ"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("3D_Refresh_Rate_Override_DEF"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("AllowSnapshot"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("AllowSubscription"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("AutoColorDepthReduction_NA"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("Adaptive De-interlacing"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("AAF_NA"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("AntiAlias_NA"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("ASTT_NA"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("AreaAniso_NA"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\amdlog"), QStringLiteral("Start"), 4u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.amd.hsa_enable");
        t.category = QStringLiteral("gpu");
        t.name = QStringLiteral("Enable AMD HSA");
        t.description = QStringLiteral("Enables AMD HSA (Heterogeneous System Architecture) for CPU/GPU compute on APUs.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Only effective on AMD APUs with HSA-capable IGPs."),
            QStringLiteral("No effect on NVIDIA or Intel."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\"), QStringLiteral("KMD_EnableHsa"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\"), QStringLiteral("KMD_EnableHsaHws"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.dwm.compositor_latency");
        t.category = QStringLiteral("gpu");
        t.name = QStringLiteral("Reduce DWM compositor latency");
        t.description = QStringLiteral("Disables DWM overlays and backdrop Mica to reduce compositor latency.");
        t.iconName = QStringLiteral("layers");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Mica backdrop and acrylic blur effects stop rendering in Windows 11 UI"),
            QStringLiteral("Hardware overlay plane disabled; video playback may use more GPU"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("DisableOverlays"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("EnableBackdropMicaForSystemBackdrop"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("FrameLatency"), 2u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("ForceDirectDrawSync"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("MaxQueuedPresentBuffers"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("SuperWetEnabled"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("SDRBoostPercentOverride"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("ResampleInLinearSpace"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("OneCoreNoDWMRawGameController"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("MPCInputRouterWaitForDebugger"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("InteractionOutputPredictionDisabled"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("InkGPUAccelOverrideVendorWhitelist"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("EnableRenderPathTestMode"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("FlattenVirtualSurfaceEffectInput"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("EnableCpuClipping"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("DisallowNonDrawListRendering"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("DisableProjectedShadowsRendering"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("DisableProjectedShadows"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("DisableLockingMemory"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("DisableHologramCompositor"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("DisableDeviceBitmaps"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("DebugFailFast"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("DisplayTestMode"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("UseHWDrawListEntriesOnWARP"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("ResampleModeOverride"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("ParallelModePolicy"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("EnableResizeOptimization"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("EnableMegaRects"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("EnableFrontBufferRenderChecks"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("EnableEffectCaching"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("EnableDesktopOverlays"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("EnablePrimitiveReordering"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("MaxD3DFeatureLevel"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("OverlayQualifyCount"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("OverlayDisqualifyCount"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("ResizeTimeoutModern"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("ResizeTimeoutGdi"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("HighColor"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("AnimationsShiftKey"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("DisableDrawListCaching"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("AnimationAttributionEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("EnableCommonSuperSets"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("DisableAdvancedDirectFlip"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("OverlayTestMode"), 5u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.d3d.core_settings");
        t.category = QStringLiteral("gpu");
        t.name = QStringLiteral("Direct3D core optimizations");
        t.description = QStringLiteral("Enables flip-no-vsync, non-local video memory usage, small vertex batch optimization, MMX for RGB, and disables debug runtime and device manager.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Some legacy D3D applications may behave differently"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D"), QStringLiteral("DisableVidMemVBs"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D"), QStringLiteral("FlipNoVsync"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D"), QStringLiteral("UseNonLocalVidMem"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D"), QStringLiteral("FullDebug"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D"), QStringLiteral("EnableMultimonDebugging"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D"), QStringLiteral("LoadDebugRuntime"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D"), QStringLiteral("DisableDM"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D"), QStringLiteral("FewVertices"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D"), QStringLiteral("DisableMMX"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D"), QStringLiteral("UseMMXForRGB"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.d3d.directdraw");
        t.category = QStringLiteral("gpu");
        t.name = QStringLiteral("DirectDraw optimizations");
        t.description = QStringLiteral("Configures DirectDraw: enables non-local video memory, disables emulation-only mode and AGP support disable, and disables point sprite / state block emulation.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Legacy DirectDraw games may render with visual artifacts or refuse to launch"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectDraw"), QStringLiteral("EmulationOnly"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectDraw"), QStringLiteral("DisableAGPSupport"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectDraw"), QStringLiteral("UseNonLocalVidMem"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectDraw"), QStringLiteral("DisableDDSCAPSInDDSD"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectDraw"), QStringLiteral("EmulatePointSprites"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectDraw"), QStringLiteral("EmulateStateBlocks"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.d3d.driver_settings");
        t.category = QStringLiteral("gpu");
        t.name = QStringLiteral("Direct3D driver enumeration");
        t.description = QStringLiteral("Configures Direct3D driver enumeration: enables reference, MMX, ramp, and null device enumeration while keeping software-only and forced RGB rasterizer off.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Older D3D9 titles that probe the reference rasterizer take longer to enumerate devices"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D\\Drivers"), QStringLiteral("SoftwareOnly"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D\\Drivers"), QStringLiteral("ForceRgbRasterizer"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D\\Drivers"), QStringLiteral("EnumReference"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D\\Drivers"), QStringLiteral("EnumSeparateMMX"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D\\Drivers"), QStringLiteral("EnumRamp"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D\\Drivers"), QStringLiteral("EnumNullDevice"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D\\Drivers"), QStringLiteral("UseMMXForRGB"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.display.dep_ie_disable");
        t.category = QStringLiteral("gpu");
        t.name = QStringLiteral("Disable DEP for Internet Explorer");
        t.description = QStringLiteral("Disables Data Execution Prevention for Internet Explorer. Originally a performance tweak for legacy IE rendering.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::Critical;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Internet Explorer loses DEP hardware protection, increasing exploit risk on legacy sites"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Internet Explorer\\Main"), QStringLiteral("DEPOff"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.driver.hdcp_disable");
        t.category = QStringLiteral("gpu");
        t.name = QStringLiteral("Disable HDCP");
        t.description = QStringLiteral("Zeros the NVIDIA HDCP key to skip content-protection handshake overhead.");
        t.iconName = QStringLiteral("monitor");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("DRM-protected streaming (Netflix 4K, Disney+) will refuse to play."),
            QStringLiteral("Blu-ray playback may fail."),
            QStringLiteral("UHD protected paths disabled."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("RMHdcpKeyglobZero"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.driver.tdr_disable");
        t.category = QStringLiteral("gpu");
        t.name = QStringLiteral("Disable Timeout Detection and Recovery");
        t.description = QStringLiteral("Fully disables the GPU timeout detection and recovery mechanism. Sets TdrLevel, TdrDelay, TdrDdiDelay, TdrDebugMode, TdrLimitCount, TdrLimitTime, and TdrTestMode all to 0. Prevents Windows from resetting the GPU driver when it stops responding.");
        t.iconName = QStringLiteral("monitor");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("A hung GPU will freeze the entire system instead of recovering the driver"),
            QStringLiteral("Hard reset or power cycle becomes the only exit from a GPU hang"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("TdrLevel"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("TdrDelay"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("TdrDdiDelay"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("TdrDebugMode"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("TdrLimitCount"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("TdrLimitTime"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("TdrTestMode"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.dwm.advanced_compositor");
        t.category = QStringLiteral("gpu");
        t.name = QStringLiteral("Advanced DWM compositor settings");
        t.description = QStringLiteral("Applies an advanced DWM compositor profile for latency and throughput.");
        t.iconName = QStringLiteral("monitor");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("DWM render-thread watchdog disabled - no auto-recovery if compositor hangs."),
            QStringLiteral("Projected shadows, hologram compositor, device bitmaps disabled."),
            QStringLiteral("Desktop overlays disabled."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("FrameLatency"), 2u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("ForceDirectDrawSync"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("MaxQueuedPresentBuffers"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("SuperWetEnabled"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("SDRBoostPercentOverride"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("ResampleInLinearSpace"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("OneCoreNoDWMRawGameController"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("MPCInputRouterWaitForDebugger"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("InteractionOutputPredictionDisabled"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("InkGPUAccelOverrideVendorWhitelist"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("EnableRenderPathTestMode"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("FlattenVirtualSurfaceEffectInput"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("EnableCpuClipping"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("DisallowNonDrawListRendering"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("DisableProjectedShadowsRendering"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("DisableProjectedShadows"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("DisableLockingMemory"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("DisableHologramCompositor"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("DisableDeviceBitmaps"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("DebugFailFast"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("DisplayTestMode"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("UseHWDrawListEntriesOnWARP"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("ResampleModeOverride"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("RenderThreadWatchdogTimeoutMilliseconds"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("ParallelModePolicy"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("EnableResizeOptimization"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("EnableMegaRects"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("EnableFrontBufferRenderChecks"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("EnableEffectCaching"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("EnableDesktopOverlays"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("EnablePrimitiveReordering"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("MaxD3DFeatureLevel"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("OverlayQualifyCount"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("OverlayDisqualifyCount"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("ResizeTimeoutModern"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("ResizeTimeoutGdi"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("HighColor"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("AnimationsShiftKey"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("DisableDrawListCaching"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("AnimationAttributionEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("EnableCommonSuperSets"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("DisableAdvancedDirectFlip"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("OverlayTestMode"), 5u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.dx.advanced_directx");
        t.category = QStringLiteral("gpu");
        t.name = QStringLiteral("Advanced Direct3D / DirectDraw settings");
        t.description = QStringLiteral("Low-level Direct3D, Direct3D\\\\Drivers, and DirectDraw keys.");
        t.iconName = QStringLiteral("monitor");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Overlaps with existing D3dCoreSettings/D3dDirectdraw/D3dDriverSettings tweaks - consider consolidating."),
        };
        t.registryOps = {
            // Not present on a stock install, so revert removes it.
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D"), QStringLiteral("DisableVidMemVBs"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D"), QStringLiteral("FlipNoVsync"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D"), QStringLiteral("UseNonLocalVidMem"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D"), QStringLiteral("FullDebug"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D"), QStringLiteral("EnableMultimonDebugging"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D"), QStringLiteral("LoadDebugRuntime"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D"), QStringLiteral("DisableDM"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D"), QStringLiteral("FewVertices"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D"), QStringLiteral("DisableMMX"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D"), QStringLiteral("UseMMXForRGB"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D\\Drivers"), QStringLiteral("SoftwareOnly"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D\\Drivers"), QStringLiteral("ForceRgbRasterizer"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D\\Drivers"), QStringLiteral("EnumReference"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D\\Drivers"), QStringLiteral("EnumSeparateMMX"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D\\Drivers"), QStringLiteral("EnumRamp"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D\\Drivers"), QStringLiteral("EnumNullDevice"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D\\Drivers"), QStringLiteral("UseMMXForRGB"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectDraw"), QStringLiteral("EmulationOnly"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectDraw"), QStringLiteral("DisableAGPSupport"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectDraw"), QStringLiteral("UseNonLocalVidMem"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectDraw"), QStringLiteral("DisableDDSCAPSInDDSD"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectDraw"), QStringLiteral("EmulatePointSprites"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectDraw"), QStringLiteral("EmulateStateBlocks"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("D3D12_ENABLE_UNSAFE_COMMAND_BUFFER_REUSE"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("D3D12_ENABLE_RUNTIME_DRIVER_OPTIMIZATIONS"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("D3D12_RESOURCE_ALIGNMENT"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("D3D11_MULTITHREADED"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("D3D12_MULTITHREADED"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("D3D11_DEFERRED_CONTEXTS"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("D3D12_DEFERRED_CONTEXTS"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("D3D11_ALLOW_TILING"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("D3D11_ENABLE_DYNAMIC_CODEGEN"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("D3D12_ALLOW_TILING"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("D3D12_CPU_PAGE_TABLE_ENABLED"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("D3D12_HEAP_SERIALIZATION_ENABLED"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("D3D12_MAP_HEAP_ALLOCATIONS"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("D3D12_RESIDENCY_MANAGEMENT_ENABLED"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("DXGI_PREEMPTION_MODE"), 3u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("DXGI_FRAME_LATENCY_WAITABLE_OBJECT"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("DXGI_SWAP_CHAIN_WAITABLE_OBJECT"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("DXGI_FORCE_FLIP_DISCARD"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("DXGI_SWAP_CHAIN_SCALE"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("DXGI_SWAP_CHAIN_ALLOW_MODE_SWITCH"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("DXGI_SWAP_CHAIN_FULLSCREEN_FLIP_MODE"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("DXGI_DISABLE_DWM_THROTTLING"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("DXGI_FORCE_FLIP_SEQUENTIAL"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("DXGI_FORCE_FULLSCREEN_FLIP_MODE"), 3u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("DXGI_MAX_FRAME_LATENCY"), 2u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("DXGI_USE_OPTIMIZED_SWAP_CHAIN"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.dx.dxgkrnl_advanced");
        t.category = QStringLiteral("gpu");
        t.name = QStringLiteral("Advanced DXGKrnl graphics kernel settings");
        t.description = QStringLiteral("Sets DirectX graphics kernel (DXGKrnl) swap-chain, CDD/DWM interop and TDR async flags.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Touches the DirectX graphics kernel service (DXGKrnl) - any misconfiguration can cause boot-time display driver init failures that fall back to Microsoft Basic Display."),
            QStringLiteral("Raising DXGKrnl worker ThreadPriority to 15 (real-time adjacent) can starve other kernel threads under heavy GPU load and has been observed to destabilise older or pre-WDDM-2.7 drivers."),
            QStringLiteral("Enabling TdrResetFromTimeoutAsync changes TDR (Timeout Detection & Recovery) timing - a hung GPU may reset faster, but some driver stacks will bugcheck (VIDEO_TDR_FAILURE) instead of recovering gracefully."),
            QStringLiteral("Swap-chain flags (DxgkCreateSwapChain / DxgkOpenSwapChain / DxgkShareSwapChainObject / SwapChainBackBuffer) alter DWM kernel negotiation; exclusive-fullscreen titles and HDR/VRR paths can regress on hybrid-GPU laptops."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DXGKrnl"), QStringLiteral("CreateGdiPrimaryOnSlaveGPU"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DXGKrnl"), QStringLiteral("DriverSupportsCddDwmInterop"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DXGKrnl"), QStringLiteral("DxgkCddSyncDxAccess"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DXGKrnl"), QStringLiteral("DxgkCddSyncGPUAccess"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DXGKrnl"), QStringLiteral("DxgkCddWaitForVerticalBlankEvent"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DXGKrnl"), QStringLiteral("DxgkCreateSwapChain"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DXGKrnl"), QStringLiteral("DxgkFreeGpuVirtualAddress"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DXGKrnl"), QStringLiteral("DxgkOpenSwapChain"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DXGKrnl"), QStringLiteral("DxgkShareSwapChainObject"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DXGKrnl"), QStringLiteral("DxgkWaitForVerticalBlankEvent"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DXGKrnl"), QStringLiteral("DxgkWaitForVerticalBlankEvent2"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DXGKrnl"), QStringLiteral("SwapChainBackBuffer"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DXGKrnl"), QStringLiteral("TdrResetFromTimeoutAsync"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DXGKrnl\\Parameters"), QStringLiteral("ThreadPriority"), 15u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.dx.multithreading");
        t.category = QStringLiteral("gpu");
        t.name = QStringLiteral("Enable DirectX multithreading optimizations");
        t.description = QStringLiteral("Enables D3D11 tiling and D3D12 command buffer inheritance for better multi-threaded rendering.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Enables a D3D12 command buffer path the runtime labels as unsafe, which can surface driver or engine bugs"),
            QStringLiteral("A misbehaving game may crash or render incorrectly after applying"),
            QStringLiteral("If you see new graphical glitches, reverting this tweak is the first thing to try"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("D3D12_ENABLE_UNSAFE_COMMAND_BUFFER_INHERITANCE"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("D3D12_ENABLE_RUNTIME_DRIVER_OPTIMIZATIONS"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("D3D12_RESOURCE_ALIGNMENT"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("D3D11_MULTITHREADED"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("D3D12_MULTITHREADED"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("D3D11_DEFERRED_CONTEXTS"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("D3D12_DEFERRED_CONTEXTS"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("D3D11_ALLOW_TILING"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("D3D11_ENABLE_DYNAMIC_CODEGEN"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("D3D12_ALLOW_TILING"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("D3D12_CPU_PAGE_TABLE_ENABLED"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("D3D12_HEAP_SERIALIZATION_ENABLED"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("D3D12_MAP_HEAP_ALLOCATIONS"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("D3D12_RESIDENCY_MANAGEMENT_ENABLED"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("DXGI_PREEMPTION_MODE"), 3u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("DXGI_FRAME_LATENCY_WAITABLE_OBJECT"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("DXGI_SWAP_CHAIN_WAITABLE_OBJECT"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("DXGI_FORCE_FLIP_DISCARD"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("DXGI_SWAP_CHAIN_SCALE"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("DXGI_SWAP_CHAIN_ALLOW_MODE_SWITCH"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("DXGI_SWAP_CHAIN_FULLSCREEN_FLIP_MODE"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("DXGI_DISABLE_DWM_THROTTLING"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("DXGI_FORCE_FLIP_SEQUENTIAL"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("DXGI_FORCE_FULLSCREEN_FLIP_MODE"), 3u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("DXGI_MAX_FRAME_LATENCY"), 2u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("DXGI_USE_OPTIMIZED_SWAP_CHAIN"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.gfxdrv.scheduling_overlays_pm");
        t.category = QStringLiteral("gpu");
        t.name = QStringLiteral("GraphicsDrivers scheduling and power optimization");
        t.description = QStringLiteral("Comprehensive GraphicsDrivers optimization: sets scheduling delay to zero, disables hardware overlays, disables runtime power management and PoFx power controls, disables content protection (OPM), eliminates GPU idle states, disables driver protection, and disables DXGK idle wait.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("HDCP/PlayReady protected playback (Netflix 4K, Blu-ray) will refuse to play"),
            QStringLiteral("GPU stays in high power state; idle temperature and fan noise rise"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("SchedulingDelay"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("DisableOverlays"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("SupportRuntimePowerManagement"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("Protection"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("ProtectionLevel"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("OPMSetProtectionLevel"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("NumberOfIdleStates"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("DriverProtection"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("PoFxStartDevicePowerManagement"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("PoFxPowerControl"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("DxgkWaitForIdle"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("RuntimePowerManagement"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("RMOPSB"), 10914u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("VRDirectFlipDPCDelayUsy"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.nvidia.disable_write_combining");
        t.category = QStringLiteral("gpu");
        t.name = QStringLiteral("NVIDIA disable write combining");
        t.description = QStringLiteral("Disables write combining on the NVIDIA GPU driver. Write combining batches multiple small writes into a single bus transaction; disabling it can reduce latency for certain workloads at the cost of throughput.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("GPU memory bandwidth drops; texture-heavy workloads and CUDA transfers slow down"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\nvlddmkm"), QStringLiteral("DisableWriteCombining"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.nvidia.dram_pll_gen4");
        t.category = QStringLiteral("gpu");
        t.name = QStringLiteral("NVIDIA DRAM PLL and Gen4 copy engine");
        t.description = QStringLiteral("Keeps the VRAM DRAM PLL always-on and enables Gen4 copy-engine mapping on NVIDIA GPUs.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Slightly higher idle power."),
            QStringLiteral("Gen4 mapping only helps on PCIe 4.0 systems."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\"), QStringLiteral("RmClkPowerOffDramPllWhenUnused"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\"), QStringLiteral("RmCeUseGen4Mapping"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.nvidia.kboost");
        t.category = QStringLiteral("gpu");
        t.name = QStringLiteral("NVIDIA K-Boost");
        t.description = QStringLiteral("Forces NVIDIA PowerMizer to maximum performance state across all GPU adapters.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Card runs at max clocks - higher temperature and power."),
            QStringLiteral("Louder fan noise."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\"), QStringLiteral("PowerMizerEnable"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.nvidia.powermizer_level");
        t.category = QStringLiteral("gpu");
        t.name = QStringLiteral("NVIDIA PowerMizer: prefer max performance");
        t.description = QStringLiteral("Sets the NVIDIA PowerMizer level to prefer maximum performance across all GPU adapters.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Higher idle power."),
            QStringLiteral("Higher idle GPU temperature."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\"), QStringLiteral("PowerMizerLevel"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.nvidia.preemption_disable");
        t.category = QStringLiteral("gpu");
        t.name = QStringLiteral("Disable GPU preemption");
        t.description = QStringLiteral("Disables GPU preemption to prevent the scheduler from interrupting GPU workloads mid-execution.");
        t.iconName = QStringLiteral("shield-off");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Desktop, cursor, and overlay apps can briefly hang during heavy GPU load because Windows cannot preempt the GPU"),
            QStringLiteral("Hangs long enough to trigger the Windows GPU recovery (TDR) can look like a driver crash"),
            QStringLiteral("A reboot is required for the change to take effect"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ControlSet001\\Services\\nvlddmkm"), QStringLiteral("DisablePreemption"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ControlSet001\\Services\\nvlddmkm"), QStringLiteral("DisableCudaContextPreemption"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ControlSet001\\Services\\nvlddmkm"), QStringLiteral("EnableCEPreemption"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ControlSet001\\Services\\nvlddmkm"), QStringLiteral("DisablePreemptionOnS3S4"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ControlSet001\\Services\\nvlddmkm"), QStringLiteral("ComputePreemption"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ControlSet001\\Services\\nvlddmkm"), QStringLiteral("EnableMidGfxPreemption"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ControlSet001\\Services\\nvlddmkm"), QStringLiteral("EnableMidGfxPreemptionVGPU"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ControlSet001\\Services\\nvlddmkm"), QStringLiteral("EnableMidBufferPreemptionForHighTdrTimeout"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ControlSet001\\Services\\nvlddmkm"), QStringLiteral("EnableMidBufferPreemption"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ControlSet001\\Services\\nvlddmkm"), QStringLiteral("EnableAsyncMidBufferPreemption"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ControlSet001\\Services\\nvlddmkm"), QStringLiteral("RmGpsPsEnablePerCpuCoreDpc"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Scheduler"), QStringLiteral("EnablePreemption"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.nvidia.profile_optimal");
        t.category = QStringLiteral("gpu");
        // Writes the eight driver registry values below. Texture filtering, shader
        // cache and threaded optimization live in the NVIDIA profile database, which
        // nvapi::optimalProfile on the GPU page writes, not this tweak.
        t.name = QStringLiteral("NVIDIA driver power and preemption");
        t.description = QStringLiteral("Pins the NVIDIA driver out of adaptive power management (PowerMizer) so clocks stay up, stops the GPU preempting work already in flight including CUDA contexts, disables write-combining in the resource manager, and turns off driver tracing. It does not touch texture filtering, shader cache or vertical sync - those live in your NVIDIA Control Panel profile, which the separate Optimal NVIDIA Profile tool writes.");
        t.iconName = QStringLiteral("sliders-horizontal");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            // Only the power management mode overlaps the NVIDIA Control Panel.
            QStringLiteral("Overrides the power management mode set in the NVIDIA Control Panel"),
            QStringLiteral("Locks the GPU near peak clocks, so power draw, heat, and fan noise stay elevated"),
            QStringLiteral("A reboot is required and future NVIDIA driver updates may reset or conflict with these keys"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\nvlddmkm"), QStringLiteral("DisablePreemption"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\nvlddmkm"), QStringLiteral("DisablePreemptionOnS3S4"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\nvlddmkm"), QStringLiteral("DisableCudaContextPreemption"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\nvlddmkm"), QStringLiteral("TraceLevel"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\NVIDIA Corporation\\Global\\NVTweak"), QStringLiteral("RaisePowerMisc"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\NVIDIA Corporation\\Global"), QStringLiteral("PowerMizerEnable"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\NVIDIA Corporation\\Global"), QStringLiteral("PowerMizerLevel"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\NVIDIA Corporation\\Global"), QStringLiteral("PerfLevelSrc"), 0x2222u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.nvidia.sig_check_bypass");
        t.category = QStringLiteral("gpu");
        t.name = QStringLiteral("NVIDIA driver signature check bypass");
        t.description = QStringLiteral("Sets the NVIDIA internal GUID flag that bypasses driver signature verification. Allows unsigned or modified NVIDIA drivers to load. Useful for custom driver builds and testing.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Unsigned drivers may be unstable"),
            QStringLiteral("Bypasses security verification"),
            QStringLiteral("vanguard"),
            QStringLiteral("faceit"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\NVIDIA Corporation\\Global"), QStringLiteral("{41FCC608-8496-4DEF-B43E-7D9BD675A6FF}"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ControlSet001\\Services\\nvlddmkm"), QStringLiteral("{41FCC608-8496-4DEF-B43E-7D9BD675A6FF}"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.nvidia.write_combining_disable");
        t.category = QStringLiteral("gpu");
        t.name = QStringLiteral("Disable NVIDIA write combining");
        t.description = QStringLiteral("Disables CPU write-combining on NVIDIA GPU memory for lower per-write latency.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Minor FPS decrease in GPU-bound scenarios."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), QStringLiteral("RMDisableWriteCombining"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("gpu.perf.resource_policy");
        t.category = QStringLiteral("gpu");
        t.name = QStringLiteral("Multimedia resource policy");
        t.description = QStringLiteral("Removes the multimedia reservation and disables network throttling for foreground parity.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Background processes compete equally for CPU."),
            QStringLiteral("System may feel less responsive under heavy load."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile"), QStringLiteral("SystemResponsiveness"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile"), QStringLiteral("NetworkThrottlingIndex"), 0xffffffffu, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\CPU\\HardCap0"), QStringLiteral("CapPercentage"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\CPU\\Paused"), QStringLiteral("CapPercentage"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\CPU\\SoftCapFull"), QStringLiteral("CapPercentage"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\CPU\\SoftCapLow"), QStringLiteral("CapPercentage"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\CPU\\HardCap0"), QStringLiteral("SchedulingType"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\CPU\\Paused"), QStringLiteral("SchedulingType"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\CPU\\SoftCapFull"), QStringLiteral("SchedulingType"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\CPU\\SoftCapLow"), QStringLiteral("SchedulingType"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\Flags\\BackgroundDefault"), QStringLiteral("IsLowPriority"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\Flags\\Frozen"), QStringLiteral("IsLowPriority"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\Flags\\FrozenDNCS"), QStringLiteral("IsLowPriority"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\Flags\\FrozenDNK"), QStringLiteral("IsLowPriority"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\Flags\\FrozenPPLE"), QStringLiteral("IsLowPriority"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\Flags\\Paused"), QStringLiteral("IsLowPriority"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\Flags\\PausedDNK"), QStringLiteral("IsLowPriority"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\Flags\\Pausing"), QStringLiteral("IsLowPriority"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\Flags\\PrelaunchForeground"), QStringLiteral("IsLowPriority"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\Flags\\ThrottleGPUInterference"), QStringLiteral("IsLowPriority"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\Importance\\Critical"), QStringLiteral("BasePriority"), 82u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\Importance\\CriticalNoUi"), QStringLiteral("BasePriority"), 82u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\Importance\\EmptyHostPPLE"), QStringLiteral("BasePriority"), 82u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\Importance\\High"), QStringLiteral("BasePriority"), 82u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\Importance\\Low"), QStringLiteral("BasePriority"), 82u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\Importance\\Lowest"), QStringLiteral("BasePriority"), 82u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\Importance\\Medium"), QStringLiteral("BasePriority"), 82u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\Importance\\MediumHigh"), QStringLiteral("BasePriority"), 82u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\Importance\\StartHost"), QStringLiteral("BasePriority"), 82u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\Importance\\VeryHigh"), QStringLiteral("BasePriority"), 82u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\Importance\\VeryLow"), QStringLiteral("BasePriority"), 82u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\Importance\\Critical"), QStringLiteral("OverTargetPriority"), 50u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\Importance\\CriticalNoUi"), QStringLiteral("OverTargetPriority"), 50u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\Importance\\EmptyHostPPLE"), QStringLiteral("OverTargetPriority"), 50u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\Importance\\High"), QStringLiteral("OverTargetPriority"), 50u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\Importance\\Low"), QStringLiteral("OverTargetPriority"), 50u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\Importance\\Lowest"), QStringLiteral("OverTargetPriority"), 50u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\Importance\\Medium"), QStringLiteral("OverTargetPriority"), 50u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\Importance\\MediumHigh"), QStringLiteral("OverTargetPriority"), 50u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\Importance\\StartHost"), QStringLiteral("OverTargetPriority"), 50u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\Importance\\VeryHigh"), QStringLiteral("OverTargetPriority"), 50u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\Importance\\VeryLow"), QStringLiteral("OverTargetPriority"), 50u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\IO\\NoCap"), QStringLiteral("IOBandwidth"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\Memory\\NoCap"), QStringLiteral("CommitLimit"), 0xffffffffu, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\Policies\\Memory\\NoCap"), QStringLiteral("CommitTarget"), 0xffffffffu, std::nullopt),
        };
        c.push_back(t);
    }
}

} // namespace vth::tweaks::detail
