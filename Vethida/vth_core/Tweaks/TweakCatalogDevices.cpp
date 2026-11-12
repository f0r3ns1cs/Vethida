#include "TweakEngine.h"
#include "DeviceOps.h"

// Per-device and dynamic-state tweaks built on DynamicOp and DeviceOps: things a
// static registry or command op can't express, like walking every NIC, GPU or
// controller at runtime, powercfg, and WMI or PowerShell state. Every change
// snapshots its prior value so the revert is exact.
//
// Defaults: RSS uses 2 queues, interrupt moderation stays on, and interrupt
// affinity is spread rather than pinned to one core.

namespace vth::tweaks::detail {

    namespace dev = vth::tweaks::devices;

    namespace {

        struct RegTarget {
            QString path;
            QString name;
            bool    isDword = true;
            quint32 dword   = 0;
            QString str;
        };

        QString snapKey(const QString& path, const QString& name) {
            return path + QLatin1Char('|') + name;
        }

        // Snapshot every target's prior value, then write the applied value. Prior
        // state is encoded "d:<n>" / "s:<text>" / "-" (absent) so revert can restore or
        // delete precisely.
        bool applyTargets(const QString& id, const QVector<RegTarget>& targets) {
            QMap<QString, QString> snap;
            bool ok = true;
            for (const RegTarget& t : targets) {
                QString prior = QStringLiteral("-");
                if (t.isDword) {
                    if (const auto v = dev::regReadDword(t.path, t.name))
                        prior = QStringLiteral("d:") + QString::number(*v);
                } else {
                    if (const auto v = dev::regReadString(t.path, t.name))
                        prior = QStringLiteral("s:") + *v;
                }
                snap.insert(snapKey(t.path, t.name), prior);
                ok = (t.isDword ? dev::regWriteDword(t.path, t.name, t.dword)
                                : dev::regWriteString(t.path, t.name, t.str)) && ok;
            }
            dev::saveSnapshot(id, snap);
            return ok;
        }

        bool revertTargets(const QString& id) {
            const QMap<QString, QString> snap = dev::loadSnapshot(id);
            bool ok = true;
            for (auto it = snap.begin(); it != snap.end(); ++it) {
                const int bar = it.key().lastIndexOf(QLatin1Char('|'));
                if (bar < 0) continue;
                const QString path = it.key().left(bar);
                const QString name = it.key().mid(bar + 1);
                const QString v = it.value();
                if (v == QStringLiteral("-"))
                    ok = dev::regDeleteValue(path, name) && ok;
                else if (v.startsWith(QStringLiteral("d:")))
                    ok = dev::regWriteDword(path, name, v.mid(2).toUInt()) && ok;
                else if (v.startsWith(QStringLiteral("s:")))
                    ok = dev::regWriteString(path, name, v.mid(2)) && ok;
            }
            dev::clearSnapshot(id);
            return ok;
        }

        DetectState detectTargets(const QVector<RegTarget>& targets) {
            if (targets.isEmpty()) return DetectState::Unknown;
            int on = 0, total = 0;
            for (const RegTarget& t : targets) {
                ++total;
                if (t.isDword) {
                    const auto v = dev::regReadDword(t.path, t.name);
                    if (v && *v == t.dword) ++on;
                } else {
                    const auto v = dev::regReadString(t.path, t.name);
                    if (v && v->compare(t.str, Qt::CaseInsensitive) == 0) ++on;
                }
            }
            // partly applied reads as off
            if (on == total) return DetectState::On;
            return DetectState::Off;
        }

        // Build a DynamicOp from a target-builder closure (re-enumerated each call so
        // hot-plugged hardware is picked up) + a stable snapshot id.
        DynamicOp targetOp(const QString& id, std::function<QVector<RegTarget>()> build) {
            DynamicOp op;
            op.label  = id;
            op.detect = [build]() { return detectTargets(build()); };
            op.apply  = [id, build]() { return applyTargets(id, build()); };
            op.revert = [id]() { return revertTargets(id); };
            return op;
        }

        RegTarget dwordT(const QString& path, const QString& name, quint32 v) {
            return RegTarget{ path, name, true, v, QString() };
        }
        RegTarget stringT(const QString& path, const QString& name, const QString& v) {
            return RegTarget{ path, name, false, 0, v };
        }

        const QString kTcpipIf = QStringLiteral(
            "HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\");
        const QString kNetbtIf = QStringLiteral(
            "HKLM\\SYSTEM\\CurrentControlSet\\Services\\NetBT\\Parameters\\Interfaces\\Tcpip_");

        bool classIsInterruptTarget(const QString& classGuidLower) {
            static const QStringList kClasses = {
                QStringLiteral("{4d36e968-e325-11ce-bfc1-08002be10318}"), // Display
                QStringLiteral("{4d36e972-e325-11ce-bfc1-08002be10318}"), // Net
                QStringLiteral("{4d36e97b-e325-11ce-bfc1-08002be10318}"), // SCSIAdapter
                QStringLiteral("{4d36e96a-e325-11ce-bfc1-08002be10318}"), // HDC
                QStringLiteral("{36fc9e60-c465-11cf-8056-444553540000}"), // USB
                QStringLiteral("{4d36e96c-e325-11ce-bfc1-08002be10318}"), // Media/audio
            };
            return kClasses.contains(classGuidLower);
        }

        // One reversible BCD element edit on {current}. `applied` holds the value
        // to set; a nullopt `applied` means "delete this element on apply" (used
        // to remove a harmful override such as useplatformclock).
        struct BcdEdit {
            QString                element;
            std::optional<QString> applied;
        };

        // Build a DynamicOp over a set of BCD element edits with an exact presence
        // snapshot. Apply records each element's prior value ("v:<val>" or "-" for
        // absent) before writing; revert restores the captured value, or deletes
        // the override so Windows falls back to its own default. Detect is Unknown:
        // reading BCD reliably needs elevation and the value strings are not worth
        // parsing for a cosmetic state read - the same choice the WMI ops make.
        DynamicOp bcdOp(const QString& id, const QVector<BcdEdit>& edits) {
            DynamicOp op;
            op.label  = id;
            op.detect = []() { return DetectState::Unknown; };
            op.apply  = [id, edits]() {
                QMap<QString, QString> snap;
                for (const BcdEdit& e : edits) {
                    const auto prior = dev::bcdReadElement(e.element);
                    snap.insert(e.element,
                        prior ? QStringLiteral("v:") + *prior : QStringLiteral("-"));
                }
                dev::saveSnapshot(id, snap);
                bool ok = true;
                for (const BcdEdit& e : edits) {
                    if (e.applied.has_value())
                        ok = dev::bcdSetElement(e.element, *e.applied) && ok;
                    else if (snap.value(e.element) != QStringLiteral("-"))
                        ok = dev::bcdDeleteElement(e.element) && ok;   // only if present
                }
                return ok;
            };
            op.revert = [id, edits]() {
                const QMap<QString, QString> snap = dev::loadSnapshot(id);
                bool ok = true;
                for (const BcdEdit& e : edits) {
                    const QString v = snap.value(e.element, QStringLiteral("-"));
                    if (v.startsWith(QStringLiteral("v:")))
                        ok = dev::bcdSetElement(e.element, v.mid(2)) && ok;
                    else if (e.applied.has_value())
                        ok = dev::bcdDeleteElement(e.element) && ok;   // undo our set
                    // absent-before + delete-on-apply: nothing to restore
                }
                dev::clearSnapshot(id);
                return ok;
            };
            return op;
        }

        // Clamp a captured Get-NetOffloadGlobalSetting state to one of the two
        // literals we ever pass back to Set-NetOffloadGlobalSetting, so a snapshot
        // can never inject anything into the command line.
        QString offloadStateOrDefault(const QString& v) {
            return v.compare(QStringLiteral("Disabled"), Qt::CaseInsensitive) == 0
                 ? QStringLiteral("Disabled")
                 : QStringLiteral("Enabled");
        }
    }


    void appendDevices(QVector<TweakSpec>& c) {
        // per-NIC Nagle
        {
            TweakSpec t;
            t.id = QStringLiteral("net.per-nic-nagle");
            t.category = QStringLiteral("networking");
            t.subCategory = QStringLiteral("TCP/IP");
            t.name = QStringLiteral("Disable Nagle (per adapter)");
            t.description = QStringLiteral("Sets TcpAckFrequency=1, TCPNoDelay=1 and TcpDelAckTicks=0 on every physical adapter's TCP/IP interface, disabling Nagle coalescing and delayed ACK. This only affects TCP traffic - it measurably lowers latency for TCP-based games and remote-desktop sessions but does nothing for the many titles that use UDP.");
            t.iconName = QStringLiteral("gauge");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                for (const auto& nic : dev::enumeratePhysicalNics()) {
                    const QString p = kTcpipIf + nic.instanceGuid;
                    v << dwordT(p, QStringLiteral("TcpAckFrequency"), 1)
                    << dwordT(p, QStringLiteral("TCPNoDelay"), 1)
                    << dwordT(p, QStringLiteral("TcpDelAckTicks"), 0);
                }
                return v;
            }) };
            c.push_back(t);
        }

        // per-NIC RSS (2 queues)
        {
            TweakSpec t;
            t.id = QStringLiteral("net.per-nic-rss");
            t.category = QStringLiteral("networking");
            t.subCategory = QStringLiteral("Adapter");
            t.name = QStringLiteral("Receive-side scaling (per adapter)");
            t.description = QStringLiteral("Enables Receive-Side Scaling on each physical adapter and limits it to 2 receive queues starting at processor 2. RSS itself is beneficial for spreading receive load; the 2-queue cap is a conservative default rather than a proven win - on a consumer connection the exact queue count rarely changes measured performance.");
            t.iconName = QStringLiteral("network");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                for (const auto& nic : dev::enumeratePhysicalNics()) {
                    v << stringT(nic.classKeyPath, QStringLiteral("*RSS"), QStringLiteral("1"))
                    << stringT(nic.classKeyPath, QStringLiteral("*NumRssQueues"), QStringLiteral("2"))
                    << stringT(nic.classKeyPath, QStringLiteral("*RssBaseProcNumber"), QStringLiteral("2"))
                    << stringT(nic.classKeyPath, QStringLiteral("*MaxRssProcessors"), QStringLiteral("2"));
                }
                return v;
            }) };
            c.push_back(t);
        }

        // per-NIC NetBIOS-over-TCP/IP off (binding cleanup)
        {
            TweakSpec t;
            t.id = QStringLiteral("net.per-nic-netbios-off");
            t.category = QStringLiteral("networking");
            t.subCategory = QStringLiteral("NetBIOS");
            t.name = QStringLiteral("Disable NetBIOS over TCP/IP (per adapter)");
            t.description = QStringLiteral(
                "Sets NetbiosOptions=2 on every physical adapter, removing the legacy NetBIOS "
                "name-resolution chatter and its attack surface.");
            t.iconName = QStringLiteral("network");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                for (const auto& nic : dev::enumeratePhysicalNics())
                    v << dwordT(kNetbtIf + nic.instanceGuid, QStringLiteral("NetbiosOptions"), 2);
                return v;
            }) };
            c.push_back(t);
        }

        // AMD GPU: maximum performance (full PP_* set)
        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.amd-ulps-off");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("AMD");
            t.name = QStringLiteral("AMD: maximum performance");
            t.description = QStringLiteral(
                "On every AMD (VEN_1002) adapter: disables ULPS, deep SCLK sleep, clock/power/"
                "DRM-DMA gating, thermal auto-throttle and GPU power-down, and forces the 3D "
                "performance state - the registry equivalent of pinning the card to full clocks.");
            t.iconName = QStringLiteral("monitor");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "AMD-only; raises idle power and temps. DalDramClockChangeLatencyNs is "
                "deliberately NOT written (it causes screen flicker on many AMD GPUs).");
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                for (const QString& k : dev::enumerateGpuClassKeys(QStringLiteral("VEN_1002"))) {
                    v << dwordT(k, QStringLiteral("EnableUlps"), 0)
                    << dwordT(k, QStringLiteral("PP_SclkDeepSleepDisable"), 1)
                    << dwordT(k, QStringLiteral("PP_ThermalAutoThrottlingEnable"), 0)
                    << dwordT(k, QStringLiteral("PP_GPUPowerDownEnabled"), 0)
                    << dwordT(k, QStringLiteral("PP_DisableClockGating"), 1)
                    << dwordT(k, QStringLiteral("PP_DisablePowerGating"), 1)
                    << dwordT(k, QStringLiteral("PP_DisableDrmdmaPowerGating"), 1)
                    << dwordT(k, QStringLiteral("PP_Force3DPerformanceMode"), 1)
                    << dwordT(k, QStringLiteral("PP_DisableAutoWattman"), 1);
                }
                return v;
            }) };
            c.push_back(t);
        }

        // NVIDIA GPU: maximum performance (registry, VEN_10DE class keys)
        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.nvidia-max-performance");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("NVIDIA");
            t.name = QStringLiteral("NVIDIA: maximum performance (registry)");
            t.description = QStringLiteral("On every NVIDIA adapter this writes registry keys intended to force P-state 0 / PowerMizer maximum, plus MSI mode and per-core DPC distribution. Note that the P-state and PowerMizer keys are largely ignored by driver branch 525 and newer - the reliable way to lock 'Prefer Maximum Performance' is the NVIDIA Control Panel power-management setting. The MSI-mode and per-core-DPC portions do still apply.");
            t.iconName = QStringLiteral("monitor");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "NVIDIA-only; higher idle power/temps. Overlaps the per-session Genesis "
                "NVIDIA profile - this one is permanent. Disabling preemption can briefly hang "
                "the desktop under very heavy GPU load.");
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                for (const QString& k : dev::enumerateGpuClassKeys(QStringLiteral("VEN_10DE"))) {
                    v << dwordT(k, QStringLiteral("DisableDynamicPstate"), 1)
                    << dwordT(k, QStringLiteral("PowerMizerEnable"), 1)
                    << dwordT(k, QStringLiteral("PowerMizerLevel"), 1)
                    << dwordT(k, QStringLiteral("PowerMizerLevelAC"), 1)
                    << dwordT(k, QStringLiteral("PowerMizerDefault"), 1)
                    << dwordT(k, QStringLiteral("PowerMizerDefaultAC"), 1)
                    << dwordT(k, QStringLiteral("PerfLevelSrc"), 0x2222)
                    << dwordT(k, QStringLiteral("RMEnablePreemption"), 0)
                    << dwordT(k, QStringLiteral("EnableMSI"), 1)
                    << dwordT(k, QStringLiteral("RmGpsPsEnablePerCpuCoreDpc"), 1)
                    << dwordT(k, QStringLiteral("RMDisableGPUIdlePowerSaving"), 1);
                }
                return v;
            }) };
            c.push_back(t);
        }

        // per-NIC power-saving / green-ethernet off
        {
            TweakSpec t;
            t.id = QStringLiteral("net.per-nic-powersaving-off");
            t.category = QStringLiteral("networking");
            t.subCategory = QStringLiteral("Power saving");
            t.name = QStringLiteral("Disable NIC power saving (per adapter)");
            t.description = QStringLiteral(
                "Turns off Energy-Efficient Ethernet, Green Ethernet, gigabit auto-downshift "
                "and selective power-down on every physical adapter, so the link never drops "
                "speed or sleeps mid-session.");
            t.iconName = QStringLiteral("network");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                for (const auto& nic : dev::enumeratePhysicalNics()) {
                    const QString k = nic.classKeyPath;
                    v << stringT(k, QStringLiteral("*EEE"), QStringLiteral("0"))
                    << stringT(k, QStringLiteral("EnableGreenEthernet"), QStringLiteral("0"))
                    << stringT(k, QStringLiteral("AdvancedEEE"), QStringLiteral("0"))
                    << stringT(k, QStringLiteral("AutoDisableGigabit"), QStringLiteral("0"))
                    << stringT(k, QStringLiteral("EnablePME"), QStringLiteral("0"))
                    << stringT(k, QStringLiteral("EnableSavePowerNow"), QStringLiteral("0"))
                    << stringT(k, QStringLiteral("AutoPowerSaveModeEnabled"), QStringLiteral("0"));
                }
                return v;
            }) };
            c.push_back(t);
        }

        // per-USB-hub power saving off (WMI MSPower_DeviceEnable)
        {
            TweakSpec t;
            t.id = QStringLiteral("usb.hub-powersaving-off");
            t.category = QStringLiteral("power");
            t.subCategory = QStringLiteral("USB power");
            t.name = QStringLiteral("Disable USB hub power saving");
            t.description = QStringLiteral(
                "Walks every USB root hub and clears the \"allow the computer to turn off this "
                "device\" power-management flag via WMI, removing wake-up latency on input.");
            t.iconName = QStringLiteral("plug-zap");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            DynamicOp op;
            op.label = t.id;
            op.detect = []() { return DetectState::Unknown; };   // WMI instance state, no cheap read
            op.apply = []() {
                return dev::runPowershell(QStringLiteral(
                    "$ErrorActionPreference='SilentlyContinue';"
                    "Get-PnpDevice | Where-Object {$_.InstanceId -like 'USB\\ROOT*'} | ForEach-Object {"
                    "$e=[regex]::Escape($_.InstanceId);"
                    "Get-CimInstance -ClassName MSPower_DeviceEnable -Namespace root\\wmi | "
                    "Where-Object {$_.InstanceName -like \"*$e*\"} | "
                    "Set-CimInstance -Property @{Enable=$false}}"), nullptr, 60000);
            };
            op.revert = []() {
                return dev::runPowershell(QStringLiteral(
                    "$ErrorActionPreference='SilentlyContinue';"
                    "Get-PnpDevice | Where-Object {$_.InstanceId -like 'USB\\ROOT*'} | ForEach-Object {"
                    "$e=[regex]::Escape($_.InstanceId);"
                    "Get-CimInstance -ClassName MSPower_DeviceEnable -Namespace root\\wmi | "
                    "Where-Object {$_.InstanceName -like \"*$e*\"} | "
                    "Set-CimInstance -Property @{Enable=$true}}"), nullptr, 60000);
            };
            t.dynamicOps = { op };
            c.push_back(t);
        }

        // per-port SATA link power management off
        {
            TweakSpec t;
            t.id = QStringLiteral("storage.sata-link-power-off");
            t.category = QStringLiteral("storage");
            t.subCategory = QStringLiteral("SATA");
            t.name = QStringLiteral("Disable SATA link power management");
            t.description = QStringLiteral(
                "Writes EnableHIPM=0 and EnableDIPM=0 under every storahci controller's "
                "StorPort device parameters, stopping AHCI link power management from adding "
                "wake latency to SATA I/O.");
            t.iconName = QStringLiteral("database");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral("desktop-oriented; on laptops it raises SATA idle power.");
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                for (const auto& p : dev::enumeratePciInstances()) {
                    if (p.service != QStringLiteral("storahci")) continue;
                    const QString sp = p.instanceKeyPath
                        + QStringLiteral("\\Device Parameters\\StorPort");
                    v << dwordT(sp, QStringLiteral("EnableHIPM"), 0)
                    << dwordT(sp, QStringLiteral("EnableDIPM"), 0);
                }
                return v;
            }) };
            c.push_back(t);
        }

        // MSI mode for interrupt-capable devices
        {
            TweakSpec t;
            t.id = QStringLiteral("kernel.msi-mode");
            t.category = QStringLiteral("advanced");
            t.subCategory = QStringLiteral("Interrupts");
            t.name = QStringLiteral("Enable MSI mode (GPU / NIC / storage / USB / audio)");
            t.description = QStringLiteral(
                "Sets MSISupported=1 on every display, network, storage, USB and audio "
                "controller, switching them from legacy line-based interrupts to message-"
                "signaled interrupts (private channel, lower ISR contention).");
            t.iconName = QStringLiteral("cpu");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "rare hardware misbehaves on MSI; if a device stops working after reboot, "
                "revert and reboot again.");
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                for (const auto& p : dev::enumeratePciInstances()) {
                    if (!classIsInterruptTarget(p.classGuid)) continue;
                    const QString mp = p.instanceKeyPath + QStringLiteral(
                        "\\Device Parameters\\Interrupt Management\\MessageSignaledInterruptProperties");
                    v << dwordT(mp, QStringLiteral("MSISupported"), 1);
                }
                return v;
            }) };
            c.push_back(t);
        }

        // interrupt affinity: spread (not pinned)
        {
            TweakSpec t;
            t.id = QStringLiteral("kernel.interrupt-affinity-spread");
            t.category = QStringLiteral("advanced");
            t.subCategory = QStringLiteral("Interrupts");
            t.name = QStringLiteral("Spread device interrupts across CPUs");
            t.description = QStringLiteral(
                "Sets each interrupt-capable device's affinity policy to spread its interrupts "
                "across all processors instead of stacking on CPU 0.");
            t.iconName = QStringLiteral("cpu");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "uses \"spread\" (DevicePolicy=5), not a fixed-core pin: empirically the best "
                "core varies by hardware and manual pinning hurts as often as it helps.");
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                for (const auto& p : dev::enumeratePciInstances()) {
                    if (!classIsInterruptTarget(p.classGuid)) continue;
                    const QString ap = p.instanceKeyPath + QStringLiteral(
                        "\\Device Parameters\\Interrupt Management\\Affinity Policy");
                    v << dwordT(ap, QStringLiteral("DevicePolicy"), 5);
                }
                return v;
            }) };
            c.push_back(t);
        }

        // granular SUB_PROCESSOR powercfg with EXACT snapshot revert
        {
            TweakSpec t;
            t.id = QStringLiteral("power.cpu-max-performance");
            t.category = QStringLiteral("power");
            t.subCategory = QStringLiteral("Processor power");
            t.name = QStringLiteral("Maximum CPU performance (per-plan)");
            t.description = QStringLiteral(
                "Unparks all cores and pins the processor to 100% on the active plan "
                "(CPMINCORES=100, PROCTHROTTLEMIN=100, PERFBOOSTMODE=Aggressive, PERFEPP=0).");
            t.iconName = QStringLiteral("cpu");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "implemented with an exact powercfg /query snapshot (no \"guessed default\" "
                "revert). Raises idle power/temps; laptops drain faster.");
            DynamicOp op;
            op.label = t.id;
            const QString id = t.id;
            // (sub alias, setting alias, applied value)
            struct PcSetting { QString setting; quint32 value; };
            static const QVector<PcSetting> kSettings = {
                { QStringLiteral("CPMINCORES"), 100 },
                { QStringLiteral("PROCTHROTTLEMIN"), 100 },
                { QStringLiteral("PROCTHROTTLEMAX"), 100 },
                { QStringLiteral("PERFBOOSTMODE"), 2 },
                { QStringLiteral("PERFEPP"), 0 },
                { QStringLiteral("PERFAUTONOMOUS"), 0 },
            };
            op.detect = [id]() {
                int on = 0, total = 0;
                for (const auto& s : kSettings) {
                    ++total;
                    const auto cur = dev::powercfgQueryAcIndex(QStringLiteral("SUB_PROCESSOR"), s.setting);
                    if (cur && *cur == s.value) ++on;
                }
                if (total == 0) return DetectState::Unknown;
                if (on == total) return DetectState::On;
                return DetectState::Off;
            };
            op.apply = [id]() {
                QMap<QString, QString> snap;
                bool ok = true;
                for (const auto& s : kSettings) {
                    const auto prior = dev::powercfgQueryAcIndex(QStringLiteral("SUB_PROCESSOR"), s.setting);
                    snap.insert(s.setting, prior ? QString::number(*prior) : QStringLiteral("-"));
                    ok = dev::powercfgSetAcIndex(QStringLiteral("SUB_PROCESSOR"), s.setting, s.value) && ok;
                }
                dev::saveSnapshot(id, snap);
                return ok;
            };
            op.revert = [id]() {
                const QMap<QString, QString> snap = dev::loadSnapshot(id);
                bool ok = true;
                for (auto it = snap.begin(); it != snap.end(); ++it) {
                    if (it.value() == QStringLiteral("-")) continue;   // no prior captured
                    ok = dev::powercfgSetAcIndex(QStringLiteral("SUB_PROCESSOR"),
                                                it.key(), it.value().toUInt()) && ok;
                }
                dev::clearSnapshot(id);
                return ok;
            };
            t.dynamicOps = { op };
            c.push_back(t);
        }

        // fixed pagefile (WMI)
        {
            TweakSpec t;
            t.id = QStringLiteral("mem.fixed-pagefile");
            t.category = QStringLiteral("advanced");
            t.subCategory = QStringLiteral("Pagefile");
            t.name = QStringLiteral("Fixed pagefile");
            t.description = QStringLiteral(
                "Turns off automatic pagefile management and pins a fixed pagefile (sized to "
                "physical RAM, clamped 4-16 GB) on the system drive, removing dynamic-resize "
                "overhead under memory pressure.");
            t.iconName = QStringLiteral("layers");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            DynamicOp op;
            op.label = t.id;
            op.detect = []() {
                const auto autom = dev::pagefileIsAutomatic();
                if (!autom) return DetectState::Unknown;
                return *autom ? DetectState::Off : DetectState::On;
            };
            op.apply  = []() { return dev::setFixedPagefile(); };
            op.revert = []() { return dev::setAutomaticPagefile(); };
            t.dynamicOps = { op };
            c.push_back(t);
        }

        // RAM compression off (MMAgent)
        {
            TweakSpec t;
            t.id = QStringLiteral("mem.ram-compression-off");
            t.category = QStringLiteral("advanced");
            t.subCategory = QStringLiteral("Memory compression");
            t.name = QStringLiteral("Disable memory compression");
            t.description = QStringLiteral("Disables the Memory Manager's RAM compression (Disable-MMAgent). Evidence is mixed: on 32 GB+ systems memory pressure is rarely reached so this is essentially a placebo, and on low-RAM systems it can make things worse by forcing more disk paging. Any real benefit is confined to specific mid-RAM, memory-heavy workloads.");
            t.iconName = QStringLiteral("layers");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "only a win with ample RAM; on memory-constrained systems it increases paging.");
            DynamicOp op;
            op.label = t.id;
            op.detect = []() {
                const auto on = dev::memoryCompressionEnabled();
                if (!on) return DetectState::Unknown;
                return *on ? DetectState::Off : DetectState::On;   // tweak On == compression off
            };
            op.apply  = []() { return dev::setMemoryCompression(false); };
            op.revert = []() { return dev::setMemoryCompression(true); };
            t.dynamicOps = { op };
            c.push_back(t);
        }

        // svchost split threshold, computed from installed RAM at apply time
        {
            TweakSpec t;
            t.id = QStringLiteral("advanced.svchost-split-threshold");
            t.category = QStringLiteral("advanced");
            t.subCategory = QStringLiteral("Services");
            t.name = QStringLiteral("Consolidate svchost processes");
            t.description = QStringLiteral(
                "Raises SvcHostSplitThresholdInKB to the machine's total physical RAM (read at "
                "apply time) so Windows stops giving each shared service its own process and "
                "groups them back into fewer svchost.exe hosts. Lowers process count and "
                "per-process overhead, but reduces service isolation - a fault in one grouped "
                "service can affect its neighbors. The effect is marginal on modern systems.");
            t.iconName = QStringLiteral("layers");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "reduces service isolation (grouped services share one process); the benefit is "
                "marginal on machines with ample RAM. Reverts by restoring the prior value, or "
                "deleting it so Windows uses its own default threshold - never a guessed one.");
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                if (const auto ramKb = dev::physicalRamKb()) {
                    const quint32 threshold =
                        static_cast<quint32>(qMin<quint64>(*ramKb, 0xFFFFFFFFull));
                    v << dwordT(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control"),
                                QStringLiteral("SvcHostSplitThresholdInKB"), threshold);
                }
                return v;
            }) };
            c.push_back(t);
        }

        // BCD: disable the dynamic tick and remove any forced platform clock
        {
            TweakSpec t;
            t.id = QStringLiteral("advanced.boot-disable-dynamic-tick");
            t.category = QStringLiteral("advanced");
            t.subCategory = QStringLiteral("Boot / BCD");
            t.name = QStringLiteral("Disable dynamic tick (BCD)");
            t.description = QStringLiteral(
                "Sets the boot-loader element disabledynamictick=yes and removes any "
                "useplatformclock override so the system uses the modern tickless timer instead "
                "of being pinned to the HPET. Some latency-focused guides recommend this; the "
                "measured benefit is hardware-dependent and often within noise. Applied through "
                "bcdedit on the running OS entry.");
            t.iconName = QStringLiteral("clock-cog");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "boot-loader (BCD) change; benefit is unproven and hardware-dependent. Revert "
                "restores your prior disabledynamictick / useplatformclock state exactly.");
            t.dynamicOps = { bcdOp(t.id, {
                { QStringLiteral("disabledynamictick"), QStringLiteral("yes") },
                { QStringLiteral("useplatformclock"),   std::nullopt },   // remove if present
            }) };
            c.push_back(t);
        }

        // BCD: enhanced TSC synchronization policy
        {
            TweakSpec t;
            t.id = QStringLiteral("advanced.boot-tsc-sync-enhanced");
            t.category = QStringLiteral("advanced");
            t.subCategory = QStringLiteral("Boot / BCD");
            t.name = QStringLiteral("Enhanced TSC sync policy (BCD)");
            t.description = QStringLiteral(
                "Sets the boot-loader element tscsyncpolicy=Enhanced, changing how Windows "
                "synchronizes the CPU timestamp counter across cores at boot. This is an "
                "advanced multi-socket / multi-die timing knob; on a typical single-socket "
                "desktop it has no measurable effect. Applied through bcdedit.");
            t.iconName = QStringLiteral("clock-cog");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "boot-loader (BCD) change with no measured benefit on typical single-socket "
                "systems. Revert restores your prior tscsyncpolicy state exactly. The harmful "
                "BCD settings from the source (nx=AlwaysOff, disableelamdrivers, "
                "hypervisorlaunchtype=off, x2apicpolicy=Disable) are deliberately not shipped.");
            t.dynamicOps = { bcdOp(t.id, {
                { QStringLiteral("tscsyncpolicy"), QStringLiteral("Enhanced") },
            }) };
            c.push_back(t);
        }

        // BCD: x2APIC interrupt controller policy
        {
            TweakSpec t;
            t.id = QStringLiteral("advanced.boot-x2apic-disable");
            t.category = QStringLiteral("advanced");
            t.subCategory = QStringLiteral("Boot / BCD");
            t.name = QStringLiteral("Disable x2APIC (BCD)");
            t.description = QStringLiteral(
                "Sets x2apicpolicy=Disable, forcing the legacy xAPIC interrupt controller mode "
                "instead of x2APIC. Some latency guides recommend this; on modern many-core CPUs "
                "x2APIC is the mode the platform is designed around, so disabling it can raise "
                "interrupt overhead rather than lower it. Applied through bcdedit.");
            t.iconName = QStringLiteral("cpu");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("On systems with many logical processors, forcing legacy xAPIC can "
                            "reduce interrupt-routing efficiency."),
            };
            t.concern = QStringLiteral(
                "no measured benefit on typical consumer hardware and plausibly harmful on "
                "high-core-count CPUs. Revert restores your prior x2apicpolicy state exactly.");
            t.dynamicOps = { bcdOp(t.id, {
                { QStringLiteral("x2apicpolicy"), QStringLiteral("Disable") },
            }) };
            c.push_back(t);
        }

        // BCD: hardware DEP / NX policy
        {
            TweakSpec t;
            t.id = QStringLiteral("advanced.boot-nx-always-off");
            t.category = QStringLiteral("advanced");
            t.subCategory = QStringLiteral("Boot / BCD");
            t.name = QStringLiteral("Disable DEP / NX (BCD)");
            t.description = QStringLiteral(
                "Sets nx=AlwaysOff, turning off hardware Data Execution Prevention for the whole "
                "system. DEP is a core exploit mitigation: it stops data pages from being executed "
                "as code, which is the mechanism a large class of memory-corruption exploits "
                "depends on. Turning it off removes that protection system-wide for no measurable "
                "performance gain on modern CPUs, where NX enforcement is free in hardware.");
            t.iconName = QStringLiteral("shield-off");
            t.impact = Impact::Critical;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Disables Data Execution Prevention system-wide - a significant, "
                            "permanent reduction in exploit protection until reverted."),
                QStringLiteral("Some anti-cheat and security software refuses to run, or flags the "
                            "machine, when DEP is off."),
            };
            t.concern = QStringLiteral(
                "shipped because it is part of the requested BcdStoreTweaks port, but it is a "
                "security regression with no measured performance benefit - marked Harmful so it "
                "is never surfaced as an optimization or included in a recommended sweep.");
            t.dynamicOps = { bcdOp(t.id, {
                { QStringLiteral("nx"), QStringLiteral("AlwaysOff") },
            }) };
            c.push_back(t);
        }

        // BCD: hypervisor launch type (VBS / Hyper-V)
        {
            TweakSpec t;
            t.id = QStringLiteral("advanced.boot-hypervisor-off");
            t.category = QStringLiteral("advanced");
            t.subCategory = QStringLiteral("Boot / BCD");
            t.name = QStringLiteral("Disable hypervisor at boot (BCD)");
            t.description = QStringLiteral(
                "Sets hypervisorlaunchtype=Off so the Hyper-V hypervisor does not start. This is "
                "the boot-loader half of turning off Virtualization-Based Security; on machines "
                "where VBS/HVCI is active it typically recovers a few percent of CPU performance "
                "in games. It also switches off everything built on the hypervisor.");
            t.iconName = QStringLiteral("shield-off");
            t.impact = Impact::High;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Breaks WSL2, Windows Sandbox, Hyper-V virtual machines, Docker "
                            "Desktop's WSL/Hyper-V backends and Android subsystem emulation."),
                QStringLiteral("Disables Credential Guard and HVCI/memory integrity, reducing "
                            "protection against credential theft and driver-level attacks."),
                QStringLiteral("Some anti-cheat systems require virtualization-based security and "
                            "will refuse to launch with the hypervisor off."),
            };
            t.concern = QStringLiteral(
                "real performance trade-off rather than a placebo, but it is a security downgrade "
                "and breaks virtualization features. Pairs with the registry-side VBS tweak.");
            t.dynamicOps = { bcdOp(t.id, {
                { QStringLiteral("hypervisorlaunchtype"), QStringLiteral("Off") },
            }) };
            c.push_back(t);
        }

        // BCD: Early Launch Anti-Malware
        {
            TweakSpec t;
            t.id = QStringLiteral("advanced.boot-disable-elam");
            t.category = QStringLiteral("advanced");
            t.subCategory = QStringLiteral("Boot / BCD");
            t.name = QStringLiteral("Disable ELAM drivers (BCD)");
            t.description = QStringLiteral(
                "Sets disableelamdrivers=yes, stopping Early Launch Anti-Malware drivers from "
                "loading during boot. ELAM lets security software evaluate boot drivers before "
                "they initialize; skipping it shaves a small amount off boot time and removes "
                "that inspection step.");
            t.iconName = QStringLiteral("shield-off");
            t.impact = Impact::High;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Boot-start drivers are no longer screened by anti-malware before "
                            "they load - the exact window rootkits target."),
                QStringLiteral("Third-party security suites that ship an ELAM driver may report "
                            "themselves as degraded or partially disabled."),
            };
            t.concern = QStringLiteral(
                "security regression for a marginal boot-time gain. Revert restores your prior "
                "disableelamdrivers state exactly.");
            t.dynamicOps = { bcdOp(t.id, {
                { QStringLiteral("disableelamdrivers"), QStringLiteral("Yes") },
            }) };
            c.push_back(t);
        }

        // BCD: force the platform tick source
        {
            TweakSpec t;
            t.id = QStringLiteral("advanced.boot-platform-tick");
            t.category = QStringLiteral("advanced");
            t.subCategory = QStringLiteral("Boot / BCD");
            t.name = QStringLiteral("Force platform tick (BCD)");
            t.description = QStringLiteral(
                "Sets useplatformtick=yes, forcing the kernel to take its periodic tick from the "
                "platform timer rather than a per-CPU source. Usually paired with disabling the "
                "dynamic tick in latency setups. Effect varies by chipset and is often within noise.");
            t.iconName = QStringLiteral("clock-cog");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "undocumented in practical terms and hardware-dependent; can raise timer overhead "
                "on systems whose platform timer is slow to read.");
            t.dynamicOps = { bcdOp(t.id, {
                { QStringLiteral("useplatformtick"), QStringLiteral("Yes") },
            }) };
            c.push_back(t);
        }

        // BCD: boot appearance / menu policy
        {
            TweakSpec t;
            t.id = QStringLiteral("advanced.boot-quiet-legacy-menu");
            t.category = QStringLiteral("advanced");
            t.subCategory = QStringLiteral("Boot / BCD");
            t.name = QStringLiteral("Quiet boot and legacy boot menu (BCD)");
            t.description = QStringLiteral(
                "Sets quietboot=yes and bootux=Disabled to suppress the boot logo and animation, "
                "and bootmenupolicy=Legacy to use the text boot menu instead of the graphical one. "
                "Cosmetic and boot-time only; no effect once Windows is running.");
            t.iconName = QStringLiteral("player-play");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "the legacy boot menu changes how you reach recovery options - F8 at boot rather "
                "than the graphical Advanced Startup screen.");
            t.dynamicOps = { bcdOp(t.id, {
                { QStringLiteral("quietboot"),       QStringLiteral("Yes") },
                { QStringLiteral("bootux"),          QStringLiteral("Disabled") },
                { QStringLiteral("bootmenupolicy"),  QStringLiteral("Legacy") },
            }) };
            c.push_back(t);
        }

        // BCD: verbose boot driver listing
        {
            TweakSpec t;
            t.id = QStringLiteral("advanced.boot-verbose-sos");
            t.category = QStringLiteral("advanced");
            t.subCategory = QStringLiteral("Boot / BCD");
            t.name = QStringLiteral("Verbose boot driver list (BCD)");
            t.description = QStringLiteral(
                "Sets sos=yes so Windows lists driver names as they load during boot. A "
                "troubleshooting aid for diagnosing a boot that stalls on a specific driver - it "
                "does not speed anything up.");
            t.iconName = QStringLiteral("list");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "diagnostic only, and visually contradicts the quiet-boot tweak if both are on.");
            t.dynamicOps = { bcdOp(t.id, {
                { QStringLiteral("sos"), QStringLiteral("Yes") },
            }) };
            c.push_back(t);
        }

        // BCD: ensure every logical processor is used at boot
        {
            TweakSpec t;
            t.id = QStringLiteral("advanced.boot-all-cpus");
            t.category = QStringLiteral("advanced");
            t.subCategory = QStringLiteral("Boot / BCD");
            t.name = QStringLiteral("Use all processors at boot (BCD)");
            t.description = QStringLiteral(
                "Sets onecpu=no, ensuring Windows boots with every logical processor available. "
                "This is already the default, so it is a repair rather than an optimization - it "
                "matters when a previous tool, a diagnostic session or msconfig left the boot "
                "entry pinned to a single core.");
            t.iconName = QStringLiteral("cpu");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "no effect on a healthy system, where onecpu is already unset.");
            t.dynamicOps = { bcdOp(t.id, {
                { QStringLiteral("onecpu"), QStringLiteral("No") },
            }) };
            c.push_back(t);
        }

        // global NDIS offload settings (RSC / RSS / packet coalescing) via WMI
        {
            TweakSpec t;
            t.id = QStringLiteral("net.offload-global-disable");
            t.category = QStringLiteral("networking");
            t.subCategory = QStringLiteral("Offloads");
            t.name = QStringLiteral("Disable global network offloads");
            t.description = QStringLiteral(
                "Turns off the machine-wide NDIS offload settings - packet coalescing filter, "
                "Receive-Side Scaling and Receive Segment Coalescing - via "
                "Set-NetOffloadGlobalSetting. Trades throughput and CPU efficiency for slightly "
                "more predictable per-packet latency.");
            t.iconName = QStringLiteral("network");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.warnings = {
                QStringLiteral("Disabling RSS confines receive processing to one CPU, which lowers "
                            "maximum throughput on fast links."),
            };
            t.concern = QStringLiteral(
                "directly opposes the per-adapter RSS tweak, which enables RSS - the two are "
                "declared mutually exclusive. Evidence for a latency win is mixed; on most "
                "consumer connections the offloads are not the bottleneck.");
            {
                DynamicOp op;
                op.label = t.id;
                const QString id = t.id;
                static const QString kRead = QStringLiteral(
                    "$g=Get-NetOffloadGlobalSetting;"
                    "\"$($g.PacketCoalescingFilter)|$($g.ReceiveSideScaling)|"
                    "$($g.ReceiveSegmentCoalescing)\"");
                op.detect = []() {
                    QString out;
                    if (!dev::runPowershell(kRead, &out, 30000))
                        return DetectState::Unknown;
                    const QStringList p = out.trimmed().split(QLatin1Char('|'));
                    if (p.size() < 3) return DetectState::Unknown;
                    int off = 0;
                    for (const QString& s : p)
                        if (s.trimmed().compare(QStringLiteral("Disabled"), Qt::CaseInsensitive) == 0)
                            ++off;
                    if (off == 3) return DetectState::On;
                    return DetectState::Off;
                };
                op.apply = [id]() {
                    QString out;
                    if (dev::runPowershell(kRead, &out, 30000)) {
                        const QStringList p = out.trimmed().split(QLatin1Char('|'));
                        if (p.size() >= 3) {
                            QMap<QString, QString> snap;
                            snap.insert(QStringLiteral("PacketCoalescingFilter"),   p.at(0).trimmed());
                            snap.insert(QStringLiteral("ReceiveSideScaling"),       p.at(1).trimmed());
                            snap.insert(QStringLiteral("ReceiveSegmentCoalescing"), p.at(2).trimmed());
                            dev::saveSnapshot(id, snap);
                        }
                    }
                    return dev::runPowershell(QStringLiteral(
                        "Set-NetOffloadGlobalSetting -PacketCoalescingFilter Disabled "
                        "-ReceiveSideScaling Disabled -ReceiveSegmentCoalescing Disabled"),
                        nullptr, 30000);
                };
                op.revert = [id]() {
                    const QMap<QString, QString> snap = dev::loadSnapshot(id);
                    const QString pcf = offloadStateOrDefault(
                        snap.value(QStringLiteral("PacketCoalescingFilter")));
                    const QString rss = offloadStateOrDefault(
                        snap.value(QStringLiteral("ReceiveSideScaling")));
                    const QString rsc = offloadStateOrDefault(
                        snap.value(QStringLiteral("ReceiveSegmentCoalescing")));
                    const bool ok = dev::runPowershell(QStringLiteral(
                        "Set-NetOffloadGlobalSetting -PacketCoalescingFilter %1 "
                        "-ReceiveSideScaling %2 -ReceiveSegmentCoalescing %3")
                        .arg(pcf, rss, rsc), nullptr, 30000);
                    dev::clearSnapshot(id);
                    return ok;
                };
                t.dynamicOps = { op };
            }
            c.push_back(t);
        }

        // per-NIC checksum / segmentation offloads + interrupt moderation
        {
            TweakSpec t;
            t.id = QStringLiteral("net.per-nic-offload-tuning");
            t.category = QStringLiteral("networking");
            t.subCategory = QStringLiteral("Offloads");
            t.name = QStringLiteral("Per-adapter offload settings");
            t.description = QStringLiteral(
                "On every physical adapter: turns off large-send offload (LSOv2 v4/v6), IPv4 and "
                "UDP checksum offload, interrupt moderation and the ARP/NS power-management "
                "offloads, while keeping TCP checksum offload on and enabling priority/VLAN "
                "tagging. Moving checksum and segmentation work back to the CPU removes a small "
                "amount of buffering at the cost of extra CPU per packet.");
            t.iconName = QStringLiteral("network");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Raises CPU use per packet; on slow CPUs with fast links this can "
                            "reduce throughput rather than improve latency."),
            };
            t.concern = QStringLiteral(
                "disables interrupt moderation, which the existing per-adapter defaults "
                "deliberately leave ON - measured results for turning it off are mixed and "
                "hardware-dependent. Values follow the source verbatim.");
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                for (const auto& nic : dev::enumeratePhysicalNics()) {
                    const QString k = nic.classKeyPath;
                    v << stringT(k, QStringLiteral("*LsoV2IPv4"), QStringLiteral("0"))
                    << stringT(k, QStringLiteral("*LsoV2IPv6"), QStringLiteral("0"))
                    << stringT(k, QStringLiteral("*IPChecksumOffloadIPv4"), QStringLiteral("0"))
                    << stringT(k, QStringLiteral("*UDPChecksumOffloadIPv4"), QStringLiteral("0"))
                    << stringT(k, QStringLiteral("*UDPChecksumOffloadIPv6"), QStringLiteral("0"))
                    << stringT(k, QStringLiteral("*TCPChecksumOffloadIPv4"), QStringLiteral("1"))
                    << stringT(k, QStringLiteral("*TCPChecksumOffloadIPv6"), QStringLiteral("1"))
                    << stringT(k, QStringLiteral("*InterruptModeration"), QStringLiteral("0"))
                    << stringT(k, QStringLiteral("*PriorityVLANTag"), QStringLiteral("1"))
                    << stringT(k, QStringLiteral("*PMARPOffload"), QStringLiteral("0"))
                    << stringT(k, QStringLiteral("*PMNSOffload"), QStringLiteral("0"));
                }
                return v;
            }) };
            c.push_back(t);
        }

        // per-NIC receive/transmit descriptor buffers
        {
            TweakSpec t;
            t.id = QStringLiteral("net.per-nic-buffers");
            t.category = QStringLiteral("networking");
            t.subCategory = QStringLiteral("Adapter");
            t.name = QStringLiteral("Per-adapter Receive/Transmit buffers");
            t.description = QStringLiteral(
                "Sets the receive descriptor count to 512 and transmit to 128 on every physical "
                "adapter. Larger receive buffers absorb bursts without dropping packets; smaller "
                "transmit buffers slightly reduce how long an outbound packet can sit queued. "
                "Adapters clamp these to whatever their driver actually supports.");
            t.iconName = QStringLiteral("network");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "the 512/128 pair is the source's fixed choice, not a measured optimum; sensible "
                "values differ per driver and link speed.");
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                for (const auto& nic : dev::enumeratePhysicalNics()) {
                    const QString k = nic.classKeyPath;
                    v << stringT(k, QStringLiteral("*ReceiveBuffers"), QStringLiteral("512"))
                    << stringT(k, QStringLiteral("*TransmitBuffers"), QStringLiteral("128"));
                }
                return v;
            }) };
            c.push_back(t);
        }

        // per-NIC wake-on-LAN off
        {
            TweakSpec t;
            t.id = QStringLiteral("net.per-nic-wake-off");
            t.category = QStringLiteral("networking");
            t.subCategory = QStringLiteral("Power saving");
            t.name = QStringLiteral("Disable wake-on-LAN (per adapter)");
            t.description = QStringLiteral(
                "Turns off every wake source on each physical adapter - magic packet, pattern "
                "match, link change, modern-standby magic packet and S5 wake - and clears the "
                "adapter's PnP wake capabilities. Stops the machine being woken by network "
                "traffic and removes the wake-arming work the driver does at sleep.");
            t.iconName = QStringLiteral("plug-zap");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Wake-on-LAN stops working; a machine you wake remotely will no "
                            "longer respond to a magic packet."),
            };
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                for (const auto& nic : dev::enumeratePhysicalNics()) {
                    const QString k = nic.classKeyPath;
                    v << stringT(k, QStringLiteral("*WakeOnMagicPacket"), QStringLiteral("0"))
                    << stringT(k, QStringLiteral("*WakeOnPattern"), QStringLiteral("0"))
                    << stringT(k, QStringLiteral("*ModernStandbyWoLMagicPacket"), QStringLiteral("0"))
                    << stringT(k, QStringLiteral("WakeOnLink"), QStringLiteral("0"))
                    << stringT(k, QStringLiteral("WakeOnLinkChange"), QStringLiteral("0"))
                    << stringT(k, QStringLiteral("S5WakeOnLan"), QStringLiteral("0"))
                    << stringT(k, QStringLiteral("WolShutdownLinkSpeed"), QStringLiteral("2"))
                    << dwordT(k, QStringLiteral("PnPCapabilities"), 24);
                }
                return v;
            }) };
            c.push_back(t);
        }

        // per-NIC extended power-saving values
        {
            TweakSpec t;
            t.id = QStringLiteral("net.per-nic-powersaving-extended");
            t.category = QStringLiteral("networking");
            t.subCategory = QStringLiteral("Power saving");
            t.name = QStringLiteral("Extended NIC power saving off (per adapter)");
            t.description = QStringLiteral(
                "Clears the remaining vendor power-management properties the main power-saving "
                "tweak does not cover: general power management, dynamic and connected power "
                "gating, ultra-low-power mode, speed reduction on power-down, GigaLite and the "
                "generic power-saving mode. Vendor-specific - adapters ignore the properties they "
                "do not implement.");
            t.iconName = QStringLiteral("plug-zap");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "raises idle power draw slightly; on laptops this shortens battery life.");
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                for (const auto& nic : dev::enumeratePhysicalNics()) {
                    const QString k = nic.classKeyPath;
                    v << stringT(k, QStringLiteral("EnablePowerManagement"), QStringLiteral("0"))
                    << stringT(k, QStringLiteral("EnableDynamicPowerGating"), QStringLiteral("0"))
                    << stringT(k, QStringLiteral("EnableConnectedPowerGating"), QStringLiteral("0"))
                    << stringT(k, QStringLiteral("ULPMode"), QStringLiteral("0"))
                    << stringT(k, QStringLiteral("ReduceSpeedOnPowerDown"), QStringLiteral("0"))
                    << stringT(k, QStringLiteral("PowerSavingMode"), QStringLiteral("0"))
                    << stringT(k, QStringLiteral("GigaLite"), QStringLiteral("0"));
                    // AdvancedEEE is left out: net.per-nic-powersaving-off already writes it as
                    // REG_SZ, the correct type. Realtek's INF registers it as an Ndi\Params enum
                    // (REG_SZ); only Type "dword" / flag 0x00010001 makes a REG_DWORD, as
                    // PnPCapabilities does.
                }
                return v;
            }) };
            c.push_back(t);
        }

        // per-NIC forced link parameters (jumbo frames + fixed speed/duplex)
        {
            TweakSpec t;
            t.id = QStringLiteral("net.per-nic-link-forced");
            t.category = QStringLiteral("networking");
            t.subCategory = QStringLiteral("Adapter");
            t.name = QStringLiteral("Force jumbo frames and gigabit full duplex (per adapter)");
            t.description = QStringLiteral(
                "Sets a 9014-byte jumbo frame MTU and pins the link to 1 Gbps full duplex on every "
                "physical adapter, rather than auto-negotiating. Only useful when every device on "
                "the path - switch, router and the machine at the other end - is configured for "
                "jumbo frames and the same fixed speed.");
            t.iconName = QStringLiteral("network");
            t.impact = Impact::High;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Jumbo frames break connectivity on any network whose switch or "
                            "router does not support a 9014-byte MTU - the usual case on home "
                            "networks and every internet path."),
                QStringLiteral("Forcing a fixed speed and duplex can drop the link entirely if the "
                            "switch port expects auto-negotiation, or on hardware that is not "
                            "gigabit-capable."),
            };
            t.concern = QStringLiteral(
                "the most likely tweak here to take a machine off the network; both values are "
                "environment-specific rather than general improvements. Kept separate from the "
                "other per-adapter tweaks so it is never applied incidentally.");
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                for (const auto& nic : dev::enumeratePhysicalNics()) {
                    const QString k = nic.classKeyPath;
                    v << stringT(k, QStringLiteral("*JumboPacket"), QStringLiteral("9014"))
                    << stringT(k, QStringLiteral("*SpeedDuplex"), QStringLiteral("6"));
                }
                return v;
            }) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("net.per-nic-interrupt-moderation");
            t.category = QStringLiteral("networking");
            t.name = QStringLiteral("Interrupt moderation (per adapter)");
            t.description = QStringLiteral(
                "Ensures interrupt moderation is ON for every physical adapter.");
            t.iconName = QStringLiteral("network");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "operator-corrected: keeps moderation ON. The popular \"turn it Off\" tweak "
                "generates ~30x more interrupts and starves audio/USB DPCs; "
                "Medium/On is the right default.");
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                for (const auto& nic : dev::enumeratePhysicalNics())
                    v << stringT(nic.classKeyPath, QStringLiteral("*InterruptModeration"), QStringLiteral("1"));
                return v;
            }) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.intel-dedicated-segment");
            t.category = QStringLiteral("gpu");
            t.name = QStringLiteral("Intel: increase iGPU dedicated memory");
            t.description = QStringLiteral(
                "Reserves 512 MB of system memory for Intel graphics on every Intel "
                "display adapter in this PC, instead of sharing it on demand.");
            t.iconName = QStringLiteral("monitor");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral("Intel-iGPU-only; no effect on a discrete-GPU-only system.");
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                for (const QString& k : dev::enumerateGpuClassKeys(QStringLiteral("VEN_8086"))) {
                    v << dwordT(k, QStringLiteral("IncreaseFixedSegment"), 1)
                    << dwordT(k, QStringLiteral("DedicatedSegmentSize"), 512);
                }
                return v;
            }) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("mem.pool-usage-maximum");
            t.category = QStringLiteral("advanced");
            t.name = QStringLiteral("Cap paged-pool usage (PoolUsageMaximum)");
            t.description = QStringLiteral(
                "Makes Windows give memory back sooner when it starts to run short, "
                "rather than holding on to it until pressure is already high.");
            t.iconName = QStringLiteral("layers");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"),
                        QStringLiteral("PoolUsageMaximum"), 60, std::nullopt),
            };
            c.push_back(t);
        }
    }

}
