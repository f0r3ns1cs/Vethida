#include "TweakEngine.h"
#include "DeviceOps.h"

namespace vth::tweaks::detail
{
    namespace dev = vth::tweaks::devices;

    namespace
    {
        // Registry targets per adapter. Existing snapshots depend on snapKey's shape.
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
            return on == total ? DetectState::On : DetectState::Off;
        }

        // Re-enumerated each call, so hot-plugged adapters are picked up.
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

        // On the active power scheme, with the earlier values snapshotted.
        struct AcSetting {
            QString sub;
            QString setting;
            quint32 value = 0;
        };

        DynamicOp acOp(const QString& id, const QVector<AcSetting>& settings) {
            DynamicOp op;
            op.label = id;
            op.detect = [settings]() {
                int on = 0, readable = 0;
                for (const AcSetting& s : settings) {
                    const auto v = dev::powercfgQueryAcIndex(s.sub, s.setting);
                    if (!v) continue;
                    ++readable;
                    if (*v == s.value) ++on;
                }
                if (readable == 0)              return DetectState::Unknown;
                return on == readable ? DetectState::On : DetectState::Off;
            };
            op.apply = [id, settings]() {
                QMap<QString, QString> snap;
                bool ok = true;
                for (const AcSetting& s : settings) {
                    QString prior = QStringLiteral("-");
                    if (const auto v = dev::powercfgQueryAcIndex(s.sub, s.setting))
                        prior = QString::number(*v);
                    snap.insert(s.sub + QLatin1Char('|') + s.setting, prior);
                    ok = dev::powercfgSetAcIndex(s.sub, s.setting, s.value) && ok;
                }
                dev::saveSnapshot(id, snap);
                return ok;
            };
            op.revert = [id]() {
                const QMap<QString, QString> snap = dev::loadSnapshot(id);
                bool ok = true;
                for (auto it = snap.begin(); it != snap.end(); ++it) {
                    const int bar = it.key().indexOf(QLatin1Char('|'));
                    if (bar < 0) continue;
                    // Unreadable before apply. A scheme index can't be deleted, so it stays.
                    if (it.value() == QStringLiteral("-")) continue;
                    ok = dev::powercfgSetAcIndex(it.key().left(bar), it.key().mid(bar + 1),
                                                 it.value().toUInt()) && ok;
                }
                dev::clearSnapshot(id);
                return ok;
            };
            return op;
        }

        const QString kVenNvidia = QStringLiteral("VEN_10DE");
        const QString kGd        = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers");
        const QString kNvlddmkm  = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\nvlddmkm");

    }

    void appendOperatorGpuPower(QVector<TweakSpec>& c) {

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.nvidia.disable_hdcp");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("NVIDIA");
            t.name = QStringLiteral("Disable HDCP");
            t.description = QStringLiteral(
                "Turns off HDCP content protection on the NVIDIA GPU's video "
                "outputs, so the display link skips the encryption handshake.");
            t.iconName = QStringLiteral("shield-off");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Some streaming services and Blu-ray playback can refuse to play or drop resolution."),
            };
            t.concern = QStringLiteral(
                "Disabling HDCP also unlocks protected-output capture.");
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                for (const QString& k : dev::enumerateGpuClassKeys(kVenNvidia))
                    v << dwordT(k, QStringLiteral("RMHdcpKeyglobZero"), 1);
                return v;
            }) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.nvidia.fts_gr535_disable");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("NVIDIA");
            t.name = QStringLiteral("Restore legacy image sharpening");
            t.description = QStringLiteral(
                "Brings back the NVIDIA driver's older image-sharpening control, "
                "which newer drivers replace with the overlay filter version.");
            t.iconName = QStringLiteral("photo-cog");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.registryOps = {
                dwordOp(kNvlddmkm + QStringLiteral("\\FTS"), QStringLiteral("EnableGR535"), 0u, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.nvidia.dynamic_pstate_off");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("NVIDIA");
            t.name = QStringLiteral("Disable dynamic performance states");
            t.description = QStringLiteral(
                "Keeps the NVIDIA GPU from stepping its clocks down between bursts "
                "of work, trading idle efficiency for steadier clocks.");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::High;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.effects = {
                QStringLiteral("Idle power draw and temperature rise."),
            };
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                for (const QString& k : dev::enumerateGpuClassKeys(kVenNvidia))
                    v << dwordT(k, QStringLiteral("DisableDynamicPstate"), 1);
                return v;
            }) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.nvidia.telemetry_disable");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("Telemetry");
            t.name = QStringLiteral("Disable NVIDIA telemetry");
            t.description = QStringLiteral(
                "Opts this PC out of the NVIDIA driver's usage reporting, disables "
                "its telemetry service and stops the usage report it sends when "
                "Windows starts.");
            t.iconName = QStringLiteral("eye-off");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.effects = {
                QStringLiteral("Crash reports no longer reach NVIDIA."),
            };
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\NVIDIA Corporation\\NvControlPanel2\\Client"),
                        QStringLiteral("OptInOrOutPreference"), 0u, std::nullopt),
                // Revert restores Automatic, because deleting Start would break the service.
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NvTelemetryContainer"),
                        QStringLiteral("Start"), 4u, 2u),
                dwordOp(kNvlddmkm + QStringLiteral("\\Global\\Startup"),
                        QStringLiteral("SendTelemetryData"), 0u, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.nvidia.disable_cuda_context_preemption");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("NVIDIA");
            t.name = QStringLiteral("Disable CUDA context preemption");
            t.description = QStringLiteral(
                "Stops the NVIDIA driver from pausing long-running GPU compute "
                "work mid-task to service other workloads.");
            t.iconName = QStringLiteral("cpu");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Under heavy GPU compute load the desktop can briefly freeze."),
            };
            t.concern = QStringLiteral(
                "these writes change scheduling, so they are separate from the telemetry switch. "
                "Never suggest unprompted.");
            t.registryOps = {
                dwordOp(kNvlddmkm, QStringLiteral("DisablePreemption"), 1u, std::nullopt),
                dwordOp(kNvlddmkm, QStringLiteral("DisableCudaContextPreemption"), 1u, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.disable-scheduler-preemption");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("GPU scheduling");
            t.name = QStringLiteral("Disable GPU scheduler preemption");
            t.description = QStringLiteral(
                "Stops Windows' GPU scheduler from interrupting running GPU work "
                "to service other tasks. Applies to every GPU in the system, not "
                "just NVIDIA.");
            t.iconName = QStringLiteral("gpu");
            t.impact = Impact::High;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Under heavy GPU load the whole PC can hang until the task finishes."),
            };
            t.concern = QStringLiteral(
                "global WDDM override, materially more aggressive than the per-vendor "
                "preemption tweaks. Never suggest unprompted.");
            t.registryOps = {
                dwordOp(kGd + QStringLiteral("\\Scheduler"), QStringLiteral("EnablePreemption"), 0u, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.nvidia.driver_tuning");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("NVIDIA");
            t.name = QStringLiteral("NVIDIA performance pack");
            t.description = QStringLiteral(
                "One switch for the NVIDIA driver: favors performance in its 3D "
                "settings, spreads its deferred work across processor cores, keeps "
                "the card out of deep idle, trims its latency margins, and turns off "
                "overlay presentation, runtime power management and output content "
                "protection.");
            t.iconName = QStringLiteral("gpu");
            t.impact = Impact::High;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Some streaming services and Blu-ray playback can refuse to play."),
            };
            t.effects = {
                QStringLiteral("The GPU runs hotter and draws more power at idle."),
                QStringLiteral("3D image quality is biased toward speed."),
            };
            t.concern = QStringLiteral(
                "the individual values are unmeasured and the protection/overlay writes are the "
                "sharp edges. Never suggest unprompted.");

            QVector<RegistryOp> o;
            o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\NVIDIA Corporation\\Global\\Anisotropic"),
                                QStringLiteral("Anisotropic Value"), 0u, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\NVIDIA Corporation\\Global\\NvCplApi"
                                               "\\Policies\\QualityVsPerformance"),
                                QStringLiteral("Quality"), 0u, std::nullopt));
            for (const QString& p : { kGd,
                                      QString(kGd + QStringLiteral("\\Power")),
                                      kNvlddmkm,
                                      QString(kNvlddmkm + QStringLiteral("\\NVAPI")),
                                      QString(kNvlddmkm + QStringLiteral("\\Global\\NVTweak")) })
                o.push_back(dwordOp(p, QStringLiteral("RmGpsPsEnablePerCpuCoreDpc"), 1u, std::nullopt));
            for (const QString& n : { QStringLiteral("SupportRuntimePowerManagement"),
                                      QStringLiteral("RuntimePowerManagement"),
                                      QStringLiteral("EnableRuntimePowerManagement"),
                                      QStringLiteral("Protection"),
                                      QStringLiteral("ProtectionLevel"),
                                      QStringLiteral("OPMSetProtectionLevel"),
                                      QStringLiteral("DriverProtection"),
                                      QStringLiteral("NumberOfIdleStates"),
                                      QStringLiteral("PoFxStartDevicePowerManagement"),
                                      QStringLiteral("PoFxPowerControl"),
                                      QStringLiteral("DxgkWaitForIdle"),
                                      QStringLiteral("SchedulingDelay") })
                o.push_back(dwordOp(kGd, n, 0u, std::nullopt));
            o.push_back(dwordOp(kGd, QStringLiteral("DisableOverlays"), 1u, std::nullopt));
            o.push_back(dwordOp(kGd, QStringLiteral("DpiMapIommuContiguous"), 1u, std::nullopt));
            t.registryOps = o;

            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                for (const QString& k : dev::enumerateGpuClassKeys(kVenNvidia)) {
                    v << dwordT(k, QStringLiteral("PowerMizerLevel"), 1)
                      << dwordT(k, QStringLiteral("D3PCLatency"), 1)
                      << dwordT(k, QStringLiteral("F1TransitionLatency"), 1)
                      << dwordT(k, QStringLiteral("LOWLATENCY"), 1)
                      << dwordT(k, QStringLiteral("Node3DLowLatency"), 1)
                      << dwordT(k, QStringLiteral("PciLatencyTimerControl"), 0x20)
                      << dwordT(k, QStringLiteral("RMDeepL1EntryLatencyUsec"), 1)
                      << dwordT(k, QStringLiteral("RmGspcMaxFtuS"), 1)
                      << dwordT(k, QStringLiteral("RmGspcMinFtuS"), 1)
                      << dwordT(k, QStringLiteral("RmGspcPerioduS"), 1)
                      << dwordT(k, QStringLiteral("RMLpwrEiIdleThresholdUs"), 1)
                      << dwordT(k, QStringLiteral("RMLpwrGrIdleThresholdUs"), 1)
                      << dwordT(k, QStringLiteral("RMLpwrGrRgIdleThresholdUs"), 1)
                      << dwordT(k, QStringLiteral("RMLpwrMsIdleThresholdUs"), 1)
                      << dwordT(k, QStringLiteral("VRDirectFlipDPCDelayUsy"), 1)
                      << dwordT(k, QStringLiteral("VRDirectFlipTimingMarginUs"), 1)
                      << dwordT(k, QStringLiteral("VRDirectJITFlipMsHybridFlipDelayUs"), 1)
                      << dwordT(k, QStringLiteral("vrrCursorMarginUs"), 1)
                      << dwordT(k, QStringLiteral("vrrDeflickerMarginUs"), 1)
                      << dwordT(k, QStringLiteral("vrrDeflickerMaxUs"), 1)
                      << dwordT(k, QStringLiteral("RmFbsrPagedDMA"), 1)
                      << dwordT(k, QStringLiteral("UseGpuTimer"), 1)
                      << dwordT(k, QStringLiteral("RMOPSB"), 0x2aa2)
                      << dwordT(k, QStringLiteral("RmClkPowerOffDramPllWhenUnused"), 0)
                      << dwordT(k, QStringLiteral("RmCeUseGen4Mapping"), 1);
                }
                return v;
            }) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("power.hibernate-off");
            t.category = QStringLiteral("power");
            t.subCategory = QStringLiteral("Hibernation");
            t.name = QStringLiteral("Disable hibernation");
            t.description = QStringLiteral(
                "Turns hibernation off and removes its file from disk, freeing space equal "
                "to your installed memory. Fast startup turns off with it.");
            t.iconName = QStringLiteral("power");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.effects = {
                QStringLiteral("Fast startup stops working too, so a cold boot takes a little longer."),
            };
            CommandOp op;
            op.program    = QStringLiteral("powercfg");
            op.applyArgs  = { QStringLiteral("/h"), QStringLiteral("off") };
            op.revertArgs = { QStringLiteral("/h"), QStringLiteral("on") };
            t.commandOps = { op };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("power.fast-startup-off");
            t.category = QStringLiteral("power");
            t.subCategory = QStringLiteral("Fast startup");
            t.name = QStringLiteral("Disable fast startup");
            t.description = QStringLiteral(
                "Makes every boot a full, clean start instead of resuming a saved "
                "kernel image, clearing the stale driver state fast startup can "
                "carry from one boot to the next.");
            t.iconName = QStringLiteral("power");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.effects = {
                QStringLiteral("Starting the PC takes a few seconds longer."),
            };
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Power"),
                        QStringLiteral("HiberbootEnabled"), 0u, 1u),
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"),
                        QStringLiteral("HiberFileSizePercent"), 0u, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("perf.power-throttling-off");
            t.category = QStringLiteral("general");
            t.subCategory = QStringLiteral("Power throttling");
            t.name = QStringLiteral("Disable power throttling");
            t.description = QStringLiteral(
                "Stops Windows slowing background programs down to save power. Programs you"
                " are using are never slowed, so this mostly matters on a laptop.");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.effects = {
                QStringLiteral("Idle power draw and temperature rise."),
            };
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerThrottling"),
                        QStringLiteral("PowerThrottlingOff"), 1u, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("power.active-plan-tuning");
            t.category = QStringLiteral("power");
            t.subCategory = QStringLiteral("Power plan");
            t.name = QStringLiteral("Keep USB and processor cores awake");
            t.description = QStringLiteral(
                "On the power plan in use: stops USB devices being suspended to save power,"
                " keeps every processor core awake instead of parked, and checks processor "
                "performance more often.");
            t.iconName = QStringLiteral("battery");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.effects = {
                QStringLiteral("Idle power draw rises, most noticeably on a laptop."),
            };
            const QString subUsb  = QStringLiteral("2a737441-1930-4402-8d77-b2bebba308a3");
            const QString subProc = QStringLiteral("54533251-82be-4824-96c1-47b60b740d00");
            t.dynamicOps = { acOp(t.id, {
                // USB 3 link power management -> off
                { subUsb,  QStringLiteral("d4e98f31-5ffe-4ce1-be31-1b38b384c009"), 0 },
                // USB selective suspend -> disabled
                { subUsb,  QStringLiteral("48e6b7a6-50f5-4782-a5d4-53bb8f07e226"), 0 },
                // core parking max/min cores -> 100% (parking off)
                { subProc, QStringLiteral("0cc5b647-c1df-4637-891a-dec35c318583"), 100 },
                { subProc, QStringLiteral("0cc5b647-c1df-4637-891a-dec35c318584"), 100 },
                // performance time-check interval -> 750ms
                { subProc, QStringLiteral("4d2b0152-7d5c-498b-88e2-34345392a2c5"), 750 },
            }) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("power.pcie-aspm-off");
            t.category = QStringLiteral("power");
            t.subCategory = QStringLiteral("PCI Express");
            t.name = QStringLiteral("Keep PCI Express links awake");
            t.description = QStringLiteral(
                "Stops Windows putting the links to your graphics card, SSDs and "
                "other expansion cards into low-power states, so they never pause "
                "to wake up.");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.effects = {
                QStringLiteral("Idle power draw rises a little, most on a laptop."),
            };
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\pci\\Parameters"),
                        QStringLiteral("ASPMOptOut"), 1u, std::nullopt),
            };
            t.dynamicOps = { acOp(t.id, {
                // PCI Express > Link State Power Management -> Off
                { QStringLiteral("501a4d13-42af-4429-9fd1-a8218c268e20"),
                  QStringLiteral("ee12f906-d277-404b-b6da-e5fa1a576df5"), 0 },
            }) };
            c.push_back(t);
        }

        // Not the event log, UBPM, ReadyBoot or TPM sessions, which Windows depends on.
        {
            TweakSpec t;
            t.id = QStringLiteral("system.trace-loggers-off");
            t.category = QStringLiteral("general");
            t.subCategory = QStringLiteral("Logging");
            t.name = QStringLiteral("Turn off background diagnostic logging");
            t.description = QStringLiteral(
                "Stops the trace logs Windows records from startup for diagnostics "
                "and telemetry: network, Wi-Fi, setup, print and usage traces. "
                "Event Viewer keeps working.");
            t.iconName = QStringLiteral("file");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.effects = {
                QStringLiteral("Microsoft support and the built-in troubleshooters have less to go on."),
            };
            t.dynamicOps = { targetOp(t.id, []() {
                static const QStringList kSessions = {
                    QStringLiteral("Cellcore"),
                    QStringLiteral("CloudExperienceHostOobe"),
                    QStringLiteral("DiagLog"),
                    QStringLiteral("LwtNetLog"),
                    QStringLiteral("Mellanox-Kernel"),
                    QStringLiteral("Microsoft-Windows-AssignedAccess-Trace"),
                    QStringLiteral("Microsoft-Windows-Setup"),
                    QStringLiteral("NBSMBLOGGER"),
                    QStringLiteral("NtfsLog"),
                    QStringLiteral("PEAuthLog"),
                    QStringLiteral("RadioMgr"),
                    QStringLiteral("RdrLog"),
                    QStringLiteral("SetupPlatform"),
                    QStringLiteral("SetupPlatformTel"),
                    QStringLiteral("SpoolerLogger"),
                    QStringLiteral("SQMLogger"),
                    QStringLiteral("TCPIPLOGGER"),
                    QStringLiteral("TileStore"),
                    QStringLiteral("WdiContextLog"),
                    QStringLiteral("WFP-IPsec Trace"),
                    QStringLiteral("WiFiDriverIHVSession"),
                    QStringLiteral("WiFiDriverIHVSessionRepro"),
                    QStringLiteral("WiFiSession"),
                    QStringLiteral("WinPhoneCritical"),
                };
                const QString root = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger");
                const QStringList present = dev::regSubKeys(root);
                QVector<RegTarget> v;
                for (const QString& s : kSessions)
                    if (present.contains(s, Qt::CaseInsensitive))
                        v << dwordT(root + QLatin1Char('\\') + s, QStringLiteral("Start"), 0);
                return v;
            }) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.nvidia.write-combining-off");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("NVIDIA");
            t.name = QStringLiteral("Disable NVIDIA write combining");
            t.description = QStringLiteral(
                "Has the NVIDIA driver send writes to the card one at a time instead "
                "of batching them. Some players report steadier input timing.");
            t.iconName = QStringLiteral("gpu");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.effects = {
                QStringLiteral("Uploads to the graphics card get slower, which can cost frame rate in some games."),
            };
            t.concern = QStringLiteral(
                "read by nvlddmkm per its registry-read dump; effect on drivers after 451.48 "
                "is disputed and unmeasured. Never suggest unprompted.");
            t.registryOps = {
                dwordOp(kNvlddmkm, QStringLiteral("DisableWriteCombining"), 1u, std::nullopt),
            };
            c.push_back(t);
        }
    }
}
