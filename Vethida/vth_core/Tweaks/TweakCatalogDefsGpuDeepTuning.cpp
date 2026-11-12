#include "TweakEngine.h"
#include "DeviceOps.h"

namespace vth::tweaks::detail {

    namespace dev = vth::tweaks::devices;

    namespace {

        struct RegTarget {
            QString path;
            QString name;
            quint32 value = 0;
        };

        QString snapKey(const QString& path, const QString& name) {
            return path + QLatin1Char('|') + name;
        }

        // Writes each target, snapshotting the prior value first so revert is exact.
        bool applyTargets(const QString& id, const QVector<RegTarget>& targets) {
            QMap<QString, QString> snap;
            bool ok = true;
            for (const RegTarget& t : targets) {
                QString prior = QStringLiteral("-");
                if (const auto v = dev::regReadDword(t.path, t.name))
                    prior = QString::number(*v);
                snap.insert(snapKey(t.path, t.name), prior);
                ok = dev::regWriteDword(t.path, t.name, t.value) && ok;
            }
            dev::saveSnapshot(id, snap);
            return ok;
        }

        // Restores each snapshotted prior value, or deletes values that did not exist.
        bool revertTargets(const QString& id) {
            const QMap<QString, QString> snap = dev::loadSnapshot(id);
            bool ok = true;
            for (auto it = snap.begin(); it != snap.end(); ++it) {
                const int bar = it.key().lastIndexOf(QLatin1Char('|'));
                if (bar < 0) continue;
                const QString path = it.key().left(bar);
                const QString name = it.key().mid(bar + 1);
                if (it.value() == QStringLiteral("-"))
                    ok = dev::regDeleteValue(path, name) && ok;
                else
                    ok = dev::regWriteDword(path, name, it.value().toUInt()) && ok;
            }
            dev::clearSnapshot(id);
            return ok;
        }

        DetectState detectTargets(const QVector<RegTarget>& targets) {
            if (targets.isEmpty()) return DetectState::Unknown;
            int on = 0, total = 0;
            for (const RegTarget& t : targets) {
                ++total;
                const auto v = dev::regReadDword(t.path, t.name);
                if (v && *v == t.value) ++on;
            }
            if (on == total) return DetectState::On;
            return DetectState::Off;
        }

        // Builds a DynamicOp from a target-list builder (re-enumerated each call so
        // hot-plugged adapters are picked up). The builder returns an empty list
        // when no matching adapter is present, making the op a no-op there.
        DynamicOp targetOp(const QString& id, std::function<QVector<RegTarget>()> build) {
            DynamicOp op;
            op.label  = id;
            op.detect = [build]() { return detectTargets(build()); };
            op.apply  = [id, build]() { return applyTargets(id, build()); };
            op.revert = [id]() { return revertTargets(id); };
            return op;
        }

        RegTarget dwordT(const QString& path, const QString& name, quint32 v) {
            return RegTarget{ path, name, v };
        }
    }

    // Per-adapter GPU driver tuning, one tweak per function so each can be applied
    // or reverted on its own. Every op enumerates the vendor's display-adapter
    // class keys at runtime and writes its own snapshot for an exact revert.
    void appendDefsGpuDeepTuning(QVector<TweakSpec>& c) {

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.amd.resizable_bar");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("Resizable BAR");
            t.name = QStringLiteral("Enable Resizable BAR control");
            t.description = QStringLiteral("Enables the KMD Resizable BAR control keys on AMD/ATI adapters.");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                const QStringList keys = dev::enumerateGpuClassKeys(QStringLiteral("VEN_1002"));
                for (const QString& k : keys) {
                    v << dwordT(k, QStringLiteral("KMD_EnableReBarForLegacyASIC"), 1);
                    v << dwordT(k, QStringLiteral("KMD_RebarControlMode"), 1);
                    v << dwordT(k, QStringLiteral("KMD_RebarControlSupport"), 1);
                }
                return v;
            }) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.nvidia.logging_telemetry_off");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("Telemetry");
            t.name = QStringLiteral("Disable driver logging and telemetry");
            t.description = QStringLiteral("Turns off NVIDIA driver event logging, telemetry, update-notification popups and the tray daemon autostart.");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                const QStringList keys = dev::enumerateGpuClassKeys(QStringLiteral("VEN_10DE"));
                if (!keys.isEmpty()) {
                    v << dwordT(QStringLiteral("HKCU\\\\Software\\\\NVIDIA Corporation\\\\NvTray"), QStringLiteral("StartOnLogin"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SOFTWARE\\\\NVIDIA Corporation\\\\Global\\\\NVTweak"), QStringLiteral("DisableLogging"), 1u);
                    v << dwordT(QStringLiteral("HKLM\\\\SOFTWARE\\\\NVIDIA Corporation\\\\NvContainer\\\\NvAppTimestamps"), QStringLiteral("NvDriverUpdateNotificationPopupShownTime"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SOFTWARE\\\\NVIDIA Corporation\\\\NvContainer\\\\NvAppTimestamps"), QStringLiteral("NvDriverUpdateNotificationTime"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm\\\\Global"), QStringLiteral("EnableLogging"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SOFTWARE\\\\NVIDIA Corporation\\\\Global\\\\FTS"), QStringLiteral("EnableRID44231"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SOFTWARE\\\\NVIDIA Corporation\\\\Global\\\\FTS"), QStringLiteral("EnableRID64640"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SOFTWARE\\\\NVIDIA Corporation\\\\Global\\\\FTS"), QStringLiteral("EnableRID66610"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SOFTWARE\\\\NVIDIA Corporation\\\\Global\\\\NVTweak"), QStringLiteral("NoDaemon"), 1u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm"), QStringLiteral("LogErrorEntries"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm"), QStringLiteral("LogEventEntries"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm"), QStringLiteral("LogPagingEntries"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm"), QStringLiteral("LogWarningEntries"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm"), QStringLiteral("EnableTelemetry"), 0u);
                }
                return v;
            }) };
            c.push_back(t);
        }


        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.amd.power_gating_off");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("AMD");
            t.name = QStringLiteral("Disable media engine power gating");
            t.description = QStringLiteral("Disables SAMU/UVD/VCE media-engine power gating on AMD adapters.");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::High;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Raises idle power draw and temperatures"),
            };
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                const QStringList keys = dev::enumerateGpuClassKeys(QStringLiteral("VEN_1002"));
                for (const QString& k : keys) {
                    v << dwordT(k, QStringLiteral("DisableSAMUPowerGating"), 1);
                    v << dwordT(k, QStringLiteral("DisableUVDPowerGatingDynamic"), 1);
                    v << dwordT(k, QStringLiteral("DisableVCEPowerGating"), 1);
                }
                return v;
            }) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.amd.aspm_off");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("AMD");
            t.name = QStringLiteral("Disable PCIe ASPM");
            t.description = QStringLiteral("Disables PCIe Active State Power Management (L0s/L1) on AMD adapters.");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::High;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Raises idle power draw"),
            };
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                const QStringList keys = dev::enumerateGpuClassKeys(QStringLiteral("VEN_1002"));
                for (const QString& k : keys) {
                    v << dwordT(k, QStringLiteral("EnableAspmL0s"), 0);
                    v << dwordT(k, QStringLiteral("EnableAspmL1"), 0);
                }
                return v;
            }) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.amd.powerplay_max");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("AMD");
            t.name = QStringLiteral("Force maximum PowerPlay performance");
            t.description = QStringLiteral("Forces the AMD PowerPlay performance level to maximum and disables dynamic P-state switching.");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::High;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Raises idle power draw and temperatures"),
            };
            t.concern = QStringLiteral("undocumented PP_* keys; effect varies by driver version");
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                const QStringList keys = dev::enumerateGpuClassKeys(QStringLiteral("VEN_1002"));
                for (const QString& k : keys) {
                    v << dwordT(k, QStringLiteral("PP_ThermalLimit"), 1);
                    v << dwordT(k, QStringLiteral("PP_EnableVoltageControl"), 1);
                    v << dwordT(k, QStringLiteral("PP_PerformanceLevel"), 7);
                    v << dwordT(k, QStringLiteral("PP_MemoryTimingLevel"), 1);
                    v << dwordT(k, QStringLiteral("DisableDynamicPstate"), 1);
                }
                return v;
            }) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.amd.radeon_features_off");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("AMD");
            t.name = QStringLiteral("Disable Radeon boost / Chill / Anti-Lag");
            t.description = QStringLiteral("Disables Radeon Boost, Chill, Anti-Lag, frame-rate-target and compute preemption on AMD adapters.");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Anti-Lag and Chill stop working on adapters that expose them"),
            };
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                const QStringList keys = dev::enumerateGpuClassKeys(QStringLiteral("VEN_1002"));
                for (const QString& k : keys) {
                    v << dwordT(k, QStringLiteral("KMD_USUEnable"), 0);
                    v << dwordT(k, QStringLiteral("KMD_RadeonBoostEnabled"), 0);
                    v << dwordT(k, QStringLiteral("KMD_ChillEnabled"), 0);
                    v << dwordT(k, QStringLiteral("KMD_DeLagEnabled"), 0);
                    v << dwordT(k, QStringLiteral("KMD_FRTEnabled"), 0);
                    v << dwordT(k, QStringLiteral("KMD_EnableComputePreemption"), 0);
                }
                return v;
            }) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.amd.stutter_flip_tuning");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("AMD");
            t.name = QStringLiteral("Stutter and flip-queue behavior");
            t.description = QStringLiteral("Disables AMD stutter mode and ULPS and sets the UMD flip-queue size.");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Disabling ULPS raises multi-GPU idle power"),
            };
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                const QStringList keys = dev::enumerateGpuClassKeys(QStringLiteral("VEN_1002"));
                for (const QString& k : keys) {
                    const QString umd = k + QStringLiteral("\\UMD");
                    v << dwordT(k, QStringLiteral("StutterMode"), 0);
                    v << dwordT(k, QStringLiteral("Ulps"), 0);
                    v << dwordT(k, QStringLiteral("DisableDMACopy"), 1);
                    v << dwordT(k, QStringLiteral("DisableBlockWrite"), 0);
                    v << dwordT(umd, QStringLiteral("StutterMode"), 0);
                    v << dwordT(umd, QStringLiteral("FlipQueueSize"), 1);
                }
                return v;
            }) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.nvidia.preemption_off");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("NVIDIA");
            t.name = QStringLiteral("Disable GPU preemption");
            t.description = QStringLiteral("Disables every RM/KMD graphics, compute, mid-buffer and CE preemption path on NVIDIA adapters and their nvlddmkm service keys.");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::High;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Disabling preemption can briefly freeze the desktop under very heavy GPU load"),
            };
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                const QStringList keys = dev::enumerateGpuClassKeys(QStringLiteral("VEN_10DE"));
                for (const QString& k : keys) {
                    v << dwordT(k, QStringLiteral("ComputePreemption"), 0);
                    v << dwordT(k, QStringLiteral("DisableCudaContextPreemption"), 1);
                    v << dwordT(k, QStringLiteral("DisablePreemptionOnS3S4"), 1);
                    v << dwordT(k, QStringLiteral("EnableAsyncMidBufferPreemption"), 0);
                    v << dwordT(k, QStringLiteral("EnableCEPreemption"), 0);
                    v << dwordT(k, QStringLiteral("EnableMidBufferPreemption"), 0);
                    v << dwordT(k, QStringLiteral("EnableMidBufferPreemptionForHighTdrTimeout"), 0);
                    v << dwordT(k, QStringLiteral("EnableMidGfxPreemption"), 0);
                    v << dwordT(k, QStringLiteral("EnableMidGfxPreemptionVGPU"), 0);
                    v << dwordT(k, QStringLiteral("EnablePreemption"), 0);
                    v << dwordT(k, QStringLiteral("EnableSCGMidBufferPreemption"), 0);
                    v << dwordT(k, QStringLiteral("GPUPreemptionLevel"), 0);
                    v << dwordT(k, QStringLiteral("PerfAnalyzeMidBufferPreemption"), 0);
                }
                if (!keys.isEmpty()) {
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm"), QStringLiteral("ComputePreemption"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm"), QStringLiteral("EnableAsyncMidBufferPreemption"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm"), QStringLiteral("EnableCEPreemption"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm"), QStringLiteral("EnableMidBufferPreemption"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm"), QStringLiteral("EnableMidBufferPreemptionForHighTdrTimeout"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm"), QStringLiteral("EnableMidGfxPreemption"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm"), QStringLiteral("EnableMidGfxPreemptionVGPU"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm"), QStringLiteral("PerfAnalyzeMidBufferPreemption"), 1u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm"), QStringLiteral("EnablePreemption"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm"), QStringLiteral("EnableSCGMidBufferPreemption"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm"), QStringLiteral("GPUPreemptionLevel"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm\\\\FTS"), QStringLiteral("ComputePreemption"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm\\\\FTS"), QStringLiteral("DisableCudaContextPreemption"), 1u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm\\\\FTS"), QStringLiteral("DisablePreemption"), 1u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm\\\\FTS"), QStringLiteral("DisablePreemptionOnS3S4"), 1u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm\\\\FTS"), QStringLiteral("EnableAsyncMidBufferPreemption"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm\\\\FTS"), QStringLiteral("EnableCEPreemption"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm\\\\FTS"), QStringLiteral("EnableComputePreemption"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm\\\\FTS"), QStringLiteral("EnableGfxPreemption"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm\\\\FTS"), QStringLiteral("EnableMidBufferPreemption"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm\\\\FTS"), QStringLiteral("EnableMidBufferPreemptionForHighTdrTimeout"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm\\\\FTS"), QStringLiteral("EnableMidGfxPreemption"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm\\\\FTS"), QStringLiteral("EnableMidGfxPreemptionVGPU"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm\\\\FTS"), QStringLiteral("EnablePreemption"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm\\\\FTS"), QStringLiteral("EnableSCGMidBufferPreemption"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm\\\\FTS"), QStringLiteral("GPUPreemptionLevel"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm\\\\FTS"), QStringLiteral("PerfAnalyzeMidBufferPreemption"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm\\\\Parameters"), QStringLiteral("DisableCudaContextPreemption"), 1u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm\\\\Parameters"), QStringLiteral("DisablePreemption"), 1u);
                }
                return v;
            }) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.nvidia.power_clock_gating_off");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("NVIDIA");
            t.name = QStringLiteral("Disable power and clock gating");
            t.description = QStringLiteral("Disables NVIDIA clock/power gating, ULPS and driver power-saving so the GPU holds clocks instead of down-gating at idle.");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::High;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Raises idle power draw and temperatures"),
            };
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                const QStringList keys = dev::enumerateGpuClassKeys(QStringLiteral("VEN_10DE"));
                for (const QString& k : keys) {
                    v << dwordT(k, QStringLiteral("DisableClockGating"), 1);
                    v << dwordT(k, QStringLiteral("DisableDynamicClockGating"), 1);
                    v << dwordT(k, QStringLiteral("DisablePowerLimit"), 1);
                    v << dwordT(k, QStringLiteral("DisablePowerManagement"), 1);
                    v << dwordT(k, QStringLiteral("DisableULPS"), 1);
                    v << dwordT(k, QStringLiteral("EnablePowerManagement"), 0);
                    v << dwordT(k, QStringLiteral("PowerLimit"), 0);
                }
                if (!keys.isEmpty()) {
                    v << dwordT(QStringLiteral("HKCU\\\\Software\\\\NVIDIA Corporation\\\\Global\\\\NvControlPanel"), QStringLiteral("AdaptivePower"), 0u);
                    v << dwordT(QStringLiteral("HKCU\\\\Software\\\\NVIDIA Corporation\\\\Global\\\\NvControlPanel"), QStringLiteral("MultiGPUPowerSaving"), 0u);
                    v << dwordT(QStringLiteral("HKCU\\\\Software\\\\NVIDIA Corporation\\\\Global\\\\NvControlPanel"), QStringLiteral("PowerSaving"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm\\\\Global\\\\NVTweak"), QStringLiteral("DisplayPowerSaving"), 0u);
                }
                return v;
            }) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.nvidia.aspm_off");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("NVIDIA");
            t.name = QStringLiteral("Disable PCIe ASPM / link power saving");
            t.description = QStringLiteral("Disables PCIe Active State Power Management (L0s/L1) and the RM link-power-saving flags on NVIDIA adapters.");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::High;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Raises idle power draw; only intended for desktops on wall power"),
            };
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                const QStringList keys = dev::enumerateGpuClassKeys(QStringLiteral("VEN_10DE"));
                for (const QString& k : keys) {
                    v << dwordT(k, QStringLiteral("EnableALPM"), 0);
                    v << dwordT(k, QStringLiteral("EnableAspm"), 0);
                    v << dwordT(k, QStringLiteral("EnableL1LowPower"), 0);
                    v << dwordT(k, QStringLiteral("L1LowPower"), 0);
                    v << dwordT(k, QStringLiteral("RMDeepL1EntryLatencyUsec"), 1);
                    v << dwordT(k, QStringLiteral("RMDisableGpuASPMFlags"), 3);
                    v << dwordT(k, QStringLiteral("RMDisableOptimalPowerForPadlinkPll"), 1);
                    v << dwordT(k, QStringLiteral("RMPcieLtrOverride"), 2);
                    v << dwordT(k, QStringLiteral("RmUnusedPcieL1"), 0);
                    v << dwordT(k, QStringLiteral("RpL1_32Support"), 0);
                }
                return v;
            }) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.nvidia.pcie_link_max");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("NVIDIA");
            t.name = QStringLiteral("Force maximum PCIe link Width/Speed");
            t.description = QStringLiteral("Pins the NVIDIA adapter to its maximum PCIe link width (x16) and speed and lengthens the PCI latency timer.");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                const QStringList keys = dev::enumerateGpuClassKeys(QStringLiteral("VEN_10DE"));
                for (const QString& k : keys) {
                    v << dwordT(k, QStringLiteral("LinkSpeed"), 4);
                    v << dwordT(k, QStringLiteral("LinkWidth"), 16);
                    v << dwordT(k, QStringLiteral("PciLatencyTimerControl"), 32);
                    v << dwordT(k, QStringLiteral("PeerMappingOverride"), 1);
                }
                if (!keys.isEmpty()) {
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm"), QStringLiteral("RMPcieLinkSpeed"), 4u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm"), QStringLiteral("RMPcieLinkWidth"), 16u);
                }
                return v;
            }) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.nvidia.pstate_perf_max");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("NVIDIA");
            t.name = QStringLiteral("Force maximum performance state");
            t.description = QStringLiteral("Forces the NVIDIA adapter into its top performance P-state and PowerMizer level instead of scaling clocks dynamically.");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::High;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Raises idle power draw and temperatures"),
            };
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                const QStringList keys = dev::enumerateGpuClassKeys(QStringLiteral("VEN_10DE"));
                for (const QString& k : keys) {
                    v << dwordT(k, QStringLiteral("MemoryPerformanceLevel"), 3);
                    v << dwordT(k, QStringLiteral("MemoryTimingLevel"), 4);
                    v << dwordT(k, QStringLiteral("PerfForced"), 1);
                    v << dwordT(k, QStringLiteral("PerformanceMemoryTiming"), 1);
                    v << dwordT(k, QStringLiteral("PerformanceTableLevel"), 3);
                    v << dwordT(k, QStringLiteral("PreferredPerformanceMode"), 1);
                    v << dwordT(k, QStringLiteral("RTXPerformanceMode"), 1);
                }
                if (!keys.isEmpty()) {
                    v << dwordT(QStringLiteral("HKCU\\\\Software\\\\NVIDIA Corporation\\\\Global\\\\NvControlPanel"), QStringLiteral("PowerMizerDefault"), 1u);
                    v << dwordT(QStringLiteral("HKCU\\\\Software\\\\NVIDIA Corporation\\\\Global\\\\NvControlPanel"), QStringLiteral("PowerMizerLevel"), 1u);
                    v << dwordT(QStringLiteral("HKLM\\\\SOFTWARE\\\\NVIDIA Corporation\\\\Global\\\\NVTweak"), QStringLiteral("DisableDynamicPstate"), 1u);
                    v << dwordT(QStringLiteral("HKLM\\\\SOFTWARE\\\\NVIDIA Corporation\\\\Global\\\\System"), QStringLiteral("ForcePState"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm\\\\Global\\\\NVTweak"), QStringLiteral("DisableDynamicPstate"), 1u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm\\\\Global\\\\NVTweak"), QStringLiteral("PerfLevelSrc"), 13107u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm\\\\Parameters"), QStringLiteral("GPUPerformanceScale"), 1u);
                }
                return v;
            }) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.nvidia.idle_power_off");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("NVIDIA");
            t.name = QStringLiteral("Disable idle / GC6 power saving");
            t.description = QStringLiteral("Disables the NVIDIA GC6 sleep, engine idle thresholds and DRAM PLL power-off so the GPU does not enter its deep idle states.");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::High;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Raises idle power draw and temperatures"),
            };
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                const QStringList keys = dev::enumerateGpuClassKeys(QStringLiteral("VEN_10DE"));
                for (const QString& k : keys) {
                    v << dwordT(k, QStringLiteral("RMElpgStateOnInit"), 3);
                    v << dwordT(k, QStringLiteral("RMFspg"), 15);
                    v << dwordT(k, QStringLiteral("RMGC6Feature"), 11185050);
                    v << dwordT(k, QStringLiteral("RMGC6Parameters"), 85);
                    v << dwordT(k, QStringLiteral("RMLpwrEiIdleThresholdUs"), 1);
                    v << dwordT(k, QStringLiteral("RMLpwrGrIdleThresholdUs"), 1);
                    v << dwordT(k, QStringLiteral("RMLpwrGrRgIdleThresholdUs"), 1);
                    v << dwordT(k, QStringLiteral("RMLpwrMsIdleThresholdUs"), 1);
                    v << dwordT(k, QStringLiteral("RmClkPowerOffDramPllWhenUnused"), 0);
                    v << dwordT(k, QStringLiteral("RmGspcMaxFtuS"), 1);
                    v << dwordT(k, QStringLiteral("RmGspcMinFtuS"), 1);
                    v << dwordT(k, QStringLiteral("RmGspcPerioduS"), 1);
                    v << dwordT(k, QStringLiteral("RmMIONoPowerOff"), 1);
                    v << dwordT(k, QStringLiteral("ThermalPolicy"), 0);
                }
                return v;
            }) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.nvidia.compression_off");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("NVIDIA");
            t.name = QStringLiteral("Disable GPU memory and color compression");
            t.description = QStringLiteral("Disables NVIDIA framebuffer, L2, delta-color and Z-buffer compression.");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Trades VRAM bandwidth efficiency for lower compression latency; can reduce effective memory bandwidth"),
            };
            t.concern = QStringLiteral("undocumented RM keys; effect varies by GPU generation");
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                const QStringList keys = dev::enumerateGpuClassKeys(QStringLiteral("VEN_10DE"));
                for (const QString& k : keys) {
                    v << dwordT(k, QStringLiteral("EnableMemoryCompression"), 0);
                    v << dwordT(k, QStringLiteral("MemoryCompression"), 0);
                    v << dwordT(k, QStringLiteral("RMDisablePostL2Compression"), 1);
                    v << dwordT(k, QStringLiteral("ZBufferCompression"), 0);
                }
                if (!keys.isEmpty()) {
                    v << dwordT(QStringLiteral("HKCU\\\\Software\\\\NVIDIA Corporation\\\\Global\\\\NvControlPanel"), QStringLiteral("MemoryCompression"), 0u);
                    v << dwordT(QStringLiteral("HKCU\\\\Software\\\\NVIDIA Corporation\\\\Global\\\\NvControlPanel"), QStringLiteral("ZBufferCompression"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm\\\\Global\\\\NVTweak"), QStringLiteral("DisableDeltaColorCompression"), 1u);
                }
                return v;
            }) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.nvidia.shader_texture_cache");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("NVIDIA");
            t.name = QStringLiteral("Shader and texture cache");
            t.description = QStringLiteral("Enables the shader/texture caches, precaching and an unbounded shader disk cache on NVIDIA adapters.");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                const QStringList keys = dev::enumerateGpuClassKeys(QStringLiteral("VEN_10DE"));
                for (const QString& k : keys) {
                    v << dwordT(k, QStringLiteral("ShaderCache"), 1);
                    v << dwordT(k, QStringLiteral("ShaderCacheSize"), 0);
                    v << dwordT(k, QStringLiteral("TextureCacheSize"), 1024);
                    v << dwordT(k, QStringLiteral("TexturePrecache"), 1);
                }
                if (!keys.isEmpty()) {
                    v << dwordT(QStringLiteral("HKCU\\\\Software\\\\NVIDIA Corporation\\\\Global\\\\NvControlPanel"), QStringLiteral("DisableShaderDiskCache"), 1u);
                    v << dwordT(QStringLiteral("HKCU\\\\Software\\\\NVIDIA Corporation\\\\Global\\\\NvControlPanel"), QStringLiteral("ShaderCacheSize"), 4294967295u);
                    v << dwordT(QStringLiteral("HKCU\\\\Software\\\\NVIDIA Corporation\\\\Global\\\\NvControlPanel"), QStringLiteral("ShaderDiskCache"), 1u);
                    v << dwordT(QStringLiteral("HKLM\\\\SOFTWARE\\\\NVIDIA Corporation\\\\Global\\\\System"), QStringLiteral("TexturePrecache"), 1u);
                }
                return v;
            }) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.nvidia.interrupt_dpc");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("NVIDIA");
            t.name = QStringLiteral("GPU interrupt and DPC handling");
            t.description = QStringLiteral("Enables the NVIDIA RM message-signaled-interrupt flags and per-CPU-core DPC distribution.");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Rare hardware misbehaves on message-signaled interrupts; if the display driver fails after reboot, revert and reboot again"),
            };
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                const QStringList keys = dev::enumerateGpuClassKeys(QStringLiteral("VEN_10DE"));
                for (const QString& k : keys) {
                    v << dwordT(k, QStringLiteral("MSISupported"), 1);
                    v << dwordT(k, QStringLiteral("MessageSignaledInterrupts"), 1);
                    v << dwordT(k, QStringLiteral("RMDisablePerIntrDPCQueueing"), 1);
                }
                if (!keys.isEmpty()) {
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm\\\\Global\\\\NVTweak"), QStringLiteral("MessageInterruptCheck"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm\\\\Global\\\\NVTweak"), QStringLiteral("RmGpsPsEnablePerCpuCoreDpc"), 1u);
                }
                return v;
            }) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.nvidia.texture_filtering");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("NVIDIA");
            t.name = QStringLiteral("Texture filtering quality (high performance)");
            t.description = QStringLiteral("Sets NVIDIA texture filtering to the high-performance profile (anisotropic level, LOD bias, trilinear optimizations).");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Lowers texture-filtering image quality slightly in exchange for throughput"),
            };
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                const QStringList keys = dev::enumerateGpuClassKeys(QStringLiteral("VEN_10DE"));
                for (const QString& k : keys) {
                    v << dwordT(k, QStringLiteral("AnisotropicLevel"), 16);
                    v << dwordT(k, QStringLiteral("TextureFiltering"), 16);
                }
                if (!keys.isEmpty()) {
                    v << dwordT(QStringLiteral("HKCU\\\\Software\\\\NVIDIA Corporation\\\\Global\\\\NvControlPanel"), QStringLiteral("AnisotropicFiltering"), 16u);
                    v << dwordT(QStringLiteral("HKCU\\\\Software\\\\NVIDIA Corporation\\\\Global\\\\NvControlPanel"), QStringLiteral("AnisotropicFilteringOpt"), 0u);
                    v << dwordT(QStringLiteral("HKCU\\\\Software\\\\NVIDIA Corporation\\\\Global\\\\NvControlPanel"), QStringLiteral("NegativeLODBias"), 1u);
                    v << dwordT(QStringLiteral("HKCU\\\\Software\\\\NVIDIA Corporation\\\\Global\\\\NvControlPanel"), QStringLiteral("LODBias"), 0u);
                    v << dwordT(QStringLiteral("HKCU\\\\Software\\\\NVIDIA Corporation\\\\Global\\\\NvControlPanel"), QStringLiteral("TextureFilteringQuality"), 0u);
                    v << dwordT(QStringLiteral("HKCU\\\\Software\\\\NVIDIA Corporation\\\\Global\\\\NvControlPanel"), QStringLiteral("TextureFilteringTrilinear"), 0u);
                }
                return v;
            }) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.nvidia.low_latency");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("NVIDIA");
            t.name = QStringLiteral("Low-latency flags");
            t.description = QStringLiteral("Enables the NVIDIA low-latency flags and zeroes the monitor latency-tolerance thresholds.");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                const QStringList keys = dev::enumerateGpuClassKeys(QStringLiteral("VEN_10DE"));
                for (const QString& k : keys) {
                    v << dwordT(k, QStringLiteral("D3PCLatency"), 1);
                    v << dwordT(k, QStringLiteral("F1TransitionLatency"), 1);
                    v << dwordT(k, QStringLiteral("LOWLATENCY"), 1);
                    v << dwordT(k, QStringLiteral("MonitorLatencyTolerance"), 0);
                    v << dwordT(k, QStringLiteral("MonitorRefreshLatencyTolerance"), 0);
                    v << dwordT(k, QStringLiteral("Node3DLowLatency"), 1);
                }
                if (!keys.isEmpty()) {
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm\\\\Parameters"), QStringLiteral("VideoEvent_EnableLatencyCheck"), 0u);
                }
                return v;
            }) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.nvidia.tdr_recovery_off");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("NVIDIA");
            t.name = QStringLiteral("Disable GPU hang recovery (TDR)");
            t.description = QStringLiteral("Disables the Timeout Detection & Recovery watchdog (TdrLevel=0, TdrDelay=0) on the NVIDIA driver.");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::High;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("A genuinely hung GPU will no longer be automatically recovered and can require a hard reboot - only for adapters already stable under custom clocks"),
            };
            t.concern = QStringLiteral("high-risk; removes the driver-hang safety net");
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                const QStringList keys = dev::enumerateGpuClassKeys(QStringLiteral("VEN_10DE"));
                if (!keys.isEmpty()) {
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm"), QStringLiteral("TdrDelay"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm"), QStringLiteral("TdrLevel"), 0u);
                }
                return v;
            }) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.nvidia.safety_checks_off");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("NVIDIA");
            t.name = QStringLiteral("Disable driver safety and health checks");
            t.description = QStringLiteral("Disables NVIDIA RM safety checks, GPU health checks, WDDM2 checks and blit sub-rect validation.");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::High;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Removes driver-level safety checks; only intended for adapters already running stable custom clocks/voltage"),
            };
            t.concern = QStringLiteral("high-risk; removes RM safety validation");
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                const QStringList keys = dev::enumerateGpuClassKeys(QStringLiteral("VEN_10DE"));
                for (const QString& k : keys) {
                    v << dwordT(k, QStringLiteral("TrackResetEngine"), 0);
                    v << dwordT(k, QStringLiteral("ValidateBlitSubRects"), 0);
                }
                if (!keys.isEmpty()) {
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm"), QStringLiteral("DisableGpuSafetyChecks"), 1u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm"), QStringLiteral("EnableBugcheckDisplay"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm\\\\FTS"), QStringLiteral("EnableGR535"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm\\\\Global\\\\NVTweak"), QStringLiteral("DisableWddm2Checks"), 1u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm\\\\Global\\\\NVTweak"), QStringLiteral("EnableGR535"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm\\\\Global\\\\NVTweak"), QStringLiteral("EnableGpuHealthCheck"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm\\\\Global\\\\NVTweak"), QStringLiteral("NoInterference"), 1u);
                }
                return v;
            }) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.nvidia.vsync_frame_pacing");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("NVIDIA");
            t.name = QStringLiteral("VSync and frame pacing");
            t.description = QStringLiteral("Disables NVIDIA VSync/VRR and frame-rate limiting and zeroes the pre-rendered/render-ahead frame queues.");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Zero pre-rendered frames can reduce smoothness on slower CPUs; disabling VSync can cause tearing"),
            };
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                const QStringList keys = dev::enumerateGpuClassKeys(QStringLiteral("VEN_10DE"));
                if (!keys.isEmpty()) {
                    v << dwordT(QStringLiteral("HKCU\\\\Software\\\\NVIDIA Corporation\\\\Global\\\\NvControlPanel"), QStringLiteral("FrameRateLimiter"), 0u);
                    v << dwordT(QStringLiteral("HKCU\\\\Software\\\\NVIDIA Corporation\\\\Global\\\\NvControlPanel"), QStringLiteral("FrameRateLimiter2"), 0u);
                    v << dwordT(QStringLiteral("HKCU\\\\Software\\\\NVIDIA Corporation\\\\Global\\\\NvControlPanel"), QStringLiteral("MaxPrerenderedFrames"), 0u);
                    v << dwordT(QStringLiteral("HKCU\\\\Software\\\\NVIDIA Corporation\\\\Global\\\\NvControlPanel"), QStringLiteral("MaxRenderAhead"), 0u);
                    v << dwordT(QStringLiteral("HKCU\\\\Software\\\\NVIDIA Corporation\\\\Global\\\\NvControlPanel"), QStringLiteral("RenderAheadLimit"), 0u);
                    v << dwordT(QStringLiteral("HKCU\\\\Software\\\\NVIDIA Corporation\\\\Global\\\\NvControlPanel"), QStringLiteral("SyncToVblank"), 0u);
                    v << dwordT(QStringLiteral("HKCU\\\\Software\\\\NVIDIA Corporation\\\\Global\\\\NvControlPanel"), QStringLiteral("TearingControl"), 0u);
                    v << dwordT(QStringLiteral("HKCU\\\\Software\\\\NVIDIA Corporation\\\\Global\\\\NvControlPanel"), QStringLiteral("VirtualPreRenderedFrames"), 0u);
                    v << dwordT(QStringLiteral("HKCU\\\\Software\\\\NVIDIA Corporation\\\\Global\\\\NvControlPanel"), QStringLiteral("VsyncBehavior"), 0u);
                    v << dwordT(QStringLiteral("HKCU\\\\Software\\\\NVIDIA Corporation\\\\Global\\\\NvControlPanel"), QStringLiteral("VsyncVRREnable"), 0u);
                }
                return v;
            }) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.nvidia.resizable_bar");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("NVIDIA");
            t.name = QStringLiteral("Enable Resizable BAR control");
            t.description = QStringLiteral("Enables the KMD Resizable BAR control keys on the NVIDIA adapter.");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                const QStringList keys = dev::enumerateGpuClassKeys(QStringLiteral("VEN_10DE"));
                for (const QString& k : keys) {
                    v << dwordT(k, QStringLiteral("KMD_EnableReBarForLegacyASIC"), 1);
                    v << dwordT(k, QStringLiteral("KMD_RebarControlMode"), 1);
                    v << dwordT(k, QStringLiteral("KMD_RebarControlSupport"), 1);
                }
                return v;
            }) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.nvidia.opengl_legacy_flags");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("NVIDIA");
            t.name = QStringLiteral("OpenGL and legacy bus flags");
            t.description = QStringLiteral("Sets the NVIDIA OpenGL threaded-control, fast-copy, shadow-FB and legacy AGP/SBA bus flags.");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral("legacy AGP/SBA flags; no effect on modern PCIe-only systems");
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                const QStringList keys = dev::enumerateGpuClassKeys(QStringLiteral("VEN_10DE"));
                for (const QString& k : keys) {
                    v << dwordT(k, QStringLiteral("EnableFastCopyPixels"), 1);
                    v << dwordT(k, QStringLiteral("ShadowFB"), 1);
                }
                if (!keys.isEmpty()) {
                    v << dwordT(QStringLiteral("HKLM\\\\SOFTWARE\\\\NVIDIA Corporation\\\\Global\\\\System"), QStringLiteral("EnableAGPFW"), 1u);
                    v << dwordT(QStringLiteral("HKLM\\\\SOFTWARE\\\\NVIDIA Corporation\\\\Global\\\\System"), QStringLiteral("EnableAGPSBA"), 1u);
                    v << dwordT(QStringLiteral("HKLM\\\\SOFTWARE\\\\NVIDIA Corporation\\\\Global\\\\System"), QStringLiteral("EnableFastCopyPixels"), 1u);
                    v << dwordT(QStringLiteral("HKLM\\\\SOFTWARE\\\\NVIDIA Corporation\\\\Global\\\\System"), QStringLiteral("EnableIrongateSBA"), 1u);
                    v << dwordT(QStringLiteral("HKLM\\\\SOFTWARE\\\\NVIDIA Corporation\\\\Global\\\\System"), QStringLiteral("EnableVIASBA"), 1u);
                    v << dwordT(QStringLiteral("HKLM\\\\SOFTWARE\\\\NVIDIA Corporation\\\\Global\\\\System"), QStringLiteral("FastVram"), 1u);
                    v << dwordT(QStringLiteral("HKLM\\\\SOFTWARE\\\\NVIDIA Corporation\\\\Global\\\\System"), QStringLiteral("OGL_Force16BitZWin"), 0u);
                    v << dwordT(QStringLiteral("HKLM\\\\SOFTWARE\\\\NVIDIA Corporation\\\\Global\\\\System"), QStringLiteral("OGL_ThreadControl"), 2u);
                    v << dwordT(QStringLiteral("HKLM\\\\SOFTWARE\\\\NVIDIA Corporation\\\\Global\\\\System"), QStringLiteral("ShadowFB"), 1u);
                    v << dwordT(QStringLiteral("HKLM\\\\SOFTWARE\\\\NVIDIA Corporation\\\\Global\\\\System"), QStringLiteral("TurboQueue"), 1u);
                }
                return v;
            }) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.nvidia.cuda_wddm");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("NVIDIA");
            t.name = QStringLiteral("CUDA / WDDM mode flags");
            t.description = QStringLiteral("Forces WDDM mode for CUDA and disables the MSHybrid NVSR switch and VTC.");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                const QStringList keys = dev::enumerateGpuClassKeys(QStringLiteral("VEN_10DE"));
                if (!keys.isEmpty()) {
                    v << dwordT(QStringLiteral("HKCU\\\\Software\\\\NVIDIA Corporation\\\\Global\\\\NvControlPanel"), QStringLiteral("CUDAForceWDDM"), 1u);
                    v << dwordT(QStringLiteral("HKCU\\\\Software\\\\NVIDIA Corporation\\\\Global\\\\NvControlPanel"), QStringLiteral("CUDAPreferWDDM"), 1u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm\\\\Global\\\\NVTweak"), QStringLiteral("DisableVTC"), 1u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm"), QStringLiteral("DisableMshybridNvsrSwitch"), 1u);
                }
                return v;
            }) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.nvidia.threading");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("NVIDIA");
            t.name = QStringLiteral("Driver threading and memory policy");
            t.description = QStringLiteral("Raises the NVIDIA driver worker-thread count and priority and sets the memory-allocation policy.");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                const QStringList keys = dev::enumerateGpuClassKeys(QStringLiteral("VEN_10DE"));
                if (!keys.isEmpty()) {
                    v << dwordT(QStringLiteral("HKCU\\\\Software\\\\NVIDIA Corporation\\\\Global\\\\NvControlPanel"), QStringLiteral("MaxThreads"), 16u);
                    v << dwordT(QStringLiteral("HKCU\\\\Software\\\\NVIDIA Corporation\\\\Global\\\\NvControlPanel"), QStringLiteral("MemoryAllocationPolicy"), 2u);
                    v << dwordT(QStringLiteral("HKCU\\\\Software\\\\NVIDIA Corporation\\\\Global\\\\NvControlPanel"), QStringLiteral("SLIEnabled"), 1u);
                    v << dwordT(QStringLiteral("HKCU\\\\Software\\\\NVIDIA Corporation\\\\Global\\\\NvControlPanel"), QStringLiteral("ThreadedOptimization"), 1u);
                    v << dwordT(QStringLiteral("HKCU\\\\Software\\\\NVIDIA Corporation\\\\Global\\\\NvControlPanel"), QStringLiteral("UMAFrameBufferSize"), 4294967295u);
                    v << dwordT(QStringLiteral("HKLM\\\\SYSTEM\\\\CurrentControlSet\\\\Services\\\\nvlddmkm\\\\Parameters"), QStringLiteral("ThreadPriority"), 31u);
                }
                return v;
            }) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.nvidia.rm_internal_flags");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("NVIDIA");
            t.name = QStringLiteral("RM internal flags");
            t.description = QStringLiteral("Sets assorted NVIDIA Resource Manager internal flags (cache location, registry caching, FBSR DMA, GPU operation mode).");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral("undocumented RM internals; effect not independently verified");
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                const QStringList keys = dev::enumerateGpuClassKeys(QStringLiteral("VEN_10DE"));
                for (const QString& k : keys) {
                    v << dwordT(k, QStringLiteral("RMGpuId"), 256);
                    v << dwordT(k, QStringLiteral("RmCacheLoc"), 0);
                    v << dwordT(k, QStringLiteral("RmDisableInforomNvlink"), 1);
                    v << dwordT(k, QStringLiteral("RmDisableInst2Sys"), 0);
                    v << dwordT(k, QStringLiteral("RmDisableRegistryCaching"), 1);
                    v << dwordT(k, QStringLiteral("RmFbsrPagedDMA"), 1);
                    v << dwordT(k, QStringLiteral("RmGpuOperationMode"), 0);
                    v << dwordT(k, QStringLiteral("RmProfilingAdminOnly"), 0);
                }
                return v;
            }) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.nvidia.feature_flags");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("NVIDIA");
            t.name = QStringLiteral("Driver feature flags");
            t.description = QStringLiteral("Sets assorted NVIDIA feature flags (DSC, write-combining, RTX20 optimizations, feature control, resolution handling).");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral("undocumented driver flags; effect not independently verified");
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                const QStringList keys = dev::enumerateGpuClassKeys(QStringLiteral("VEN_10DE"));
                for (const QString& k : keys) {
                    v << dwordT(k, QStringLiteral("Acceleration.Level"), 0);
                    v << dwordT(k, QStringLiteral("DLSSQuality"), 0);
                    v << dwordT(k, QStringLiteral("DesktopStereoShortcuts"), 0);
                    v << dwordT(k, QStringLiteral("DisableDSC"), 1);
                    v << dwordT(k, QStringLiteral("DisableWriteCombining"), 1);
                    v << dwordT(k, QStringLiteral("EnableRTX20Optimizations"), 1);
                    v << dwordT(k, QStringLiteral("FeatureControl"), 4);
                    v << dwordT(k, QStringLiteral("NVDeviceSupportKFilter"), 0);
                    v << dwordT(k, QStringLiteral("PruningMode"), 0);
                    v << dwordT(k, QStringLiteral("TCCSupported"), 0);
                    v << dwordT(k, QStringLiteral("UseBestResolution"), 1);
                }
                return v;
            }) };
            c.push_back(t);
        }
    }

}
