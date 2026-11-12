#include "TweakEngine.h"

// The DirectX runtime, Direct3D/DXGI, GraphicsDrivers latency and DWM
// frame-queue surface that the other catalogs didn't already cover. Values that
// are Windows defaults, or
// already shipped elsewhere (TdrLevel/TdrDelay/FlipQueueSize/HAGS/MPO in
// TweakCatalogGpu, AllowTearing/ForceFlipDiscard/FrameLatency), are not repeated.


namespace vth::tweaks::detail {

    namespace {
        void addD(QVector<RegistryOp>& o, const QString& p, const QString& n, quint32 v,
                std::optional<quint32> rev = std::nullopt) {
            o.push_back(dwordOp(p, n, v, rev));
        }
        const QString kDirectX = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX");
        const QString kDirect3D = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D");
        const QString kDxgi = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX\\DXGI");
        const QString kGdrv = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers");
        const QString kDwm = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\Dwm");
    }

    void appendGraphics2(QVector<TweakSpec>& c) {
        {
            TweakSpec t;
            t.id = QStringLiteral("appear.dwm-flat");
            t.category = QStringLiteral("appearance");
            t.subCategory = QStringLiteral("Visual effects");
            t.name = QStringLiteral("Disable DWM animations and transparency");
            t.description = QStringLiteral(
                "Turns off Desktop Window Manager animations, Aero Peek, window blur, "
                "transparency and the accent gradient for a flat, zero-compositor-effect look.");
            t.iconName = QStringLiteral("sparkles");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "preference, not performance: this strips the premium Windows look. Negligible "
                "measured gain; only apply if you actually want a flat theme.");
            QVector<RegistryOp> o;
            addD(o, kDwm, QStringLiteral("DisallowAnimations"), 1, 0);
            addD(o, kDwm, QStringLiteral("EnableAeroPeek"), 0, 1);
            addD(o, kDwm, QStringLiteral("DisableWindowBlur"), 1, 0);
            addD(o, kDwm, QStringLiteral("Blur"), 0);
            addD(o, kDwm, QStringLiteral("DisableAccentGradient"), 1, 0);
            addD(o, QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"),
                QStringLiteral("EnableTransparency"), 0, 1);
            t.registryOps = o;
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.directx-runtime");
            t.category = QStringLiteral("gpu");
            t.name = QStringLiteral("DirectX runtime optimizations");
            t.description = QStringLiteral(
                "Enables multithreaded DirectX, deferred contexts, texture tiling, dynamic "
                "code generation and the D3D11/D3D12 runtime-optimization flags, plus flip-"
                "without-vsync and a deeper command queue.");
            t.iconName = QStringLiteral("monitor");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "placebo-leaning: most of these are advisory hints the app/driver chooses to "
                "honor or ignore; global registry values rarely override per-app behavior. "
                "Harmless, low expected effect.");
            QVector<RegistryOp> o;
            for (const QString& n : {
                QStringLiteral("MULTITHREADED"), QStringLiteral("DEFERRED_CONTEXTS"),
                QStringLiteral("ALLOW_TILING"), QStringLiteral("ENABLE_DYNAMIC_CODEGEN"),
                QStringLiteral("D3D11_ALLOW_TILING"), QStringLiteral("D3D11_ENABLE_DEFERRED_CONTEXTS"),
                QStringLiteral("D3D11_MULTITHREADED"), QStringLiteral("D3D11_ENABLE_DYNAMIC_CODEGEN"),
                QStringLiteral("D3D12_ALLOW_TILING"), QStringLiteral("D3D12_ENABLE_DYNAMIC_CODEGEN"),
                QStringLiteral("D3D12_MULTITHREADED"), QStringLiteral("D3D12_DEFERRED_CONTEXTS"),
                QStringLiteral("D3D12_ENABLE_UNSAFE_COMMAND_BUFFER_REUSE"),
                QStringLiteral("D3D12_ENABLE_RUNTIME_DRIVER_OPTIMIZATIONS"),
                QStringLiteral("D3D12_RESOURCE_ALIGNMENT"), QStringLiteral("D3D12_CPU_PAGE_TABLE_ENABLED"),
                QStringLiteral("D3D12_HEAP_SERIALIZATION_ENABLED"), QStringLiteral("D3D12_MAP_HEAP_ALLOCATIONS"),
                QStringLiteral("D3D12_RESIDENCY_MANAGEMENT_ENABLED") })
                addD(o, kDirectX, n, 1);
            addD(o, kDirect3D, QStringLiteral("FlipNoVsync"), 1);
            addD(o, kDirect3D, QStringLiteral("MaxCommandsInQueue"), 32);
            addD(o, kDxgi, QStringLiteral("DisableMaxFrameLatency"), 1);
            addD(o, kDxgi, QStringLiteral("SwapEffect"), 4);
            t.registryOps = o;
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.graphicsdrivers-latency");
            t.category = QStringLiteral("gpu");
            t.name = QStringLiteral("GPU scheduler latency");
            t.description = QStringLiteral(
                "Forces the direct-flip path, contiguous IOMMU/system-memory mapping, and zero "
                "scheduler/vsync idle delay on the WDDM graphics scheduler; extends the DDI "
                "timeout so long renders don't trip a reset.");
            t.iconName = QStringLiteral("monitor");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "effects are largely unmeasured; safe but expect little. TdrDdiDelay=60 means a "
                "genuinely hung GPU takes longer to recover.");
            QVector<RegistryOp> o;
            addD(o, kGdrv, QStringLiteral("TdrDdiDelay"), 60);
            addD(o, kGdrv, QStringLiteral("ForceDirectFlip"), 1);
            addD(o, kGdrv, QStringLiteral("DpiMapIommuContiguous"), 1);
            addD(o, kGdrv, QStringLiteral("PreferSystemMemoryContiguous"), 1);
            addD(o, kGdrv, QStringLiteral("UseFlipToReduceLatency"), 1);
            addD(o, kGdrv, QStringLiteral("SchedulingDelay"), 0);
            addD(o, kGdrv, QStringLiteral("VsyncIdleTimeout"), 0);
            addD(o, kGdrv, QStringLiteral("MonitorLatencyTolerance"), 0);
            addD(o, kGdrv, QStringLiteral("MonitorRefreshLatencyTolerance"), 0);
            t.registryOps = o;
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.dwm-latency");
            t.category = QStringLiteral("gpu");
            t.name = QStringLiteral("DWM compositor latency reduction");
            t.description = QStringLiteral(
                "Shrinks the Desktop Window Manager's present/frame queues to their minimum and "
                "disables OS-level pointer prediction and DWM-side input processing for the "
                "lowest compositor latency in borderless/windowed games.");
            t.iconName = QStringLiteral("monitor");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "helps borderless/windowed; irrelevant in exclusive fullscreen (DWM is out of "
                "the present path). Minimum queues can cost 1-2% throughput in GPU-bound scenes.");
            QVector<RegistryOp> o;
            addD(o, kDwm, QStringLiteral("MaxQueuedPresentBuffers"), 1, 2);
            addD(o, kDwm, QStringLiteral("MaxQueuedFrames"), 1);
            addD(o, kDwm, QStringLiteral("MaxQueuedPresentStallCount"), 1);
            addD(o, kDwm, QStringLiteral("MaximumPrePresentStalls"), 1);
            addD(o, kDwm, QStringLiteral("InteractionOutputPredictionDisabled"), 1);
            addD(o, kDwm, QStringLiteral("EnableDWMInputProcessing"), 0, 1);
            addD(o, kDwm, QStringLiteral("ForceDirectPresent"), 1);
            t.registryOps = o;
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.dwm-internal-flags");
            t.category = QStringLiteral("gpu");
            t.name = QStringLiteral("DWM internal rendering flags");
            t.description = QStringLiteral(
                "Writes a large set of DWM's internal (undocumented) rendering flags: "
                "resize/effect caching, draw-list handling, front-buffer checks, watchdog "
                "timeouts and render-path test switches.");
            t.iconName = QStringLiteral("monitor");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "unverified: these are internal DWM debug/tuning flags with no Microsoft "
                "documentation and no published measurement. Several read as diagnostic "
                "switches (DisplayTestMode, EnableRenderPathTestMode, DebugFailFast) rather "
                "than performance knobs. Treat as experimental; keep only if you can measure "
                "a difference.");
            QVector<RegistryOp> o;
            addD(o, kDwm, QStringLiteral("FrameLatency"), 2);
            addD(o, kDwm, QStringLiteral("ForceDirectDrawSync"), 0);
            addD(o, kDwm, QStringLiteral("SuperWetEnabled"), 1);
            addD(o, kDwm, QStringLiteral("SDRBoostPercentOverride"), 1);
            addD(o, kDwm, QStringLiteral("ResampleInLinearSpace"), 1);
            addD(o, kDwm, QStringLiteral("OneCoreNoDWMRawGameController"), 0);
            addD(o, kDwm, QStringLiteral("MPCInputRouterWaitForDebugger"), 0);
            addD(o, kDwm, QStringLiteral("InkGPUAccelOverrideVendorWhitelist"), 1);
            addD(o, kDwm, QStringLiteral("EnableRenderPathTestMode"), 0);
            addD(o, kDwm, QStringLiteral("FlattenVirtualSurfaceEffectInput"), 1);
            addD(o, kDwm, QStringLiteral("EnableCpuClipping"), 1);
            addD(o, kDwm, QStringLiteral("DisallowNonDrawListRendering"), 1);
            addD(o, kDwm, QStringLiteral("DisableProjectedShadowsRendering"), 1);
            addD(o, kDwm, QStringLiteral("DisableProjectedShadows"), 1);
            addD(o, kDwm, QStringLiteral("DisableLockingMemory"), 1);
            addD(o, kDwm, QStringLiteral("DisableHologramCompositor"), 1);
            addD(o, kDwm, QStringLiteral("DisableDeviceBitmaps"), 1);
            addD(o, kDwm, QStringLiteral("DebugFailFast"), 0);
            addD(o, kDwm, QStringLiteral("DisplayTestMode"), 0);
            addD(o, kDwm, QStringLiteral("UseHWDrawListEntriesOnWARP"), 1);
            addD(o, kDwm, QStringLiteral("ResampleModeOverride"), 1);
            addD(o, kDwm, QStringLiteral("RenderThreadWatchdogTimeoutMilliseconds"), 0);
            addD(o, kDwm, QStringLiteral("ParallelModePolicy"), 1);
            addD(o, kDwm, QStringLiteral("EnableResizeOptimization"), 1);
            addD(o, kDwm, QStringLiteral("EnableMegaRects"), 1);
            addD(o, kDwm, QStringLiteral("EnableFrontBufferRenderChecks"), 0);
            addD(o, kDwm, QStringLiteral("EnableEffectCaching"), 1);
            addD(o, kDwm, QStringLiteral("EnableDesktopOverlays"), 0);
            addD(o, kDwm, QStringLiteral("EnablePrimitiveReordering"), 0);
            addD(o, kDwm, QStringLiteral("MaxD3DFeatureLevel"), 0);
            addD(o, kDwm, QStringLiteral("OverlayQualifyCount"), 0);
            addD(o, kDwm, QStringLiteral("OverlayDisqualifyCount"), 0);
            addD(o, kDwm, QStringLiteral("ResizeTimeoutModern"), 0);
            addD(o, kDwm, QStringLiteral("ResizeTimeoutGdi"), 0);
            addD(o, kDwm, QStringLiteral("HighColor"), 0);
            addD(o, kDwm, QStringLiteral("AnimationsShiftKey"), 0);
            addD(o, kDwm, QStringLiteral("DisableDrawListCaching"), 1);
            addD(o, kDwm, QStringLiteral("AnimationAttributionEnabled"), 0);
            addD(o, kDwm, QStringLiteral("EnableCommonSuperSets"), 1);
            addD(o, kDwm, QStringLiteral("DisableAdvancedDirectFlip"), 1);
            t.registryOps = o;
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.direct3d-dxgi-legacy");
            t.category = QStringLiteral("gpu");
            t.name = QStringLiteral("Direct3D / DXGI legacy registry set");
            t.description = QStringLiteral(
                "Writes a broad set of legacy Direct3D, DirectDraw, DXGI and DXGKrnl registry "
                "flags covering deferred contexts, swap-chain behavior and driver debug modes.");
            t.iconName = QStringLiteral("monitor");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "placebo-leaning: many of these are advisory hints the driver may ignore, and "
                "some (FullDebug, LoadDebugRuntime, EnumReference/EnumRamp/EnumNullDevice) are "
                "debug-runtime switches with no gaming benefit. Harmless but largely inert on "
                "modern drivers.");
            QVector<RegistryOp> o;
            const QString kD3D  = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D");
            const QString kD3DD = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Direct3D\\Drivers");
            const QString kDD   = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectDraw");
            const QString kDxKrnl = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DXGKrnl");
            addD(o, kD3D, QStringLiteral("DisableVidMemVBs"), 0);
            addD(o, kD3D, QStringLiteral("UseNonLocalVidMem"), 1);
            addD(o, kD3D, QStringLiteral("FullDebug"), 0);
            addD(o, kD3D, QStringLiteral("EnableMultimonDebugging"), 0);
            addD(o, kD3D, QStringLiteral("LoadDebugRuntime"), 0);
            addD(o, kD3D, QStringLiteral("DisableDM"), 1);
            addD(o, kD3D, QStringLiteral("FewVertices"), 1);
            addD(o, kD3D, QStringLiteral("DisableMMX"), 0);
            addD(o, kD3D, QStringLiteral("UseMMXForRGB"), 1);
            addD(o, kD3DD, QStringLiteral("SoftwareOnly"), 0);
            addD(o, kD3DD, QStringLiteral("ForceRgbRasterizer"), 0);
            addD(o, kD3DD, QStringLiteral("EnumReference"), 1);
            addD(o, kD3DD, QStringLiteral("EnumSeparateMMX"), 1);
            addD(o, kD3DD, QStringLiteral("EnumRamp"), 1);
            addD(o, kD3DD, QStringLiteral("EnumNullDevice"), 1);
            addD(o, kD3DD, QStringLiteral("UseMMXForRGB"), 1);
            addD(o, kDD, QStringLiteral("EmulationOnly"), 0);
            addD(o, kDD, QStringLiteral("DisableAGPSupport"), 0);
            addD(o, kDD, QStringLiteral("UseNonLocalVidMem"), 1);
            addD(o, kDD, QStringLiteral("DisableDDSCAPSInDDSD"), 0);
            addD(o, kDD, QStringLiteral("EmulatePointSprites"), 0);
            addD(o, kDD, QStringLiteral("EmulateStateBlocks"), 0);
            addD(o, kDirectX, QStringLiteral("D3D11_DEFERRED_CONTEXTS"), 1);
            addD(o, kDirectX, QStringLiteral("DXGI_PREEMPTION_MODE"), 3);
            addD(o, kDirectX, QStringLiteral("DXGI_FRAME_LATENCY_WAITABLE_OBJECT"), 1);
            addD(o, kDirectX, QStringLiteral("DXGI_SWAP_CHAIN_WAITABLE_OBJECT"), 1);
            addD(o, kDirectX, QStringLiteral("DXGI_FORCE_FLIP_DISCARD"), 1);
            addD(o, kDirectX, QStringLiteral("DXGI_SWAP_CHAIN_SCALE"), 1);
            addD(o, kDirectX, QStringLiteral("DXGI_SWAP_CHAIN_ALLOW_MODE_SWITCH"), 1);
            addD(o, kDirectX, QStringLiteral("DXGI_SWAP_CHAIN_FULLSCREEN_FLIP_MODE"), 1);
            addD(o, kDirectX, QStringLiteral("DXGI_DISABLE_DWM_THROTTLING"), 1);
            addD(o, kDirectX, QStringLiteral("DXGI_FORCE_FLIP_SEQUENTIAL"), 1);
            addD(o, kDirectX, QStringLiteral("DXGI_FORCE_FULLSCREEN_FLIP_MODE"), 3);
            addD(o, kDirectX, QStringLiteral("DXGI_MAX_FRAME_LATENCY"), 2);
            addD(o, kDirectX, QStringLiteral("DXGI_USE_OPTIMIZED_SWAP_CHAIN"), 1);
            addD(o, kDxKrnl, QStringLiteral("CreateGdiPrimaryOnSlaveGPU"), 1);
            addD(o, kDxKrnl, QStringLiteral("DriverSupportsCddDwmInterop"), 1);
            addD(o, kDxKrnl, QStringLiteral("DxgkCddSyncDxAccess"), 1);
            addD(o, kDxKrnl, QStringLiteral("DxgkCddSyncGPUAccess"), 1);
            addD(o, kDxKrnl, QStringLiteral("DxgkCddWaitForVerticalBlankEvent"), 1);
            addD(o, kDxKrnl, QStringLiteral("DxgkCreateSwapChain"), 1);
            addD(o, kDxKrnl, QStringLiteral("DxgkFreeGpuVirtualAddress"), 1);
            addD(o, kDxKrnl, QStringLiteral("DxgkOpenSwapChain"), 1);
            addD(o, kDxKrnl, QStringLiteral("DxgkShareSwapChainObject"), 1);
            addD(o, kDxKrnl, QStringLiteral("DxgkWaitForVerticalBlankEvent"), 1);
            addD(o, kDxKrnl, QStringLiteral("DxgkWaitForVerticalBlankEvent2"), 1);
            addD(o, kDxKrnl, QStringLiteral("SwapChainBackBuffer"), 1);
            addD(o, kDxKrnl, QStringLiteral("TdrResetFromTimeoutAsync"), 1);
            t.registryOps = o;
            c.push_back(t);
        }
    }
}
