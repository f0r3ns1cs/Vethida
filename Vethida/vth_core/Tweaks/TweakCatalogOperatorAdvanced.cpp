#include "TweakEngine.h"

namespace vth::tweaks::detail
{
    namespace
    {
        // A value the machine ships without: applied on apply, deleted on revert.
        RegistryOp del(const QString& path, const QString& name, quint32 v) {
            return dwordOp(path, name, v, std::nullopt);
        }

        struct NV { const char* name; quint32 value; };

        const QString kSmKernel  = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Kernel");
        const QString kSmMemory  = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management");
        const QString kDwm       = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM");
        const QString kIfeoBase  = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\");
        const QString kD3D       = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D");
        const QString kDDraw     = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectDraw");
        const QString kDirectX   = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX");
        const QString kDxgk      = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DXGKrnl");
    }

    void appendOperatorAdvanced(QVector<TweakSpec>& c) {

        {
            TweakSpec t;
            t.id = QStringLiteral("sec.disable-exploit-mitigations");
            t.category = QStringLiteral("security");
            t.subCategory = QStringLiteral("Exploit protection");
            t.name = QStringLiteral("Disable exploit protections");
            t.description = QStringLiteral(
                "Turns off a group of Windows exploit defenses at once: control-flow "
                "guarding, the exception-handler checks, the kernel system-call filter, "
                "virtualization-based security with its memory integrity, and the "
                "protection against devices reading memory while the PC is locked.");
            t.iconName = QStringLiteral("shield-off");
            t.impact = Impact::Critical;
            t.askFirst = true;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Removes defenses that stop malicious programs and devices from taking over the PC. Only for a PC that runs nothing untrusted."),
                QStringLiteral("Turns off memory integrity, which some anti-cheat and work software require."),
            };
            t.concern = QStringLiteral(
                "a broad security downgrade: "
                "disables CFG (EnableCfg), user- and kernel-mode SEHOP "
                "(DisableExceptionChainValidation / KernelSEHOPEnabled), the win32k syscall "
                "filter (DisallowWin32kSystemCalls), VBS + HVCI "
                "(EnableVirtualizationBasedSecurity / HVCIMATRequired), Kernel DMA protection "
                "(DmaGuard Enable / DisableExternalDMAUnderLock) and ELAM boot-driver vetting "
                "(DriverLoadPolicy). Never suggest unprompted.");
            t.registryOps = {
                del(kSmKernel, QStringLiteral("DisallowWin32kSystemCalls"), 1u),
                dwordOp(kSmKernel, QStringLiteral("KernelSEHOPEnabled"), 0u, 1u),
                dwordOp(kSmKernel, QStringLiteral("DisableExceptionChainValidation"), 1u, 0u),
                del(kSmMemory, QStringLiteral("EnableCfg"), 0u),
                del(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\DeviceGuard"),
                    QStringLiteral("EnableVirtualizationBasedSecurity"), 0u),
                del(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DeviceGuard"),
                    QStringLiteral("HVCIMATRequired"), 0u),
                del(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\DeviceGuard\\Scenarios\\DmaGuard"),
                    QStringLiteral("Enable"), 0u),
                del(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\FVE"),
                    QStringLiteral("DisableExternalDMAUnderLock"), 0u),
                del(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\EarlyLaunch"),
                    QStringLiteral("DriverLoadPolicy"), 0u),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("perf.disable-paging-executive");
            t.category = QStringLiteral("advanced");
            t.subCategory = QStringLiteral("Memory");
            t.name = QStringLiteral("Keep the kernel in memory");
            t.description = QStringLiteral(
                "Stops Windows from moving core operating-system code out of memory "
                "and onto the disk, so it is always resident. Helps a machine with "
                "plenty of memory and can hurt one that is short of it.");
            t.iconName = QStringLiteral("cpu");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "DisablePagingExecutive - a long-standing tweak with a small, RAM-dependent "
                "effect; on a memory-tight machine it makes paging pressure worse.");
            t.registryOps = {
                dwordOp(kSmMemory, QStringLiteral("DisablePagingExecutive"), 1u, 0u),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("perf.fault-tolerant-heap-off");
            t.category = QStringLiteral("advanced");
            t.subCategory = QStringLiteral("Reliability");
            t.name = QStringLiteral("Disable the crash-slowdown shim");
            t.description = QStringLiteral(
                "Turns off the Windows feature that quietly slows a program down "
                "after it has crashed repeatedly, in the hope of keeping it stable. "
                "A program that was being held back this way runs at full speed "
                "again; one that was crashing may crash again.");
            t.iconName = QStringLiteral("activity");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.warnings = {
                QStringLiteral("A program Windows was keeping stable can start crashing again."),
            };
            t.concern = QStringLiteral(
                "FTH Enabled=0 - removes an automatic stability workaround. "
                "Default is on; revert restores it.");
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\FTH"),
                        QStringLiteral("Enabled"), 0u, 1u),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.dwm-advanced");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("Desktop composition");
            t.name = QStringLiteral("Desktop compositor overrides");
            t.description = QStringLiteral(
                "Shortens the desktop compositor's frame queue, raises the priority of the "
                "compositor and the core system process, turns off projected shadows, "
                "holograms and desktop overlays, and sets a long list of internal rendering"
                " switches. Many of them are undocumented.");
            t.iconName = QStringLiteral("monitor");
            t.impact = Impact::High;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Some visual effects and overlays stop appearing, and a few apps can draw incorrectly."),
                QStringLiteral("On a PC with few cores, the raised system priority can slow other work."),
            };
            t.concern = QStringLiteral(
                "Many of these DWM values are undocumented or are "
                "debug/test flags being forced to their default-off state (DebugFailFast, "
                "DisplayTestMode, EnableRenderPathTestMode); a handful are real behavioral "
                "changes (MaxQueuedPresentBuffers, DisableAdvancedDirectFlip, MaxD3DFeatureLevel, "
                "the projected-shadow/overlay disables). Effect unmeasured, and the csrss/dwm "
                "IoPriority/CpuPriorityClass boosts are aggressive. Never suggest unprompted.");

            QVector<RegistryOp> o;
            o.push_back(del(kIfeoBase + QStringLiteral("dwm.exe\\PerfOptions"),
                            QStringLiteral("IoPriority"), 3u));
            o.push_back(del(kIfeoBase + QStringLiteral("csrss.exe\\PerfOptions"),
                            QStringLiteral("CpuPriorityClass"), 4u));
            o.push_back(del(kIfeoBase + QStringLiteral("csrss.exe\\PerfOptions"),
                            QStringLiteral("IoPriority"), 4u));
            static const NV kDwmValues[] = {
                { "FrameLatency", 2 }, { "ForceDirectDrawSync", 0 },
                { "MaxQueuedPresentBuffers", 1 }, { "SuperWetEnabled", 1 },
                { "SDRBoostPercentOverride", 1 }, { "ResampleInLinearSpace", 1 },
                { "OneCoreNoDWMRawGameController", 0 }, { "MPCInputRouterWaitForDebugger", 0 },
                { "InteractionOutputPredictionDisabled", 1 },
                { "InkGPUAccelOverrideVendorWhitelist", 1 }, { "EnableRenderPathTestMode", 0 },
                { "FlattenVirtualSurfaceEffectInput", 1 }, { "EnableCpuClipping", 1 },
                { "DisallowNonDrawListRendering", 1 }, { "DisableProjectedShadowsRendering", 1 },
                { "DisableProjectedShadows", 1 }, { "DisableLockingMemory", 1 },
                { "DisableHologramCompositor", 1 }, { "DisableDeviceBitmaps", 1 },
                { "DebugFailFast", 0 }, { "DisplayTestMode", 0 },
                { "UseHWDrawListEntriesOnWARP", 1 }, { "ResampleModeOverride", 1 },
                { "RenderThreadWatchdogTimeoutMilliseconds", 0 }, { "ParallelModePolicy", 1 },
                { "EnableResizeOptimization", 1 }, { "EnableMegaRects", 1 },
                { "EnableFrontBufferRenderChecks", 0 }, { "EnableEffectCaching", 1 },
                { "EnableDesktopOverlays", 0 }, { "EnablePrimitiveReordering", 0 },
                { "MaxD3DFeatureLevel", 0 }, { "OverlayQualifyCount", 0 },
                { "OverlayDisqualifyCount", 0 }, { "ResizeTimeoutModern", 0 },
                { "ResizeTimeoutGdi", 0 }, { "HighColor", 0 }, { "AnimationsShiftKey", 0 },
                { "DisableDrawListCaching", 1 }, { "AnimationAttributionEnabled", 0 },
                { "EnableCommonSuperSets", 1 }, { "DisableAdvancedDirectFlip", 1 },
            };
            for (const NV& v : kDwmValues)
                o.push_back(del(kDwm, QString::fromLatin1(v.name), v.value));
            t.registryOps = o;
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.direct3d-advanced");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("Graphics runtime");
            t.name = QStringLiteral("Graphics runtime overrides");
            t.description = QStringLiteral(
                "Across the older and current Direct3D layers and the graphics kernel: "
                "prefers flip-based presentation, turns on multi-threaded and deferred "
                "rendering, and sets a long list of runtime and swap-chain flags. Many of "
                "them are undocumented.");
            t.iconName = QStringLiteral("gpu");
            t.impact = Impact::High;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("A game that draws wrong or will not start after this should be checked with it turned off first."),
            };
            t.concern = QStringLiteral(
                "spans Direct3D, DirectDraw, DirectX (the "
                "D3D11/D3D12/DXGI_* flags) and the DXGKrnl service. Several are unsafe-by-name "
                "(D3D12_ENABLE_UNSAFE_COMMAND_BUFFER_REUSE, DXGI_FORCE_FULLSCREEN_FLIP_MODE=3, "
                "MaxD3DFeatureLevel territory) and many are legacy no-ops on current Windows. "
                "Effect unmeasured. Never suggest unprompted.");

            QVector<RegistryOp> o;
            static const NV kD3DValues[] = {
                { "DisableVidMemVBs", 0 }, { "FlipNoVsync", 1 }, { "UseNonLocalVidMem", 1 },
                { "FullDebug", 0 }, { "EnableMultimonDebugging", 0 }, { "LoadDebugRuntime", 0 },
                { "DisableDM", 1 }, { "FewVertices", 1 }, { "DisableMMX", 0 },
                { "UseMMXForRGB", 1 },
            };
            for (const NV& v : kD3DValues)
                o.push_back(del(kD3D, QString::fromLatin1(v.name), v.value));
            static const NV kD3DDriverValues[] = {
                { "SoftwareOnly", 0 }, { "ForceRgbRasterizer", 0 }, { "EnumReference", 1 },
                { "EnumSeparateMMX", 1 }, { "EnumRamp", 1 }, { "EnumNullDevice", 1 },
                { "UseMMXForRGB", 1 },
            };
            for (const NV& v : kD3DDriverValues)
                o.push_back(del(kD3D + QStringLiteral("\\Drivers"), QString::fromLatin1(v.name), v.value));
            static const NV kDDrawValues[] = {
                { "EmulationOnly", 0 }, { "DisableAGPSupport", 0 }, { "UseNonLocalVidMem", 1 },
                { "DisableDDSCAPSInDDSD", 0 }, { "EmulatePointSprites", 0 },
                { "EmulateStateBlocks", 0 },
            };
            for (const NV& v : kDDrawValues)
                o.push_back(del(kDDraw, QString::fromLatin1(v.name), v.value));
            static const NV kDirectXValues[] = {
                { "D3D12_ENABLE_UNSAFE_COMMAND_BUFFER_REUSE", 1 },
                { "D3D12_ENABLE_RUNTIME_DRIVER_OPTIMIZATIONS", 1 },
                { "D3D12_RESOURCE_ALIGNMENT", 1 }, { "D3D11_MULTITHREADED", 1 },
                { "D3D12_MULTITHREADED", 1 }, { "D3D11_DEFERRED_CONTEXTS", 1 },
                { "D3D12_DEFERRED_CONTEXTS", 1 }, { "D3D11_ALLOW_TILING", 1 },
                { "D3D11_ENABLE_DYNAMIC_CODEGEN", 1 }, { "D3D12_ALLOW_TILING", 1 },
                { "D3D12_CPU_PAGE_TABLE_ENABLED", 1 }, { "D3D12_HEAP_SERIALIZATION_ENABLED", 1 },
                { "D3D12_MAP_HEAP_ALLOCATIONS", 1 }, { "D3D12_RESIDENCY_MANAGEMENT_ENABLED", 1 },
                { "DXGI_PREEMPTION_MODE", 3 }, { "DXGI_FRAME_LATENCY_WAITABLE_OBJECT", 1 },
                { "DXGI_SWAP_CHAIN_WAITABLE_OBJECT", 1 }, { "DXGI_FORCE_FLIP_DISCARD", 1 },
                { "DXGI_SWAP_CHAIN_SCALE", 1 }, { "DXGI_SWAP_CHAIN_ALLOW_MODE_SWITCH", 1 },
                { "DXGI_SWAP_CHAIN_FULLSCREEN_FLIP_MODE", 1 }, { "DXGI_DISABLE_DWM_THROTTLING", 1 },
                { "DXGI_FORCE_FLIP_SEQUENTIAL", 1 }, { "DXGI_FORCE_FULLSCREEN_FLIP_MODE", 3 },
                { "DXGI_MAX_FRAME_LATENCY", 2 }, { "DXGI_USE_OPTIMIZED_SWAP_CHAIN", 1 },
            };
            for (const NV& v : kDirectXValues)
                o.push_back(del(kDirectX, QString::fromLatin1(v.name), v.value));
            static const NV kDxgkValues[] = {
                { "CreateGdiPrimaryOnSlaveGPU", 1 }, { "DriverSupportsCddDwmInterop", 1 },
                { "DxgkCddSyncDxAccess", 1 }, { "DxgkCddSyncGPUAccess", 1 },
                { "DxgkCddWaitForVerticalBlankEvent", 1 }, { "DxgkCreateSwapChain", 1 },
                { "DxgkFreeGpuVirtualAddress", 1 }, { "DxgkOpenSwapChain", 1 },
                { "DxgkShareSwapChainObject", 1 }, { "DxgkWaitForVerticalBlankEvent", 1 },
                { "DxgkWaitForVerticalBlankEvent2", 1 }, { "SwapChainBackBuffer", 1 },
                { "TdrResetFromTimeoutAsync", 1 },
            };
            for (const NV& v : kDxgkValues)
                o.push_back(del(kDxgk, QString::fromLatin1(v.name), v.value));
            t.registryOps = o;
            c.push_back(t);
        }
    }
}

