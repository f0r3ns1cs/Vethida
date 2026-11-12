#include "TweakEngine.h"
#include "DeviceOps.h"

#include <QtCore/QByteArray>
#include <QtCore/QRegularExpression>

#include <initializer_list>

namespace vth::tweaks::detail
{
    namespace dev = vth::tweaks::devices;

    namespace
    {
        // dword op, revert = restore the machine's real prior value (the ledger
        // captures it on apply); the delete here is only the no-capture fallback.
        void addD(QVector<RegistryOp>& ops, const QString& path, const QString& name, quint32 v) {
            ops.push_back(dwordOp(path, name, v, std::nullopt));
        }
        void addS(QVector<RegistryOp>& ops, const QString& path, const QString& name, const QString& v) {
            ops.push_back(stringOp(path, name, v, std::nullopt));
        }

        QByteArray bytes(std::initializer_list<int> b) {
            QByteArray a;
            a.reserve(static_cast<int>(b.size()));
            for (int x : b) a.append(static_cast<char>(x));
            return a;
        }
        // Binary op: REG_BINARY is not ledger-captured, so revert is the explicit
        // default bytes passed here.
        RegistryOp binOp(const QString& path, const QString& name,
                         const QByteArray& applied, const QByteArray& revert) {
            RegistryOp op;
            op.path = path;
            op.name = name;
            op.kind = vth::services::RegistryValueKind::Binary;
            op.appliedValue = applied;
            op.revertValue  = revert;
            return op;
        }
        CommandOp psOp(const QString& applyScript, const QString& revertScript) {
            CommandOp op;
            op.program = QStringLiteral("powershell");
            const QStringList base = {
                QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"),
                QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"),
                QStringLiteral("-Command"),
            };
            op.applyArgs = base;
            op.applyArgs << applyScript;
            op.revertArgs = base;
            op.revertArgs << revertScript;
            return op;
        }

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
            // Applied only when every target holds its value; a mix is not.
            return on == total ? DetectState::On : DetectState::Off;
        }
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
        // Per-adapter TCP/IP interface key prefix (append the adapter's instance
        // GUID). Used by the per-interface latency tweak below.
        const QString kTcpipIf = QStringLiteral(
            "HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\");

        // The engine refuses this write for protected services, so a group can name one.
        QVector<RegistryOp> svcOff(std::initializer_list<const char*> names) {
            QVector<RegistryOp> o;
            o.reserve(static_cast<int>(names.size()));
            for (const char* n : names)
                o.push_back(dwordOp(
                    QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\") + QLatin1String(n),
                    QStringLiteral("Start"), 4u, std::nullopt));
            return o;
        }

        // CPU idle-disable via powercfg, with an exact prior-index snapshot.
        struct BcdEdit {
            QString                element;
            std::optional<QString> applied;
        };
        DynamicOp bcdOp(const QString& id, const QVector<BcdEdit>& edits) {
            DynamicOp op;
            op.label  = id;
            op.detect = []() { return DetectState::Unknown; };
            op.apply  = [id, edits]() {
                QMap<QString, QString> snap;
                for (const BcdEdit& e : edits) {
                    const auto prior = dev::bcdReadElement(e.element);
                    snap.insert(e.element, prior ? QStringLiteral("v:") + *prior : QStringLiteral("-"));
                }
                dev::saveSnapshot(id, snap);
                bool ok = true;
                for (const BcdEdit& e : edits) {
                    if (e.applied.has_value())
                        ok = dev::bcdSetElement(e.element, *e.applied) && ok;
                    else if (snap.value(e.element) != QStringLiteral("-"))
                        ok = dev::bcdDeleteElement(e.element) && ok;
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
                        ok = dev::bcdDeleteElement(e.element) && ok;
                }
                dev::clearSnapshot(id);
                return ok;
            };
            return op;
        }
        DynamicOp powercfgIdleDisableOp(const QString& id) {
            static const QString kSub  = QStringLiteral("SUB_PROCESSOR");
            static const QString kName = QStringLiteral("IDLEDISABLE");
            static const QString kKey  = QStringLiteral("idledisable");
            DynamicOp op;
            op.label  = id;
            op.detect = []() -> DetectState {
                const auto v = dev::powercfgQueryAcIndex(kSub, kName);
                if (!v) return DetectState::Unknown;
                return *v == 1u ? DetectState::On : DetectState::Off;
            };
            // Recorded, because powercfg writes to scheme_current and the plan can change.
            static const QString kSchemeKey = QStringLiteral("scheme");
            op.apply = [id]() {
                const auto prior = dev::powercfgQueryAcIndex(kSub, kName);
                QMap<QString, QString> snap;
                snap.insert(kKey, prior ? QString::number(*prior) : QStringLiteral("-"));
                QString out;
                if (dev::runHidden(QStringLiteral("powercfg"),
                                   { QStringLiteral("/getactivescheme") }, &out, 15000)) {
                    static const QRegularExpression re(QStringLiteral(
                        "[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}"));
                    const auto m = re.match(out);
                    if (m.hasMatch())
                        snap.insert(kSchemeKey, m.captured(0));
                }
                dev::saveSnapshot(id, snap);
                return dev::powercfgSetAcIndex(kSub, kName, 1u);
            };
            op.revert = [id]() {
                const QMap<QString, QString> snap = dev::loadSnapshot(id);
                const QString v = snap.value(kKey, QStringLiteral("-"));
                const QString value = QString::number(v == QStringLiteral("-") ? 0u : v.toUInt());
                const QString scheme = snap.value(kSchemeKey);
                bool ok = true;
                if (!scheme.isEmpty()) {
                    ok = dev::runHidden(QStringLiteral("powercfg"),
                        { QStringLiteral("/setacvalueindex"), scheme, kSub, kName, value },
                        nullptr, 15000);
                }
                // The plan in use now as well: idle back on wherever the
                // machine is, not only where it was.
                ok = dev::powercfgSetAcIndex(kSub, kName, value.toUInt()) && ok;
                dev::clearSnapshot(id);
                return ok;
            };
            return op;
        }

        // Memory compression via MMAgent (WMI); prior on/off snapshotted.
        DynamicOp memoryCompressionOffOp(const QString& id) {
            static const QString kKey = QStringLiteral("memcompression");
            DynamicOp op;
            op.label  = id;
            op.detect = []() -> DetectState {
                const auto v = dev::memoryCompressionEnabled();
                if (!v) return DetectState::Unknown;
                return *v ? DetectState::Off : DetectState::On;   // applied == compression OFF
            };
            op.apply = [id]() {
                const auto prior = dev::memoryCompressionEnabled();
                QMap<QString, QString> snap;
                snap.insert(kKey, prior ? (*prior ? QStringLiteral("1") : QStringLiteral("0"))
                                        : QStringLiteral("-"));
                dev::saveSnapshot(id, snap);
                return dev::setMemoryCompression(false);
            };
            op.revert = [id]() {
                const QMap<QString, QString> snap = dev::loadSnapshot(id);
                const QString v = snap.value(kKey, QStringLiteral("1"));
                const bool ok = dev::setMemoryCompression(v != QStringLiteral("0"));
                dev::clearSnapshot(id);
                return ok;
            };
            return op;
        }

        // Display-adapter class GUID, for the message-signaled-interrupt sweep.
        const QString kDisplayClass = QStringLiteral("{4d36e968-e325-11ce-bfc1-08002be10318}");

    }

    void appendOperatorBatch(QVector<TweakSpec>& c) {
        // OneDrive.
        {
            TweakSpec t;
            t.id = QStringLiteral("debloat.remove-onedrive");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("OneDrive");
            t.name = QStringLiteral("Remove OneDrive");
            t.description = QStringLiteral(
                "Runs the OneDrive uninstaller and removes the leftover OneDrive icon "
                "from File Explorer's sidebar.");
            t.iconName = QStringLiteral("package-off");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.reversible = false;
            t.warnings = {
                QStringLiteral("Let anything unsynced finish syncing first."),
            };
            const QString apply = QStringLiteral(
                "$ErrorActionPreference='SilentlyContinue';"
                "$s=\"$env:SystemRoot\\SysWOW64\\OneDriveSetup.exe\";"
                "if(-not(Test-Path $s)){$s=\"$env:SystemRoot\\System32\\OneDriveSetup.exe\"}"
                "Start-Process $s '/uninstall' -Wait;"
                "Remove-Item 'HKCU:\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\"
                "Desktop\\NameSpace\\{018D5C66-4533-4307-9B53-224DE2ED1FE6}' -Force");
            const QString revert = QStringLiteral(
                "$ErrorActionPreference='SilentlyContinue';"
                "$s=\"$env:SystemRoot\\SysWOW64\\OneDriveSetup.exe\";"
                "if(-not(Test-Path $s)){$s=\"$env:SystemRoot\\System32\\OneDriveSetup.exe\"}"
                "Start-Process $s -Wait");
            t.commandOps = { psOp(apply, revert) };
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("debloat.onedrive-purge-leftovers");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("OneDrive");
            t.name = QStringLiteral("Purge OneDrive leftovers");
            t.description = QStringLiteral(
                "Deletes what the OneDrive uninstaller leaves behind: the personal "
                "OneDrive folder, its staging and app-data folders, and the remaining "
                "File Explorer sidebar entries. Run after \"Remove OneDrive\".");
            t.iconName = QStringLiteral("package-off");
            t.impact = Impact::High;
            t.requiresAdmin = true;
            t.reversible = false;
            t.warnings = {
                QStringLiteral("Deletes the local OneDrive folder and everything in it. Anything not synced to the cloud is lost."),
            };
            t.concern = QStringLiteral(
                "destructive: deletes the OneDrive folders under the real per-user paths.");
            const QString apply = QStringLiteral(
                "$ErrorActionPreference='SilentlyContinue';"
                "Stop-Process -Name OneDrive -Force;"
                "foreach($d in \"$env:USERPROFILE\\OneDrive\",'C:\\OneDriveTemp',"
                "\"$env:LOCALAPPDATA\\Microsoft\\OneDrive\","
                "\"$env:PROGRAMDATA\\Microsoft OneDrive\"){Remove-Item $d -Recurse -Force}"
                "foreach($k in 'HKCU:\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\"
                "Desktop\\NameSpace\\{018D5C66-4533-4307-9B53-224DE2ED1FE6}',"
                "'HKCU:\\SOFTWARE\\Classes\\CLSID\\{018D5C66-4533-4307-9B53-224DE2ED1FE6}',"
                "'HKCU:\\SOFTWARE\\Classes\\Wow6432Node\\CLSID\\{018D5C66-4533-4307-9B53-224DE2ED1FE6}',"
                "'HKLM:\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\"
                "Desktop\\NameSpace\\{018D5C66-4533-4307-9B53-224DE2ED1FE6}'"
                "){Remove-Item $k -Recurse -Force}");
            const QString revert = QStringLiteral(
                "$ErrorActionPreference='SilentlyContinue';"
                "Write-Output 'OneDrive leftovers were deleted; nothing to restore.'");
            t.commandOps = { psOp(apply, revert) };
            c.push_back(t);
        }

        // Privacy.
        {
            TweakSpec t;
            t.id = QStringLiteral("op.privacy.location-sensors");
            t.category = QStringLiteral("privacy");
            t.subCategory = QStringLiteral("Location");
            t.name = QStringLiteral("Turn off location and sensors");
            t.description = QStringLiteral(
                "Turns off Windows location and sensor access system-wide and stops "
                "Find My Device from tracking where this PC is.");
            t.iconName = QStringLiteral("world");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            QVector<RegistryOp> o;
            const QString las = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\LocationAndSensors");
            addD(o, las, QStringLiteral("DisableLocation"), 1);
            addD(o, las, QStringLiteral("DisableLocationScripting"), 1);
            addD(o, las, QStringLiteral("DisableSensors"), 1);
            addD(o, las, QStringLiteral("DisableWindowsLocationProvider"), 1);
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\System\\AllowLocation"), QStringLiteral("value"), 0);
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\current\\device\\System"), QStringLiteral("AllowLocation"), 0);
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\FindMyDevice"), QStringLiteral("AllowFindMyDevice"), 0);
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Settings\\FindMyDevice"), QStringLiteral("LocationSyncEnabled"), 0);
            addS(o, QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\location"), QStringLiteral("Value"), QStringLiteral("Deny"));
            t.registryOps = o;
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.privacy.telemetry-feedback");
            t.category = QStringLiteral("privacy");
            t.subCategory = QStringLiteral("Telemetry");
            t.name = QStringLiteral("Stop feedback and error reporting");
            t.description = QStringLiteral(
                "Turns off the customer-experience program, feedback prompts, tailored "
                "experiences, online tips, and Windows error reporting.");
            t.iconName = QStringLiteral("eye-off");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            QVector<RegistryOp> o;
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\SQMClient\\Windows"), QStringLiteral("CEIPEnable"), 0);
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\SQMClient\\Windows"), QStringLiteral("DisableOptinExperience"), 1);
            const QString assist = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Assistance\\Client\\1.0");
            addD(o, assist, QStringLiteral("NoExplicitFeedback"), 1);
            addD(o, assist, QStringLiteral("NoActiveHelp"), 1);
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\PCHealth\\ErrorReporting"), QStringLiteral("DoReport"), 0);
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Error Reporting"), QStringLiteral("Disabled"), 1);
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\Windows Error Reporting\\Escalation"), QStringLiteral("DisableEscalation"), 1);
            addD(o, QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\Windows Error Reporting"), QStringLiteral("DontSendAdditionalData"), 1);
            addD(o, QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Siuf\\Rules"), QStringLiteral("NumberOfSIUFInPeriod"), 0);
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Diagnostics\\DiagTrack"), QStringLiteral("ShowedToastAtLevel"), 1);
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Diagnostics\\DiagTrack\\EventTranscriptKey"), QStringLiteral("EnableEventTranscript"), 0);
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\Settings\\AllowOnlineTips"), QStringLiteral("value"), 0);
            addD(o, QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Privacy"), QStringLiteral("TailoredExperiencesWithDiagnosticDataEnabled"), 0);
            t.registryOps = o;
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.privacy.typing-speech-inking");
            t.category = QStringLiteral("privacy");
            t.subCategory = QStringLiteral("Typing and speech");
            t.name = QStringLiteral("Stop typing, inking and speech collection");
            t.description = QStringLiteral(
                "Stops Windows from collecting your typing and handwriting samples, "
                "turns off online speech recognition and voice activation, and opts out "
                "of the language-list web signal.");
            t.iconName = QStringLiteral("keyboard");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            QVector<RegistryOp> o;
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\InputPersonalization"), QStringLiteral("AllowInputPersonalization"), 0);
            const QString ip = QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\InputPersonalization");
            addD(o, ip, QStringLiteral("RestrictImplicitInkCollection"), 1);
            addD(o, ip, QStringLiteral("RestrictImplicitTextCollection"), 1);
            addD(o, QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\InputPersonalization\\TrainedDataStore"), QStringLiteral("HarvestContacts"), 0);
            addD(o, QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Personalization\\Settings"), QStringLiteral("AcceptedPrivacyPolicy"), 0);
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Speech"), QStringLiteral("AllowSpeechModelUpdate"), 0);
            addD(o, QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Speech_OneCore\\Settings\\OnlineSpeechPrivacy"), QStringLiteral("HasAccepted"), 0);
            const QString va = QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Speech_OneCore\\Settings\\VoiceActivation\\UserPreferenceForAllApps");
            addD(o, va, QStringLiteral("AgentActivationEnabled"), 0);
            addD(o, va, QStringLiteral("AgentActivationOnLockScreenEnabled"), 0);
            addD(o, va, QStringLiteral("AgentActivationLastUsed"), 0);
            addD(o, QStringLiteral("HKCU\\Control Panel\\International\\User Profile"), QStringLiteral("HttpAcceptLanguageOptOut"), 1);
            t.registryOps = o;
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.privacy.app-access");
            t.category = QStringLiteral("privacy");
            t.subCategory = QStringLiteral("App permissions");
            t.name = QStringLiteral("Deny background app data access");
            t.description = QStringLiteral(
                "Denies apps access to your notifications, account info and app-diagnostics, "
                "and turns off background activity for Store apps and their search access.");
            t.iconName = QStringLiteral("shield-off");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            QVector<RegistryOp> o;
            const QString cs = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\");
            addS(o, cs + QStringLiteral("userNotificationListener"), QStringLiteral("Value"), QStringLiteral("Deny"));
            addS(o, cs + QStringLiteral("appDiagnostics"), QStringLiteral("Value"), QStringLiteral("Deny"));
            addS(o, cs + QStringLiteral("userAccountInformation"), QStringLiteral("Value"), QStringLiteral("Deny"));
            addD(o, QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\BackgroundAccessApplications"), QStringLiteral("GlobalUserDisabled"), 1);
            addD(o, QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Search"), QStringLiteral("GlobalUserDisabled"), 1);
            t.registryOps = o;
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.privacy.advertising-id");
            t.category = QStringLiteral("privacy");
            t.subCategory = QStringLiteral("Advertising");
            t.name = QStringLiteral("Turn off the advertising ID");
            t.description = QStringLiteral(
                "Turns off the per-user advertising ID that apps use to tailor ads to you.");
            t.iconName = QStringLiteral("tag");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            QVector<RegistryOp> o;
            addD(o, QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AdvertisingInfo"), QStringLiteral("Enabled"), 0);
            addD(o, QStringLiteral("HKCU\\SOFTWARE\\Policies\\Microsoft\\Windows\\AdvertisingInfo"), QStringLiteral("DisabledByGroupPolicy"), 1);
            t.registryOps = o;
            c.push_back(t);
        }

        // Debloat and content.
        {
            TweakSpec t;
            t.id = QStringLiteral("op.debloat.suggested-content");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Suggestions");
            t.name = QStringLiteral("Turn off suggested content and Spotlight");
            t.description = QStringLiteral(
                "Turns off consumer suggestions, Spotlight, tips and the promotional "
                "content Windows drops onto the lock screen, Start and Settings.");
            t.iconName = QStringLiteral("sparkles");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            QVector<RegistryOp> o;
            const QString cc = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\CloudContent");
            addD(o, cc, QStringLiteral("DisableSoftLanding"), 1);
            addD(o, cc, QStringLiteral("DisableWindowsConsumerFeatures"), 1);
            addD(o, cc, QStringLiteral("DisableWindowsSpotlightFeatures"), 1);
            addD(o, cc, QStringLiteral("DisableConsumerAccountStateContent"), 1);
            addD(o, cc, QStringLiteral("DisableCloudOptimizedContent"), 1);
            const QString cdm = QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager");
            for (const char* n : { "338393", "338394", "338396", "353694", "353696",
                                   "310093", "338389", "338387", "338388", "353698" })
                addD(o, cdm, QStringLiteral("SubscribedContent-%1Enabled").arg(QLatin1String(n)), 0);
            addD(o, cdm, QStringLiteral("RotatingLockScreenEnabled"), 0);
            addD(o, cdm, QStringLiteral("RotatingLockScreenOverlayEnabled"), 0);
            addD(o, cdm, QStringLiteral("SoftLandingEnabled"), 0);
            addD(o, cdm, QStringLiteral("DisableTailoredExperiencesWithDiagnosticData"), 1);
            addD(o, cdm, QStringLiteral("DisableWindowsConsumerFeatures"), 1);
            addD(o, QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\UserProfileEngagement"), QStringLiteral("ScoobeSystemSettingEnabled"), 0);
            t.registryOps = o;
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.debloat.news-feeds");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("News and interests");
            t.name = QStringLiteral("Turn off news and interests");
            t.description = QStringLiteral(
                "Turns off the taskbar news-and-interests feed and its widgets.");
            t.iconName = QStringLiteral("rss");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            QVector<RegistryOp> o;
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Dsh"), QStringLiteral("AllowNewsAndInterests"), 0);
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Feeds"), QStringLiteral("EnableFeeds"), 0);
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"), QStringLiteral("EnableFeeds"), 0);
            const QString feeds = QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Feeds");
            addD(o, feeds, QStringLiteral("ShellFeedsTaskbarViewMode"), 2);
            addD(o, feeds, QStringLiteral("IsFeedsAvailable"), 0);
            addD(o, feeds, QStringLiteral("ShellFeedsTaskbarOpenOnHover"), 0);
            t.registryOps = o;
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.debloat.copilot-cortana");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Copilot and Cortana");
            t.name = QStringLiteral("Turn off Copilot and Cortana");
            t.description = QStringLiteral(
                "Turns off Windows Copilot and Cortana and stops Start-menu search from "
                "reaching out to the web.");
            t.iconName = QStringLiteral("bot");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            QVector<RegistryOp> o;
            addD(o, QStringLiteral("HKCU\\SOFTWARE\\Policies\\Microsoft\\Windows"), QStringLiteral("TurnOffWindowsCopilot"), 1);
            const QString ws = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search");
            addD(o, ws, QStringLiteral("AllowCortana"), 0);
            addD(o, ws, QStringLiteral("ConnectedSearchUseWeb"), 0);
            addD(o, ws, QStringLiteral("PreventUsingAdvancedIndexingOptions"), 0);
            addD(o, QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Windows Search"), QStringLiteral("CortanaConsent"), 0);
            const QString ss = QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SearchSettings");
            addD(o, ss, QStringLiteral("SafeSearchMode"), 0);
            addD(o, ss, QStringLiteral("IsDeviceSearchHistoryEnabled"), 0);
            t.registryOps = o;
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.debloat.sync-activity");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Sync and activity");
            t.name = QStringLiteral("Turn off settings sync and activity history");
            t.description = QStringLiteral(
                "Turns off syncing your settings, passwords and clipboard between devices, "
                "and stops Windows recording your activity and timeline.");
            t.iconName = QStringLiteral("refresh-cw");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            QVector<RegistryOp> o;
            addD(o, QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SettingSync"), QStringLiteral("SyncPolicy"), 5);
            const QString sg = QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SettingSync\\Groups\\");
            for (const char* g : { "Personalization", "BrowserSettings", "Credentials",
                                   "Accessibility", "Windows", "Language" })
                addD(o, sg + QLatin1String(g), QStringLiteral("Enabled"), 0);
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Messaging"), QStringLiteral("AllowMessageSync"), 0);
            const QString clip = QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Clipboard");
            addD(o, clip, QStringLiteral("AllowCrossDeviceClipboard"), 0);
            addD(o, clip, QStringLiteral("EnableClipboardHistory"), 0);
            const QString sys = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System");
            addD(o, sys, QStringLiteral("AllowClipboardHistory"), 0);
            addD(o, sys, QStringLiteral("EnableCdp"), 0);
            addD(o, sys, QStringLiteral("PublishUserActivities"), 0);
            addD(o, sys, QStringLiteral("UploadUserActivities"), 0);
            addD(o, sys, QStringLiteral("EnableActivityFeed"), 0);
            t.registryOps = o;
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.debloat.game-dvr-capture");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Game DVR");
            t.name = QStringLiteral("Turn off Game DVR and background capture");
            t.description = QStringLiteral(
                "Turns off the background game recorder and its notifications, and lets "
                "full-screen games skip the capture path.");
            t.iconName = QStringLiteral("gamepad-2");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            QVector<RegistryOp> o;
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\ApplicationManagement\\AllowGameDVR"), QStringLiteral("value"), 0);
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\GameDVR"), QStringLiteral("AllowGameDVR"), 0);
            const QString gcs = QStringLiteral("HKCU\\SYSTEM\\GameConfigStore");
            addD(o, gcs, QStringLiteral("GameDVR_Enabled"), 0);
            addD(o, gcs, QStringLiteral("GameDVR_FSEBehaviorMode"), 2);
            addD(o, gcs, QStringLiteral("GameDVR_HonorUserFSEBehaviorMode"), 0);
            addD(o, gcs, QStringLiteral("GameDVR_EFSEFeatureFlags"), 0);
            addD(o, gcs, QStringLiteral("GameDVR_DXGIHonorFSEWindowsCompatible"), 1);
            addD(o, QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\GameDVR"), QStringLiteral("AppCaptureEnabled"), 0);
            const QString gb = QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\GameBar");
            addD(o, gb, QStringLiteral("AutoGameModeEnabled"), 0);
            addD(o, gb, QStringLiteral("ShowGameModeNotifications"), 0);
            t.registryOps = o;
            c.push_back(t);
        }

        // Security downgrades - isolated one per tweak, each with its own warnings.
        {
            TweakSpec t;
            t.id = QStringLiteral("op.security.uac-off");
            t.category = QStringLiteral("security");
            t.subCategory = QStringLiteral("User Account Control");
            t.name = QStringLiteral("Disable User Account Control");
            t.description = QStringLiteral(
                "Turns off User Account Control so programs no longer ask for permission "
                "before making changes.");
            t.iconName = QStringLiteral("shield-off");
            t.impact = Impact::Critical;
            t.askFirst = true;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Every program runs with full administrator rights, with no prompt."),
                QStringLiteral("Store apps, widgets and parts of the Start menu stop working."),
            };
            t.concern = QStringLiteral(
                "disables UAC AND breaks UWP/Start on "
                "Win11. Never suggest it unprompted.");
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"),
                        QStringLiteral("EnableLUA"), 0, 1u),
            };
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.security.smartscreen-off");
            t.category = QStringLiteral("security");
            t.subCategory = QStringLiteral("SmartScreen");
            t.name = QStringLiteral("Disable SmartScreen");
            t.description = QStringLiteral(
                "Turns off the SmartScreen reputation check that warns before running "
                "unrecognized downloads.");
            t.iconName = QStringLiteral("shield-off");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "security downgrade. Never suggest it unprompted.");
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System"),
                        QStringLiteral("EnableSmartScreen"), 0, std::nullopt),
                stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer"),
                         QStringLiteral("SmartScreenEnabled"), QStringLiteral("Off"), std::nullopt),
            };
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.security.cpu-mitigations-off");
            t.category = QStringLiteral("security");
            t.subCategory = QStringLiteral("Exploit mitigations");
            t.name = QStringLiteral("Disable processor security mitigations");
            t.description = QStringLiteral(
                "Turns off the processor-level protections Windows added for the Spectre "
                "and Meltdown class of attacks, trading a small amount of security for a "
                "little processor performance.");
            t.iconName = QStringLiteral("shield-off");
            t.impact = Impact::Critical;
            t.askFirst = true;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Removes processor defenses against data-theft attacks. Only for a PC that runs nothing untrusted."),
            };
            t.effects = {
                QStringLiteral("The speed gain is small on current processors."),
            };
            t.concern = QStringLiteral(
                "disables Spectre/Meltdown mitigations - a real security downgrade. "
                "Never suggest it unprompted.");
            const QString mm = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management");
            t.registryOps = {
                dwordOp(mm, QStringLiteral("FeatureSettings"), 1, std::nullopt),
                dwordOp(mm, QStringLiteral("FeatureSettingsOverride"), 3, std::nullopt),
                dwordOp(mm, QStringLiteral("FeatureSettingsOverrideMask"), 3, std::nullopt),
            };
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.security.attack-surface");
            t.category = QStringLiteral("security");
            t.subCategory = QStringLiteral("Attack surface");
            t.name = QStringLiteral("Reduce attack surface");
            t.description = QStringLiteral(
                "Keeps the downloaded-from-the-internet mark on saved files, turns off "
                "AutoPlay on new media, and turns off Remote Assistance.");
            t.iconName = QStringLiteral("shield-check");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            QVector<RegistryOp> o;
            addD(o, QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Attachments"), QStringLiteral("SaveZoneInformation"), 1);
            addD(o, QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\AutoplayHandlers"), QStringLiteral("DisableAutoplay"), 1);
            addD(o, QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Remote Assistance"), QStringLiteral("fAllowToGetHelp"), 0);
            t.registryOps = o;
            c.push_back(t);
        }

        // System, logon and boot.
        {
            TweakSpec t;
            t.id = QStringLiteral("op.system.logon-boot");
            t.category = QStringLiteral("general");
            t.subCategory = QStringLiteral("Logon and boot");
            t.name = QStringLiteral("Shorten sign-in and shutdown");
            t.description = QStringLiteral(
                "Shows detailed status messages during sign-in and shutdown, skips the "
                "first-logon animation and the startup sound, and stops apps from blocking "
                "shutdown.");
            t.iconName = QStringLiteral("power");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            QVector<RegistryOp> o;
            const QString ps = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System");
            addD(o, ps, QStringLiteral("DisableAutomaticRestartSignOn"), 1);
            addD(o, ps, QStringLiteral("DelayedDesktopSwitchTimeout"), 0);
            addD(o, ps, QStringLiteral("VerboseStatus"), 1);
            addD(o, ps, QStringLiteral("EnableFirstLogonAnimation"), 0);
            addD(o, ps, QStringLiteral("DisableCAD"), 1);
            addD(o, ps, QStringLiteral("EnableLinkedConnections"), 0);
            const QString psys = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System");
            addD(o, psys, QStringLiteral("AllowBlockingAppsAtShutdown"), 0);
            addD(o, psys, QStringLiteral("DisableAcrylicBackgroundOnLogon"), 1);
            addD(o, psys, QStringLiteral("DisableLockScreenAppNotifications"), 1);
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Authentication\\LogonUI\\BootAnimation"), QStringLiteral("DisableStartupSound"), 1);
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Serialize"), QStringLiteral("StartupDelayInMSec"), 0);
            t.registryOps = o;
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.system.driver-metadata");
            t.category = QStringLiteral("general");
            t.subCategory = QStringLiteral("Drivers");
            t.name = QStringLiteral("Stop online driver metadata");
            t.description = QStringLiteral(
                "Stops Windows from fetching driver descriptions and device icons from the "
                "internet and from reporting missing-driver events.");
            t.iconName = QStringLiteral("devices");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            QVector<RegistryOp> o;
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DriverSearching"), QStringLiteral("SearchOrderConfig"), 0);
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Device Metadata"), QStringLiteral("PreventDeviceMetadataFromNetwork"), 1);
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DeviceInstall\\Settings"), QStringLiteral("DisableSendGenericDriverNotFoundToWER"), 1);
            addD(o, QStringLiteral("HKLM\\SYSTEM\\DriverDatabase\\Policies\\Settings"), QStringLiteral("DisableSendGenericDriverNotFoundToWER"), 1);
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\DeviceHealthAttestationService"), QStringLiteral("DisableSendGenericDriverNotFoundToWER"), 1);
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\WUDF"), QStringLiteral("LogLevel"), 0);
            t.registryOps = o;
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.debloat.maintenance-restore");
            t.category = QStringLiteral("general");
            t.subCategory = QStringLiteral("Maintenance");
            t.name = QStringLiteral("Turn off automatic maintenance and diagnostics");
            t.description = QStringLiteral(
                "Turns off automatic maintenance, the scheduled diagnostics, File History "
                "and the built-in restore-point scheduler.");
            t.iconName = QStringLiteral("wrench");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.warnings = {
                QStringLiteral("Windows stops making restore points on its own. Make one before big changes."),
            };
            QVector<RegistryOp> o;
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\Maintenance"), QStringLiteral("MaintenanceDisabled"), 1);
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\ScheduledDiagnostics"), QStringLiteral("EnabledExecution"), 0);
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\ScheduledDiagnostics"), QStringLiteral("EnabledExecution"), 0);
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\FileHistory"), QStringLiteral("Disabled"), 1);
            const QString sr = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\SystemRestore");
            addD(o, sr, QStringLiteral("RPSessionInterval"), 0);
            addD(o, sr, QStringLiteral("SystemRestorePointCreationFrequency"), 0);
            t.registryOps = o;
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.debloat.windows-features");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Windows features");
            t.name = QStringLiteral("Quiet background traffic from built-in features");
            t.description = QStringLiteral(
                "Holds back map, Store and Insider background traffic, turns off Bluetooth "
                "advertising and web printing, and stops the media player and codecs from "
                "reaching out on their own.");
            t.iconName = QStringLiteral("settings");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            QVector<RegistryOp> o;
            addD(o, QStringLiteral("HKLM\\SYSTEM\\Maps"), QStringLiteral("UpdateOnlyOnWifi"), 1);
            const QString pmaps = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Maps");
            addD(o, pmaps, QStringLiteral("AutoDownloadAndUpdateMapData"), 0);
            addD(o, pmaps, QStringLiteral("AllowUntriggeredNetworkTrafficOnSettingsPage"), 0);
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\current\\device\\Bluetooth"), QStringLiteral("AllowAdvertising"), 0);
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\WindowsStore"), QStringLiteral("AutoDownload"), 2);
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\current\\device\\Browser"), QStringLiteral("AllowAddressBarDropdown"), 0);
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\EdgeUI"), QStringLiteral("DisableMFUTracking"), 1);
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\WindowsSelfHost\\UI\\Visibility"), QStringLiteral("HideInsiderPage"), 1);
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\FlightedFeatures"), QStringLiteral("ImmersiveContextMenu"), 1);
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PenWorkspace"), QStringLiteral("PenWorkspaceAppSuggestionsEnabled"), 0);
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PCHC"), QStringLiteral("PreviousUninstall"), 1);
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows NT\\Printers"), QStringLiteral("DisableWebPrinting"), 1);
            addD(o, QStringLiteral("HKCU\\SOFTWARE\\Policies\\Microsoft\\WindowsMediaPlayer"), QStringLiteral("PreventCodecDownload"), 1);
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows NT\\CurrentVersion\\Software Protection Platform"), QStringLiteral("NoGenTicket"), 1);
            addD(o, QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\NetworkProvider"), QStringLiteral("RestoreConnection"), 0);
            t.registryOps = o;
            c.push_back(t);
        }

        // Appearance and shell.
        {
            TweakSpec t;
            t.id = QStringLiteral("op.appearance.taskbar-start");
            t.category = QStringLiteral("appearance");
            t.subCategory = QStringLiteral("Taskbar and Start");
            t.name = QStringLiteral("Declutter the taskbar and Start");
            t.description = QStringLiteral(
                "Left-aligns the taskbar, hides Task View, chat, widgets and badges, opens "
                "File Explorer to This PC, and stops Start and Explorer from tracking and "
                "showing recent items.");
            t.iconName = QStringLiteral("layout");
            t.impact = Impact::Low;
            QVector<RegistryOp> o;
            const QString adv = QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced");
            addD(o, adv, QStringLiteral("TaskbarAl"), 0);
            addD(o, adv, QStringLiteral("ShowTaskViewButton"), 0);
            addD(o, adv, QStringLiteral("TaskbarMn"), 0);
            addD(o, adv, QStringLiteral("TaskbarBadges"), 0);
            addD(o, adv, QStringLiteral("TaskbarGlomLevel"), 2);
            addD(o, adv, QStringLiteral("ShowInfoTip"), 0);
            addD(o, adv, QStringLiteral("IconsOnly"), 1);
            addD(o, adv, QStringLiteral("LaunchTo"), 1);
            addD(o, adv, QStringLiteral("Start_TrackProgs"), 0);
            addD(o, adv, QStringLiteral("Start_TrackDocs"), 0);
            addD(o, adv, QStringLiteral("DisallowShaking"), 1);
            addD(o, adv, QStringLiteral("ShowSyncProviderNotifications"), 0);
            addD(o, QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced\\People"), QStringLiteral("PeopleBand"), 0);
            const QString ex = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer");
            addD(o, ex, QStringLiteral("HubMode"), 1);
            addD(o, ex, QStringLiteral("ShowFrequent"), 0);
            addD(o, ex, QStringLiteral("ShowRecent"), 0);
            addD(o, ex, QStringLiteral("HoverSelectDesktops"), 0);
            t.requiresAdmin = true;
            t.registryOps = o;
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.appearance.show-hidden-files");
            t.category = QStringLiteral("appearance");
            t.subCategory = QStringLiteral("Explorer");
            t.name = QStringLiteral("Show hidden and system files");
            t.description = QStringLiteral(
                "Shows hidden files, and also the protected operating-system files Explorer "
                "keeps out of sight.");
            t.iconName = QStringLiteral("folder");
            t.impact = Impact::Low;
            t.effects = {
                QStringLiteral("Protected system files show in Explorer too."),
            };
            const QString adv = QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced");
            t.registryOps = {
                dwordOp(adv, QStringLiteral("Hidden"), 1, 2u),
                dwordOp(adv, QStringLiteral("ShowSuperHidden"), 1, 0u),
            };
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.appearance.visual-effects");
            t.category = QStringLiteral("appearance");
            t.subCategory = QStringLiteral("Visual effects");
            t.name = QStringLiteral("Reduce visual effects");
            t.description = QStringLiteral(
                "Turns off animations, transparency, window shadows and the menu-open delay "
                "for a flatter, faster-feeling desktop.");
            t.iconName = QStringLiteral("palette");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            QVector<RegistryOp> o;
            addD(o, QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VisualEffects"), QStringLiteral("VisualFXSetting"), 3);
            o.push_back(binOp(QStringLiteral("HKCU\\Control Panel\\Desktop"), QStringLiteral("UserPreferencesMask"),
                              bytes({ 0x90,0x12,0x03,0x80,0x10,0x00,0x00,0x00 }),
                              bytes({ 0x9E,0x1E,0x07,0x80,0x12,0x00,0x00,0x00 })));
            const QString desk = QStringLiteral("HKCU\\Control Panel\\Desktop");
            addS(o, desk, QStringLiteral("DragFullWindows"), QStringLiteral("0"));
            addS(o, desk, QStringLiteral("MenuShowDelay"), QStringLiteral("0"));
            addD(o, desk, QStringLiteral("PaintDesktopVersion"), 1);
            addS(o, QStringLiteral("HKCU\\Control Panel\\Desktop\\WindowMetrics"), QStringLiteral("MinAnimate"), QStringLiteral("0"));
            addD(o, QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("TaskbarAnimations"), 0);
            const QString dwm = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM");
            addD(o, dwm, QStringLiteral("EnableAeroPeek"), 0);
            addD(o, dwm, QStringLiteral("AlwaysHibernateThumbnails"), 0);
            const QString adv = QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced");
            addD(o, adv, QStringLiteral("ListviewShadow"), 0);
            addD(o, adv, QStringLiteral("ListviewAlphaSelect"), 0);
            addD(o, QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"), QStringLiteral("EnableTransparency"), 0);
            const QString cur = QStringLiteral("HKCU\\Control Panel\\Cursors");
            addD(o, cur, QStringLiteral("ContactVisualization"), 0);
            addD(o, cur, QStringLiteral("GestureVisualization"), 0);
            addD(o, QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Multimedia\\Audio"), QStringLiteral("UserDuckingPreference"), 3);
            t.registryOps = o;
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.appearance.explorer-privacy");
            t.category = QStringLiteral("appearance");
            t.subCategory = QStringLiteral("Explorer");
            t.name = QStringLiteral("Explorer link tracking off");
            t.description = QStringLiteral(
                "Stops Explorer from tracking and repairing shortcut targets, hides the "
                "Meet Now button, and turns off its online tips.");
            t.iconName = QStringLiteral("link");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            QVector<RegistryOp> o;
            const QString pe = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer");
            addD(o, pe, QStringLiteral("LinkResolveIgnoreLinkInfo"), 1);
            addD(o, pe, QStringLiteral("NoResolveSearch"), 1);
            addD(o, pe, QStringLiteral("NoResolveTrack"), 1);
            addD(o, pe, QStringLiteral("NoInternetOpenWith"), 1);
            addD(o, pe, QStringLiteral("NoInstrumentation"), 1);
            addD(o, pe, QStringLiteral("HideSCAMeetNow"), 1);
            addD(o, pe, QStringLiteral("AllowOnlineTips"), 0);
            addD(o, pe, QStringLiteral("NoLowDiskSpaceChecks"), 1);
            t.registryOps = o;
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.appearance.thumbnails-off");
            t.category = QStringLiteral("appearance");
            t.subCategory = QStringLiteral("Explorer");
            t.name = QStringLiteral("Turn off file thumbnails");
            t.description = QStringLiteral(
                "Shows generic file-type icons instead of image and document previews in "
                "Explorer.");
            t.iconName = QStringLiteral("image");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.effects = {
                QStringLiteral("Explorer shows icons instead of picture and document previews."),
            };
            const QString pe = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer");
            t.registryOps = {
                dwordOp(pe, QStringLiteral("DisableThumbnails"), 1, std::nullopt),
                dwordOp(pe, QStringLiteral("NoThumbnailCache"), 1, std::nullopt),
                dwordOp(pe, QStringLiteral("DisableThumbnailCache"), 1, std::nullopt),
            };
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.appearance.hide-tray-icons");
            t.category = QStringLiteral("appearance");
            t.subCategory = QStringLiteral("Taskbar and Start");
            t.name = QStringLiteral("Hide the notification tray");
            t.description = QStringLiteral(
                "Hides the notification-area icons and their balloon pop-ups.");
            t.iconName = QStringLiteral("bell");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.warnings = {
                QStringLiteral("Hides every tray icon, volume, network and battery included."),
            };
            const QString pe = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer");
            t.registryOps = {
                dwordOp(pe, QStringLiteral("NoAutoTrayNotify"), 1, std::nullopt),
                dwordOp(pe, QStringLiteral("TaskbarNoNotification"), 1, std::nullopt),
                dwordOp(pe, QStringLiteral("NoTrayItemsDisplay"), 1, std::nullopt),
            };
            c.push_back(t);
        }

        // Input.
        {
            TweakSpec t;
            t.id = QStringLiteral("op.input.mouse-acceleration-off");
            t.category = QStringLiteral("general");
            t.subCategory = QStringLiteral("Mouse");
            t.name = QStringLiteral("Disable mouse acceleration");
            t.description = QStringLiteral(
                "Turns off Enhance pointer precision, so the cursor moves the same "
                "distance for the same hand movement at any speed. Most games expect "
                "this.");
            t.iconName = QStringLiteral("mouse");
            t.impact = Impact::Low;
            QVector<RegistryOp> o;
            const QString mouse = QStringLiteral("HKCU\\Control Panel\\Mouse");
            o.push_back(binOp(mouse, QStringLiteral("SmoothMouseXCurve"),
                bytes({ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0xC0,0xCC,0x0C,0x00,0x00,0x00,0x00,0x00,
                        0x80,0x99,0x19,0x00,0x00,0x00,0x00,0x00, 0x40,0x66,0x26,0x00,0x00,0x00,0x00,0x00,
                        0x00,0x33,0x33,0x00,0x00,0x00,0x00,0x00 }),
                bytes({ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x15,0x6E,0x00,0x00,0x00,0x00,0x00,0x00,
                        0x00,0x40,0x01,0x00,0x00,0x00,0x00,0x00, 0x29,0xDC,0x03,0x00,0x00,0x00,0x00,0x00,
                        0x00,0x00,0x28,0x00,0x00,0x00,0x00,0x00 })));
            o.push_back(binOp(mouse, QStringLiteral("SmoothMouseYCurve"),
                bytes({ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x38,0x00,0x00,0x00,0x00,0x00,
                        0x00,0x00,0x70,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0xA8,0x00,0x00,0x00,0x00,0x00,
                        0x00,0x00,0xE0,0x00,0x00,0x00,0x00,0x00 }),
                bytes({ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x38,0x00,0x00,0x00,0x00,0x00,
                        0x00,0x00,0x70,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0xA8,0x00,0x00,0x00,0x00,0x00,
                        0x00,0x00,0xE0,0x00,0x00,0x00,0x00,0x00 })));
            addS(o, mouse, QStringLiteral("MouseSensitivity"), QStringLiteral("10"));
            addS(o, mouse, QStringLiteral("MouseSpeed"), QStringLiteral("0"));
            addS(o, mouse, QStringLiteral("MouseThreshold1"), QStringLiteral("0"));
            addS(o, mouse, QStringLiteral("MouseThreshold2"), QStringLiteral("0"));
            t.registryOps = o;
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.input.keyboard-response");
            t.category = QStringLiteral("general");
            t.subCategory = QStringLiteral("Keyboard");
            t.name = QStringLiteral("Improve keyboard and mouse response");
            t.description = QStringLiteral(
                "Sets the fastest key-repeat, deepens the keyboard and mouse input queues, "
                "and turns off the Sticky, Filter and Toggle Keys shortcut prompts.");
            t.iconName = QStringLiteral("keyboard");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            QVector<RegistryOp> o;
            addD(o, QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\kbdclass\\Parameters"), QStringLiteral("KeyboardDataQueueSize"), 0x14);
            addD(o, QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\mouclass\\Parameters"), QStringLiteral("MouseDataQueueSize"), 0x14);
            const QString kb = QStringLiteral("HKCU\\Control Panel\\Keyboard");
            addS(o, kb, QStringLiteral("KeyboardDelay"), QStringLiteral("0"));
            addS(o, kb, QStringLiteral("KeyboardSpeed"), QStringLiteral("31"));
            addD(o, QStringLiteral("HKCU\\Control Panel\\Accessibility"), QStringLiteral("DynamicScrollbars"), 0);
            addS(o, QStringLiteral("HKCU\\Control Panel\\Accessibility\\MouseKeys"), QStringLiteral("Flags"), QStringLiteral("0"));
            addS(o, QStringLiteral("HKCU\\Control Panel\\Accessibility\\StickyKeys"), QStringLiteral("Flags"), QStringLiteral("0"));
            addS(o, QStringLiteral("HKCU\\Control Panel\\Accessibility\\Keyboard Response"), QStringLiteral("Flags"), QStringLiteral("0"));
            addS(o, QStringLiteral("HKCU\\Control Panel\\Accessibility\\ToggleKeys"), QStringLiteral("Flags"), QStringLiteral("0"));
            t.registryOps = o;
            c.push_back(t);
        }

        // Storage, multimedia and GPU.
        {
            TweakSpec t;
            t.id = QStringLiteral("op.storage.ntfs-tunneling-off");
            t.category = QStringLiteral("storage");
            t.subCategory = QStringLiteral("NTFS");
            t.name = QStringLiteral("Disable NTFS file-name tunneling");
            t.description = QStringLiteral(
                "Turns off the legacy behavior that briefly remembers the name and dates of "
                "a deleted or renamed file so a quick recreate inherits them.");
            t.iconName = QStringLiteral("database");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem"),
                        QStringLiteral("MaximumTunnelEntries"), 0, std::nullopt),
            };
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.performance.multimedia-scheduler");
            t.category = QStringLiteral("general");
            t.subCategory = QStringLiteral("Multimedia (MMCSS)");
            t.name = QStringLiteral("Keep the multimedia scheduler awake");
            t.description = QStringLiteral(
                "Stops the multimedia scheduler from dropping into its idle mode between "
                "bursts of audio and video work, so games and media get its priority boost "
                "without a wake-up delay.");
            t.iconName = QStringLiteral("gauge");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "NoLazyMode and AlwaysOn are read by the multimedia scheduler service but are "
                "not documented by Microsoft; the effect is workload-dependent.");
            const QString sp = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile");
            t.registryOps = {
                dwordOp(sp, QStringLiteral("NoLazyMode"), 1, std::nullopt),
                dwordOp(sp, QStringLiteral("AlwaysOn"), 1, std::nullopt),
            };
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.gpu.directx-vrr-off");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("DirectX");
            t.name = QStringLiteral("Turn off DirectX variable refresh rate");
            t.description = QStringLiteral(
                "Turns off the global DirectX variable-refresh-rate optimization for apps "
                "that do not manage it themselves.");
            t.iconName = QStringLiteral("gpu");
            t.impact = Impact::Low;
            t.registryOps = {
                stringOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\DirectX\\UserGpuPreferences"),
                         QStringLiteral("DirectXUserGlobalSettings"),
                         QStringLiteral("VRROptimizeEnable=0;"), std::nullopt),
            };
            c.push_back(t);
        }

        // Power.
        {
            TweakSpec t;
            t.id = QStringLiteral("op.power.cpu-idle-off");
            t.category = QStringLiteral("power");
            t.subCategory = QStringLiteral("Processor");
            t.name = QStringLiteral("Disable processor idle states");
            t.description = QStringLiteral(
                "Stops the processor from entering its low-power idle states, keeping every "
                "core ready to respond instantly.");
            t.iconName = QStringLiteral("cpu");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.warnings = {
                QStringLiteral("Task Manager shows the processor at 100% the whole time. Nothing is actually running."),
            };
            t.effects = {
                QStringLiteral("Power draw, heat and fan noise stay near full load."),
            };
            t.concern = QStringLiteral(
                "raises sustained power draw and temperature; only sensible on a desktop with "
                "real cooling. Effect on latency is small and workload-dependent.");
            t.dynamicOps = { powercfgIdleDisableOp(t.id) };
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.power.memory-compression-off");
            t.category = QStringLiteral("power");
            t.subCategory = QStringLiteral("Memory");
            t.name = QStringLiteral("Disable memory compression");
            t.description = QStringLiteral(
                "Turns off the feature that compresses least-used memory to fit more in RAM, "
                "trading a little processor work for the compression against more paging to "
                "disk when memory runs low.");
            t.iconName = QStringLiteral("layers");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "benefit is machine-dependent: helps CPU-bound systems with plenty of RAM, "
                "hurts memory-tight ones by forcing more disk paging.");
            t.dynamicOps = { memoryCompressionOffOp(t.id) };
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.performance.memory-management");
            t.category = QStringLiteral("general");
            t.subCategory = QStringLiteral("Memory");
            t.name = QStringLiteral("Memory manager settings");
            t.description = QStringLiteral(
                "Applies a set of long-standing memory-manager values: a larger system "
                "cache, no page combining, and no page-file wipe at shutdown.");
            t.iconName = QStringLiteral("layers");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "several of these values (SecondLevelDataCache, the pool-size hints) are "
                "legacy no-ops on modern Windows. "
                "Effect is not measured.");
            QVector<RegistryOp> o;
            const QString mm = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management");
            addD(o, mm, QStringLiteral("LargeSystemCache"), 1);
            addD(o, mm, QStringLiteral("DisablePagingCombining"), 1);
            addD(o, mm, QStringLiteral("ClearPageFileAtShutdown"), 0);
            addD(o, mm, QStringLiteral("IoPageLockLimit"), 0);
            addD(o, mm, QStringLiteral("NonPagedPoolQuota"), 0);
            addD(o, mm, QStringLiteral("NonPagedPoolSize"), 0);
            addD(o, mm, QStringLiteral("PagedPoolQuota"), 0);
            addD(o, mm, QStringLiteral("PagedPoolSize"), 0);
            addD(o, mm, QStringLiteral("PhysicalAddressExtension"), 1);
            addD(o, mm, QStringLiteral("PoolUsageMaximum"), 0x28);
            addD(o, mm, QStringLiteral("SecondLevelDataCache"), 0x100);
            addD(o, mm, QStringLiteral("SessionPoolSize"), 0x20);
            addD(o, mm, QStringLiteral("SessionViewSize"), 0x20);
            addD(o, mm, QStringLiteral("SystemPages"), 0);
            t.registryOps = o;
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.performance.disable-pagefile");
            t.category = QStringLiteral("general");
            t.subCategory = QStringLiteral("Memory");
            t.name = QStringLiteral("Disable the page file");
            t.description = QStringLiteral(
                "Turns off the Windows page file so memory is served entirely from RAM.");
            t.iconName = QStringLiteral("database");
            t.impact = Impact::Critical;
            t.askFirst = true;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Many games crash with \"out of memory\" errors without a page file, even with RAM to spare."),
            };
            t.effects = {
                QStringLiteral("Full crash dumps can no longer be written."),
            };
            t.concern = QStringLiteral(
                "risky. Reverting hands the page file back to Windows' "
                "automatic management rather than a captured prior size.");
            // An empty list means no page file, and ?:\pagefile.sys is system-managed.
            RegistryOp pf;
            pf.path = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management");
            pf.name = QStringLiteral("PagingFiles");
            pf.kind = vth::services::RegistryValueKind::MultiString;
            pf.appliedValue = QStringList();
            pf.revertValue  = QStringList{ QStringLiteral("?:\\pagefile.sys") };
            t.registryOps = { pf };
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.performance.superfetch-prefetch-off");
            t.category = QStringLiteral("general");
            t.subCategory = QStringLiteral("Memory");
            t.name = QStringLiteral("Turn off prefetch and boot layout optimization");
            t.description = QStringLiteral(
                "Turns off the prefetcher and the boot-time file layout optimization, the "
                "background pre-loading that mainly helped on mechanical drives. The SysMain "
                "service itself is a separate tweak.");
            t.iconName = QStringLiteral("rocket");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "prefetch/SysMain do little on an SSD but are not harmful on one either; the "
                "gain from disabling them is small and drive-dependent.");
            QVector<RegistryOp> o;
            const QString pp = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management\\PrefetchParameters");
            addD(o, pp, QStringLiteral("EnableBoottrace"), 0);
            addD(o, pp, QStringLiteral("EnablePrefetcher"), 0);
            addD(o, pp, QStringLiteral("EnableSuperfetch"), 0);
            addD(o, pp, QStringLiteral("SfTracingState"), 1);
            // REG_SZ, because the defrag engine ignores any other type.
            addS(o, QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Dfrg\\BootOptimizeFunction"), QStringLiteral("Enable"), QStringLiteral("N"));
            addD(o, QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\OptimalLayout"), QStringLiteral("EnableAutoLayout"), 0);
            t.registryOps = o;
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.gpu.msi-mode");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("Interrupts");
            t.name = QStringLiteral(
                "Use message-signaled interrupts for the graphics card");
            t.description = QStringLiteral(
                "Switches every graphics adapter to message-signaled interrupts, which "
                "can lower interrupt latency.");
            t.iconName = QStringLiteral("gpu");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "usually a win but hardware-dependent; a few GPUs regress or misbehave with "
                "MSI mode. Reverting restores each adapter's captured prior setting.");
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                for (const auto& pci : dev::enumeratePciInstances()) {
                    if (pci.classGuid != kDisplayClass) continue;
                    const QString p = pci.instanceKeyPath
                        + QStringLiteral("\\Device Parameters\\Interrupt Management\\MessageSignaledInterruptProperties");
                    v << dwordT(p, QStringLiteral("MSISupported"), 1);
                }
                return v;
            }) };
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.power.transition-latency");
            t.category = QStringLiteral("power");
            t.subCategory = QStringLiteral("Latency");
            t.name = QStringLiteral("Lower power-state transition latency");
            t.description = QStringLiteral(
                "Tells the power manager and the display driver to treat idle and "
                "power-state transitions as low-latency, so the system wakes back to full "
                "responsiveness faster.");
            t.iconName = QStringLiteral("timer");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "many of these latency-tolerance values are undocumented and their real "
                "effect is not measured.");
            QVector<RegistryOp> o;
            const QString pw = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power");
            for (const char* n : { "ExitLatency", "ExitLatencyCheckEnabled", "Latency",
                    "LatencyToleranceDefault", "LatencyToleranceFSVP",
                    "LatencyTolerancePerfOverride", "LatencyToleranceScreenOffIR",
                    "RtlCapabilityCheckLatency" })
                addD(o, pw, QLatin1String(n), 1);
            const QString gp = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Power");
            for (const char* n : {
                    "DefaultD3TransitionLatencyActivelyUsed", "DefaultD3TransitionLatencyIdleLongTime",
                    "DefaultD3TransitionLatencyIdleMonitorOff", "DefaultD3TransitionLatencyIdleNoContext",
                    "DefaultD3TransitionLatencyIdleShortTime", "DefaultD3TransitionLatencyIdleVeryLongTime",
                    "DefaultLatencyToleranceIdle0", "DefaultLatencyToleranceIdle0MonitorOff",
                    "DefaultLatencyToleranceIdle1", "DefaultLatencyToleranceIdle1MonitorOff",
                    "DefaultLatencyToleranceMemory", "DefaultLatencyToleranceNoContext",
                    "DefaultLatencyToleranceNoContextMonitorOff", "DefaultLatencyToleranceOther",
                    "DefaultLatencyToleranceTimerPeriod",
                    "DefaultMemoryRefreshLatencyToleranceActivelyUsed",
                    "DefaultMemoryRefreshLatencyToleranceMonitorOff",
                    "DefaultMemoryRefreshLatencyToleranceNoContext", "Latency",
                    "MaxIAverageGraphicsLatencyInOneBucket", "MiracastPerfTrackGraphicsLatency",
                    "MonitorLatencyTolerance", "MonitorRefreshLatencyTolerance", "TransitionLatency" })
                addD(o, gp, QLatin1String(n), 1);
            t.registryOps = o;
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.power.driver-power-saving-off");
            t.category = QStringLiteral("power");
            t.subCategory = QStringLiteral("Estimation");
            t.name = QStringLiteral("Turn off energy tracking");
            t.description = QStringLiteral(
                "Stops Windows estimating battery drain and logging how much power "
                "each app uses in the background.");
            t.iconName = QStringLiteral("battery");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            QVector<RegistryOp> o;
            const QString pw = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power");
            addD(o, pw, QStringLiteral("EnergyEstimationDisabled"), 1);
            addD(o, pw, QStringLiteral("UserBatteryDischargeEstimator"), 1);
            addD(o, pw, QStringLiteral("EventProcessorEnabled"), 0);
            addD(o, pw, QStringLiteral("EnergyEstimationEnabled"), 0);
            // Per-app energy logging, the "power tracking" half.
            const QString te = pw + QStringLiteral("\\EnergyEstimation\\TaggedEnergy");
            addD(o, te, QStringLiteral("DisableTaggedEnergyLogging"), 1);
            addD(o, te, QStringLiteral("TelemetryMaxApplication"), 0);
            addD(o, te, QStringLiteral("TelemetryMaxTagPerApplication"), 0);
            t.registryOps = o;
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.power.sleep-study-off");
            t.category = QStringLiteral("power");
            t.subCategory = QStringLiteral("Diagnostics");
            t.name = QStringLiteral("Turn off Sleep Study and unhide power options");
            t.description = QStringLiteral(
                "Turns off the modern-standby Sleep Study report and unhides a few advanced "
                "entries in the power-options list.");
            t.iconName = QStringLiteral("power");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            QVector<RegistryOp> o;
            addD(o, QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("SleepStudyDisabled"), 1);
            addD(o, QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\54533251-82be-4824-96c1-47b60b740d00\\75b0ae3f-bce0-45a7-8c89-c9611c25e100"), QStringLiteral("Attributes"), 2);
            addD(o, QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\238C9FA8-0AAD-41ED-83F4-97BE242C8F20\\d4c1d4c8-d5cc-43d3-b83e-fc51215cb04d"), QStringLiteral("Attributes"), 1);
            addD(o, QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\0012ee47-9041-4b5d-9b77-535fba8b1442\\6738e2c4-e8a5-4a42-b16a-e040e769756e"), QStringLiteral("Attributes"), 1);
            t.registryOps = o;
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.power.timer-coalescing-off");
            t.category = QStringLiteral("power");
            t.subCategory = QStringLiteral("Timers");
            t.name = QStringLiteral("Disable timer coalescing");
            t.description = QStringLiteral(
                "Stops Windows from grouping timer wake-ups together, which it does to save "
                "power at the cost of a little timing precision.");
            t.iconName = QStringLiteral("clock");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "raises wake frequency slightly for a small precision gain; effect is not "
                "measured on modern builds.");
            QVector<RegistryOp> o;
            for (const char* k : {
                    "HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Power",
                    "HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management",
                    "HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel",
                    "HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Executive",
                    "HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager",
                    "HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\ModernSleep",
                    "HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power",
                    "HKLM\\SYSTEM\\CurrentControlSet\\Control" })
                addD(o, QLatin1String(k), QStringLiteral("CoalescingTimerInterval"), 0);
            t.registryOps = o;
            c.push_back(t);
        }

        // Networking.
        {
            TweakSpec t;
            t.id = QStringLiteral("op.performance.disable-hpet");
            t.category = QStringLiteral("advanced");
            t.subCategory = QStringLiteral("Timers");
            t.name = QStringLiteral("Disable HPET");
            t.description = QStringLiteral(
                "Stops Windows being forced onto the high-precision event timer, so it "
                "uses the lower-overhead built-in timer instead.");
            t.iconName = QStringLiteral("timer");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "edits the boot configuration; effect on latency is hardware-dependent and "
                "often nil on modern systems. A store backup is taken before the write.");
            t.dynamicOps = { bcdOp(t.id, {
                BcdEdit{ QStringLiteral("disabledynamictick"), QStringLiteral("yes") },
                BcdEdit{ QStringLiteral("useplatformclock"), std::nullopt },
            }) };
            c.push_back(t);
        }
        auto psDynOp = [](const QString& id, const QString& applyPs,
                          const QString& revertPs) -> DynamicOp {
            DynamicOp op;
            op.label  = id;
            op.detect = []() { return DetectState::Unknown; };
            op.apply  = [applyPs]()  { return dev::runPowershell(applyPs); };
            op.revert = [revertPs]() { return dev::runPowershell(revertPs); };
            return op;
        };

        {
            TweakSpec t;
            t.id = QStringLiteral("op.net.file-sharing-server");
            t.category = QStringLiteral("networking");
            t.subCategory = QStringLiteral("File sharing");
            t.name = QStringLiteral("Adjust the file-sharing server");
            t.description = QStringLiteral(
                "Raises the file-sharing server's I/O stack depth and request-buffer size "
                "and lowers its thread priority - favoring throughput on a PC that serves "
                "files.");
            t.iconName = QStringLiteral("server");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "legacy SMB1-era knobs with little effect on modern file sharing.");
            QVector<RegistryOp> o;
            const QString lp = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters");
            addD(o, lp, QStringLiteral("IRPStackSize"), 0x32);
            addD(o, lp, QStringLiteral("SizReqBuf"), 17424);
            addD(o, QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer"), QStringLiteral("ThreadPriority"), 0);
            t.registryOps = o;
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.net.qos-autotuning");
            t.category = QStringLiteral("networking");
            t.subCategory = QStringLiteral("QoS");
            t.name = QStringLiteral("TCP receive window and traffic marking");
            t.description = QStringLiteral(
                "Sets automatic TCP receive-window sizing to its experimental level, "
                "lets apps mark their own traffic priority, and stops the traffic "
                "scheduler depending on the network type.");
            t.iconName = QStringLiteral("route");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "experimental autotuning can help or hurt by link.");
            QVector<RegistryOp> o;
            addS(o, QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\QoS"), QStringLiteral("Do not use NLA"), QStringLiteral("1"));
            const QString q = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\QoS");
            addS(o, q, QStringLiteral("Tcp Autotuning Level"), QStringLiteral("Experimental"));
            addS(o, q, QStringLiteral("Application DSCP Marking Request"), QStringLiteral("Allowed"));
            t.registryOps = o;
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.net.connectivity-probe");
            t.category = QStringLiteral("networking");
            t.subCategory = QStringLiteral("Connectivity");
            t.name = QStringLiteral("Change the connectivity check");
            t.description = QStringLiteral(
                "Points Windows' internet and captive-portal check at Google's public DNS "
                "and shortens its timers, so the network status clears faster.");
            t.iconName = QStringLiteral("wifi");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.warnings = {
                QStringLiteral("If Google's DNS is blocked on your network, Windows can wrongly report no internet."),
            };
            t.concern = QStringLiteral(
                "changes where the probe points (Google DNS); effect is cosmetic (the status icon).");
            QVector<RegistryOp> o;
            const QString nla = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NlaSvc\\Parameters\\Internet");
            addS(o, nla, QStringLiteral("ActiveDnsProbeContent"), QStringLiteral("8.8.4.4"));
            addS(o, nla, QStringLiteral("ActiveDnsProbeContentV6"), QStringLiteral("2001:4860:4860::8844"));
            addS(o, nla, QStringLiteral("ActiveDnsProbeHost"), QStringLiteral("dns.google"));
            addS(o, nla, QStringLiteral("ActiveDnsProbeHostV6"), QStringLiteral("dns.google"));
            addS(o, nla, QStringLiteral("ActiveWebProbeContent"), QStringLiteral("Microsoft Connect Test"));
            addS(o, nla, QStringLiteral("ActiveWebProbeContentV6"), QStringLiteral("Microsoft Connect Test"));
            addS(o, nla, QStringLiteral("ActiveWebProbeHost"), QStringLiteral("www.msftconnecttest.com"));
            addS(o, nla, QStringLiteral("ActiveWebProbePath"), QStringLiteral("connecttest.txt"));
            addS(o, nla, QStringLiteral("ActiveWebProbePathV6"), QStringLiteral("connecttest.txt"));
            addD(o, nla, QStringLiteral("CaptivePortalTimer"), 0);
            addD(o, nla, QStringLiteral("CaptivePortalTimerBackOffIncrementsInSeconds"), 5);
            addD(o, nla, QStringLiteral("CaptivePortalTimerMaxInSeconds"), 30);
            addD(o, nla, QStringLiteral("EnableActiveProbing"), 1);
            addD(o, nla, QStringLiteral("PassivePollPeriod"), 15);
            addD(o, nla, QStringLiteral("StaleThreshold"), 30);
            addD(o, nla, QStringLiteral("WebTimeout"), 35);
            t.registryOps = o;
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.net.per-adapter-tcp-latency");
            t.category = QStringLiteral("networking");
            t.subCategory = QStringLiteral("Adapter");
            t.name = QStringLiteral("Reduce per-adapter TCP latency");
            t.description = QStringLiteral(
                "On every network adapter: sends small packets right away instead of "
                "batching them, acknowledges data immediately, fixes the packet size "
                "and reserves no bandwidth for traffic priority.");
            t.iconName = QStringLiteral("gauge");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "only helps TCP-based games and remote desktop; UDP titles are unaffected. "
                "Removes the QoS bandwidth reservation. Effect is NIC- and workload-dependent.");
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                for (const auto& nic : dev::enumeratePhysicalNics()) {
                    const QString p = kTcpipIf + nic.instanceGuid;
                    v << dwordT(p, QStringLiteral("TCPNoDelay"), 1)
                      << dwordT(p, QStringLiteral("TcpAckFrequency"), 1)
                      << dwordT(p, QStringLiteral("TcpDelAckTicks"), 0)
                      << dwordT(p, QStringLiteral("TcpInitialRTT"), 2)
                      << dwordT(p, QStringLiteral("MTU"), 0x5DC)
                      << dwordT(p, QStringLiteral("NonBestEffortLimit"), 0)
                      << dwordT(p, QStringLiteral("DeadGWDetectDefault"), 1)
                      << dwordT(p, QStringLiteral("PerformRouterDiscovery"), 1)
                      << dwordT(p, QStringLiteral("UseZeroBroadcast"), 0);
                }
                return v;
            }) };
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.net.adapter-power-offload");
            t.category = QStringLiteral("networking");
            t.subCategory = QStringLiteral("Adapter");
            t.name = QStringLiteral("Set adapter power and offloads");
            t.description = QStringLiteral(
                "On every physical adapter: turns off power saving, Wake-on-LAN, "
                "green-ethernet and energy-efficient ethernet, disables the checksum and "
                "large-send offloads, enables receive-side scaling, and raises the buffer "
                "and packet sizes.");
            t.iconName = QStringLiteral("network");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Turns on jumbo frames and fixes the link speed. Networks that do not support them can drop the connection."),
            };
            t.effects = {
                QStringLiteral("Network work moves from the adapter to the processor."),
            };
            t.concern = QStringLiteral(
                "aggressive per-adapter set: jumbo frames and forced speed/duplex are "
                "breakage-prone and NIC-specific; disabling offloads trades CPU for "
                "determinism. Effect varies by adapter. Never recommend unprompted.");
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                for (const auto& nic : dev::enumeratePhysicalNics()) {
                    const QString k = nic.classKeyPath;
                    for (const char* n : { "EnablePME", "EnableSavePowerNow", "EnablePowerManagement",
                            "EnableDynamicPowerGating", "EnableConnectedPowerGating",
                            "AutoPowerSaveModeEnabled", "ULPMode", "ReduceSpeedOnPowerDown",
                            "AutoDisableGigabit", "S5WakeOnLan", "PowerSavingMode", "GigaLite",
                            "EnableGreenEthernet", "WakeOnLinkChange", "WakeOnLink",
                            "*ModernStandbyWoLMagicPacket", "*WakeOnMagicPacket", "*WakeOnPattern",
                            "*FlowControl", "*EEE", "*InterruptModeration", "*IPChecksumOffloadIPv4",
                            "*LsoV2IPv4", "*LsoV2IPv6", "*PMARPOffload", "*PMNSOffload",
                            "*UDPChecksumOffloadIPv4", "*UDPChecksumOffloadIPv6" })
                        v << stringT(k, QLatin1String(n), QStringLiteral("0"));
                    v << stringT(k, QStringLiteral("*RSS"), QStringLiteral("1"))
                      << stringT(k, QStringLiteral("*NumRssQueues"), QStringLiteral("4"))
                      << stringT(k, QStringLiteral("*JumboPacket"), QStringLiteral("9014"))
                      << stringT(k, QStringLiteral("*ReceiveBuffers"), QStringLiteral("512"))
                      << stringT(k, QStringLiteral("*TransmitBuffers"), QStringLiteral("128"))
                      << stringT(k, QStringLiteral("*SpeedDuplex"), QStringLiteral("6"))
                      << stringT(k, QStringLiteral("WolShutdownLinkSpeed"), QStringLiteral("2"))
                      << stringT(k, QStringLiteral("*TCPChecksumOffloadIPv4"), QStringLiteral("1"))
                      << stringT(k, QStringLiteral("*TCPChecksumOffloadIPv6"), QStringLiteral("1"))
                      << stringT(k, QStringLiteral("*PriorityVLANTag"), QStringLiteral("1"))
                      << dwordT(k, QStringLiteral("AdvancedEEE"), 0)
                      << dwordT(k, QStringLiteral("PnPCapabilities"), 0x18);
                }
                return v;
            }) };
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.net.offload-global-off");
            t.category = QStringLiteral("networking");
            t.subCategory = QStringLiteral("Offloads");
            t.name = QStringLiteral("Disable global receive offloads");
            t.description = QStringLiteral(
                "Turns off receive-side scaling, receive-segment coalescing and the "
                "packet-coalescing filter at the whole-stack level.");
            t.iconName = QStringLiteral("network");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.warnings = {
                QStringLiteral("Pulls the opposite way to \"Set adapter power and offloads\". Use one or the other."),
            };
            t.concern = QStringLiteral(
                "contradicts the per-adapter offload tweak (which enables scaling) and generally "
                "hurts throughput on modern multi-core NICs; effect is not measured. Never "
                "recommend unprompted.");
            t.dynamicOps = { psDynOp(t.id,
                QStringLiteral("$ErrorActionPreference='SilentlyContinue';"
                    "Set-NetOffloadGlobalSetting -ReceiveSideScaling Disabled "
                    "-ReceiveSegmentCoalescing Disabled -PacketCoalescingFilter Disabled"),
                QStringLiteral("$ErrorActionPreference='SilentlyContinue';"
                    "Set-NetOffloadGlobalSetting -ReceiveSideScaling Enabled "
                    "-ReceiveSegmentCoalescing Disabled -PacketCoalescingFilter Disabled")) };
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.net.tcp-global-throughput");
            t.category = QStringLiteral("networking");
            t.subCategory = QStringLiteral("TCP/IP");
            t.name = QStringLiteral("Apply the throughput network profile");
            t.description = QStringLiteral(
                "A network profile built for throughput: experimental receive-window sizing, "
                "congestion notification on, timestamps off, a wider dynamic port range and "
                "shorter retransmit timers, plus related stack-wide settings.");
            t.iconName = QStringLiteral("route");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.effects = {
                QStringLiteral("Turning it off sets Windows' defaults, not your previous values."),
                QStringLiteral("A few of its commands do nothing on some Windows builds."),
            };
            t.concern = QStringLiteral(
                "includes deprecated no-op commands and hardcoded adapter names. Effect is link- "
                "and adapter-dependent.");
            t.dynamicOps = { psDynOp(t.id,
                QStringLiteral("$ErrorActionPreference='SilentlyContinue';"
                    "Set-NetTCPSetting -SettingName Internet -Timestamps Disabled -EcnCapability Enabled "
                    "-NonSackRttResiliency Disabled -MemoryPressureProtection Disabled -InitialRtoMs 2000 "
                    "-MaxSynRetransmissions 2 -AutoTuningLevelLocal Experimental "
                    "-DynamicPortRangeStartPort 1025 -DynamicPortRangeNumberOfPorts 64511"),
                QStringLiteral("$ErrorActionPreference='SilentlyContinue';"
                    "Set-NetTCPSetting -SettingName Internet -Timestamps Disabled -EcnCapability Disabled "
                    "-MemoryPressureProtection Enabled -InitialRtoMs 3000 -MaxSynRetransmissions 4 "
                    "-AutoTuningLevelLocal Normal")) };
            // Joined with '&', so a command newer Windows rejects doesn't stop the rest.
            CommandOp netsh;
            netsh.program = QStringLiteral("cmd");
            const QStringList head = { QStringLiteral("/c") };
            netsh.applyArgs = head;
            netsh.applyArgs << QStringLiteral(
                "netsh int tcp set heuristics disabled & "
                "netsh int tcp set supplemental template=internet congestionprovider=ctcp & "
                "netsh int tcp set global rss=enabled & "
                "netsh int tcp set global rsc=disabled & "
                "netsh int tcp set global ecncapability=enabled & "
                "netsh int tcp set global fastopen=enabled & "
                "netsh int tcp set global fastopenfallback=enabled & "
                "netsh int tcp set global pacingprofile=off & "
                "netsh int tcp set global hystart=disabled & "
                "netsh int 6to4 set state state=enabled & "
                "netsh int udp set global uro=enabled & "
                "netsh winsock set autotuning on & "
                "netsh int tcp set supplemental template=custom icw=10 & "
                "netsh interface teredo set state enterprise & "
                "netsh int tcp set security mpp=disabled & "
                "netsh int tcp set security profiles=disabled & "
                "netsh interface ipv4 set subinterface \"Wi-Fi\" mtu=1500 store=persistent & "
                "netsh interface ipv4 set subinterface \"Ethernet\" mtu=1500 store=persistent & "
                "exit /b 0");
            netsh.revertArgs = head;
            netsh.revertArgs << QStringLiteral(
                "netsh int tcp set heuristics enabled & "
                "netsh int tcp set supplemental template=internet congestionprovider=default & "
                "netsh int tcp set global rsc=enabled & "
                "netsh int tcp set global hystart=enabled & "
                "netsh int tcp set global pacingprofile=default & "
                "netsh int tcp set security mpp=enabled & "
                "netsh int tcp set security profiles=enabled & "
                "exit /b 0");
            t.commandOps = { netsh };
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.net.unbind-ipv6-qos");
            t.category = QStringLiteral("networking");
            t.subCategory = QStringLiteral("Bindings");
            t.name = QStringLiteral("Unbind IPv6 and the QoS scheduler");
            t.description = QStringLiteral(
                "Removes the IPv6 and QoS packet-scheduler bindings from every adapter, so "
                "the machine runs IPv4-only and without traffic prioritization.");
            t.iconName = QStringLiteral("shield-off");
            t.impact = Impact::Critical;
            t.askFirst = true;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Without IPv6, parts of Xbox networking and some Store apps stop working."),
            };
            t.effects = {
                QStringLiteral("QoS traffic marking stops, which some games use."),
            };
            t.concern = QStringLiteral(
                "IPv6 breaks modern services, and the scheduler carries game DSCP marking. Never "
                "recommend unprompted.");
            t.dynamicOps = { psDynOp(t.id,
                QStringLiteral("$ErrorActionPreference='SilentlyContinue';"
                    "Disable-NetAdapterBinding -Name '*' -ComponentID ms_tcpip6,ms_pacer"),
                QStringLiteral("$ErrorActionPreference='SilentlyContinue';"
                    "Enable-NetAdapterBinding -Name '*' -ComponentID ms_tcpip6,ms_pacer")) };
            c.push_back(t);
        }

        // Isolated, because disabling one of these can break the PC.
        {
            TweakSpec t;
            t.id = QStringLiteral("op.svc.disable-msaccount");
            t.category = QStringLiteral("security");
            t.subCategory = QStringLiteral("Sign-in");
            t.name = QStringLiteral("Disable Microsoft account sign-in");
            t.description = QStringLiteral(
                "Turns off the Microsoft account sign-in assistant, which handles logging in "
                "with a Microsoft account.");
            t.iconName = QStringLiteral("key");
            t.impact = Impact::Critical;
            t.askFirst = true;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("If you sign in with a Microsoft account, this can lock you out at the next sign-in."),
            };
            t.concern = QStringLiteral(
                "can lock out Microsoft-account logins. "
                "Never recommend unprompted.");
            t.registryOps = svcOff({ "wlidsvc" });
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.svc.disable-hid");
            t.category = QStringLiteral("advanced");
            t.subCategory = QStringLiteral("Input");
            t.name = QStringLiteral("Disable the Human Interface Device service");
            t.description = QStringLiteral(
                "Turns off the service that drives hot-keys and special buttons on keyboards, "
                "remotes and multimedia devices.");
            t.iconName = QStringLiteral("keyboard");
            t.impact = Impact::Critical;
            t.askFirst = true;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Keyboards, mice and other input devices can stop working. Have another way to control the PC ready."),
            };
            t.concern = QStringLiteral("high risk to input devices. Never recommend unprompted.");
            t.registryOps = svcOff({ "hidserv" });
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.svc.disable-firewall-driver");
            t.category = QStringLiteral("security");
            t.subCategory = QStringLiteral("Firewall");
            t.name = QStringLiteral("Disable the firewall authorization driver");
            t.description = QStringLiteral(
                "Turns off the driver that backs Windows Firewall authorization.");
            t.iconName = QStringLiteral("shield-off");
            t.impact = Impact::Critical;
            t.askFirst = true;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("The Windows Firewall stops protecting this PC."),
            };
            t.concern = QStringLiteral(
                "disables a firewall component - a real security downgrade. Never recommend "
                "unprompted.");
            t.registryOps = svcOff({ "mpsdrv" });
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.svc.disable-lanman-server");
            t.category = QStringLiteral("networking");
            t.subCategory = QStringLiteral("File sharing");
            t.name = QStringLiteral("Disable the file and printer sharing server");
            t.description = QStringLiteral(
                "Turns off the server that shares this PC's files, printers and named pipes "
                "over the network.");
            t.iconName = QStringLiteral("server");
            t.impact = Impact::Critical;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.effects = {
                QStringLiteral("Other PCs can no longer reach this PC's shared folders or printers."),
            };
            t.concern = QStringLiteral(
                "breaks file/print sharing. Never recommend unprompted.");
            t.registryOps = svcOff({ "LanmanServer" });
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.svc.disable-netlogon");
            t.category = QStringLiteral("security");
            t.subCategory = QStringLiteral("Domain");
            t.name = QStringLiteral("Disable Netlogon");
            t.description = QStringLiteral(
                "Turns off the service that authenticates this PC to a domain controller.");
            t.iconName = QStringLiteral("key");
            t.impact = Impact::Critical;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Breaks signing in on a PC that is joined to a work domain."),
            };
            t.concern = QStringLiteral("breaks domain authentication. Never recommend unprompted.");
            t.registryOps = svcOff({ "Netlogon" });
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.svc.disable-print-spooler");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Printing");
            t.name = QStringLiteral("Disable the Print Spooler");
            t.description = QStringLiteral(
                "Turns off the Print Spooler, which every printer and many PDF and "
                "print-to-file tools rely on.");
            t.iconName = QStringLiteral("printer");
            t.impact = Impact::Critical;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.effects = {
                QStringLiteral("Print to PDF stops working too."),
            };
            t.concern = QStringLiteral(
                "removes all printing. Often suggested to reduce attack surface; only for a PC "
                "that never prints. Never recommend unprompted.");
            t.registryOps = svcOff({ "Spooler" });
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.svc.disable-bitlocker");
            t.category = QStringLiteral("security");
            t.subCategory = QStringLiteral("Encryption");
            t.name = QStringLiteral("Disable BitLocker drive encryption");
            t.description = QStringLiteral(
                "Turns off the BitLocker drive-encryption service.");
            t.iconName = QStringLiteral("lock");
            t.impact = Impact::Critical;
            t.askFirst = true;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("A BitLocker-encrypted drive can become impossible to unlock."),
            };
            t.concern = QStringLiteral(
                "can make BitLocker drives unreadable. Never recommend unprompted.");
            t.registryOps = svcOff({ "BDESVC" });
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.svc.disable-radio");
            t.category = QStringLiteral("networking");
            t.subCategory = QStringLiteral("Radio");
            t.name = QStringLiteral("Disable the radio management service");
            t.description = QStringLiteral(
                "Turns off the radio management service that controls airplane mode and the "
                "wireless radios.");
            t.iconName = QStringLiteral("wifi");
            t.impact = Impact::Critical;
            t.askFirst = true;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Wi-Fi and Bluetooth are likely to stop working."),
            };
            t.concern = QStringLiteral(
                "can break Wi-Fi/Bluetooth. Never recommend unprompted.");
            t.registryOps = svcOff({ "RmSvc" });
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.svc.disable-remote-desktop");
            t.category = QStringLiteral("security");
            t.subCategory = QStringLiteral("Remote");
            t.name = QStringLiteral("Disable Remote Desktop");
            t.description = QStringLiteral(
                "Turns off Remote Desktop services, so no one can connect to this PC remotely.");
            t.iconName = QStringLiteral("device-desktop");
            t.impact = Impact::Critical;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.effects = {
                QStringLiteral("Incoming Remote Desktop connections stop."),
            };
            t.concern = QStringLiteral(
                "breaks remote connections. Never recommend unprompted.");
            t.registryOps = svcOff({ "TermService" });
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.svc.disable-pen");
            t.category = QStringLiteral("advanced");
            t.subCategory = QStringLiteral("Input");
            t.name = QStringLiteral("Disable pen support");
            t.description = QStringLiteral(
                "Turns off the service behind active-pen input, such as the Surface Pen.");
            t.iconName = QStringLiteral("keyboard");
            t.impact = Impact::Critical;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.effects = {
                QStringLiteral("Pen input stops working."),
            };
            t.concern = QStringLiteral("breaks pen input.");
            t.registryOps = svcOff({ "PenService" });
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("op.svc.disable-file-history");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Backup");
            t.name = QStringLiteral("Disable File History");
            t.description = QStringLiteral(
                "Turns off File History, which copies your files to a backup location.");
            t.iconName = QStringLiteral("history");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.effects = {
                QStringLiteral("File History stops backing up your files."),
            };
            t.registryOps = svcOff({ "fhsvc" });
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.svc.disable-windows-backup");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Backup");
            t.name = QStringLiteral("Disable Windows Backup");
            t.description = QStringLiteral(
                "Turns off the Windows Backup and block-level backup engine.");
            t.iconName = QStringLiteral("history");
            t.impact = Impact::High;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.effects = {
                QStringLiteral("Windows Backup stops working."),
            };
            t.concern = QStringLiteral(
                "breaks backup and recovery.");
            t.registryOps = svcOff({ "wbengine", "SDRSVC" });
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.svc.disable-secondary-logon");
            t.category = QStringLiteral("security");
            t.subCategory = QStringLiteral("Sign-in");
            t.name = QStringLiteral("Disable Secondary Logon");
            t.description = QStringLiteral(
                "Turns off Secondary Logon, which starts programs under different credentials.");
            t.iconName = QStringLiteral("key");
            t.impact = Impact::High;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.effects = {
                QStringLiteral("\"Run as different user\" stops working."),
            };
            t.concern = QStringLiteral(
                "on Vethida's protected-service list, so the engine refuses this at apply time - "
                "it is listed for completeness but does not take effect.");
            t.registryOps = svcOff({ "seclogon" });
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.svc.disable-capture-service");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Capture");
            t.name = QStringLiteral("Disable the capture service");
            t.description = QStringLiteral(
                "Turns off the capture service that the Snipping Tool and screen capture rely on.");
            t.iconName = QStringLiteral("video");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.effects = {
                QStringLiteral("The Snipping Tool and other screen capture stop working."),
            };
            t.registryOps = svcOff({ "CaptureService" });
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.svc.disable-hyperv");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Virtualization");
            t.name = QStringLiteral("Disable Hyper-V services");
            t.description = QStringLiteral(
                "Turns off the Hyper-V host and guest-integration services.");
            t.iconName = QStringLiteral("device-desktop");
            t.impact = Impact::High;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("WSL2, virtual machines and some security features stop working."),
            };
            t.concern = QStringLiteral(
                "only for a PC that runs no virtual machines and no Hyper-V-backed features.");
            t.registryOps = svcOff({ "HvHost", "vmickvpexchange", "vmicguestinterface",
                "vmicshutdown", "vmicheartbeat", "vmcompute", "vmicvmsession", "vmicrdv",
                "vmictimesync", "vmms", "vmicvss" });
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("op.svc.edge-services-off");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Microsoft Edge");
            t.name = QStringLiteral("Disable Edge background services");
            t.description = QStringLiteral(
                "Turns off Microsoft Edge's updater and elevation background services.");
            t.iconName = QStringLiteral("globe");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "overlaps the Edge-removal tweak, which disables these same services by command; "
                "both together are idempotent (no shared registry value, so no ledger conflict).");
            t.registryOps = svcOff({ "MicrosoftEdgeElevationService", "edgeupdate", "edgeupdatem" });
            c.push_back(t);
        }

        // Feature groups.
        {
            TweakSpec t;
            t.id = QStringLiteral("op.svc.disable-bluetooth");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Bluetooth");
            t.name = QStringLiteral("Disable Bluetooth");
            t.description = QStringLiteral(
                "Turns off the Bluetooth stack and its supporting services.");
            t.iconName = QStringLiteral("bluetooth");
            t.impact = Impact::Critical;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.effects = {
                QStringLiteral("Bluetooth devices stop working."),
            };
            t.registryOps = svcOff({ "BTHPORT", "BthAvctpSvc", "BTAGService", "bthserv",
                                     "BluetoothUserService" });
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.svc.disable-smartcard");
            t.category = QStringLiteral("security");
            t.subCategory = QStringLiteral("Smart card");
            t.name = QStringLiteral("Disable smart card support");
            t.description = QStringLiteral(
                "Turns off the smart-card services, including certificate propagation and the "
                "smart-card sign-in policy.");
            t.iconName = QStringLiteral("key");
            t.impact = Impact::High;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.effects = {
                QStringLiteral("Smart-card sign-in stops working."),
            };
            t.registryOps = svcOff({ "SCardSvr", "ScDeviceEnum", "SCPolicySvc", "CertPropSvc" });
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.svc.disable-store-appx");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Store");
            t.name = QStringLiteral("Disable Store and app deployment services");
            t.description = QStringLiteral(
                "Turns off the Store licensing, app-deployment, client-license and install "
                "services.");
            t.iconName = QStringLiteral("package-off");
            t.impact = Impact::High;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Store apps stop installing, updating and sometimes opening, the NVIDIA Control Panel among them."),
            };
            t.effects = {
                QStringLiteral("Some third-party installers fail too."),
            };
            t.concern = QStringLiteral(
                "breaks Store packages. Never recommend unprompted.");
            t.registryOps = svcOff({ "LicenseManager", "AppXSVC", "ClipSVC", "InstallService",
                                     "EntAppSvc" });
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.svc.disable-sysmain");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Caching");
            t.name = QStringLiteral("Disable SysMain");
            t.description = QStringLiteral(
                "Turns off SysMain, the background pre-loading and caching service that mainly "
                "helped on mechanical drives.");
            t.iconName = QStringLiteral("rocket");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.registryOps = svcOff({ "SysMain" });
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.svc.time-sync-off");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Time");
            t.name = QStringLiteral("Disable automatic time and time zone");
            t.description = QStringLiteral(
                "Turns off Windows time synchronization and the automatic time-zone and "
                "cellular-time updates.");
            t.iconName = QStringLiteral("clock");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("The clock drifts, which can break sign-ins and secure websites."),
            };
            t.concern = QStringLiteral(
                "the core time service is on Vethida's protected list and the engine refuses it "
                "at apply time; the auto time-zone/cellular parts do apply.");
            t.registryOps = svcOff({ "W32Time", "tzautoupdate", "autotimesvc" });
            c.push_back(t);
        }

        // Driver groups.
        {
            TweakSpec t;
            t.id = QStringLiteral("op.drv.legacy-ports");
            t.category = QStringLiteral("advanced");
            t.subCategory = QStringLiteral("Legacy drivers");
            t.name = QStringLiteral("Disable legacy port drivers");
            t.description = QStringLiteral(
                "Turns off drivers for hardware almost no modern PC has: the PC speaker beep, "
                "parallel and serial ports, and infrared.");
            t.iconName = QStringLiteral("usb");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.effects = {
                QStringLiteral("Serial and parallel port devices stop working."),
            };
            t.registryOps = svcOff({ "Beep", "Parport", "Serial", "IrDA", "TermDD" });
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.drv.legacy-network");
            t.category = QStringLiteral("networking");
            t.subCategory = QStringLiteral("Legacy drivers");
            t.name = QStringLiteral("Disable legacy network drivers");
            t.description = QStringLiteral(
                "Turns off legacy network drivers and protocols: NetBIOS, link-layer topology "
                "discovery, the tunnel and port-sharing drivers and similar.");
            t.iconName = QStringLiteral("network");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Older file sharing and some VPNs can stop working."),
            };
            t.concern = QStringLiteral(
                "overlaps the per-adapter binding tweak on the link-layer-topology drivers, by a "
                "different mechanism.");
            t.registryOps = svcOff({ "MsLldp", "lltdio", "rspndr", "NdisVirtualBus", "NetBIOS",
                "NetBT", "tcpipreg", "tunnel", "lmhosts", "NetTcpPortSharing",
                "IpxlatCfgSvc", "bowser", "afunix", "iphlpsvc" });
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.drv.wan-ras");
            t.category = QStringLiteral("networking");
            t.subCategory = QStringLiteral("Dial-up and VPN");
            t.name = QStringLiteral("Disable dial-up and VPN drivers");
            t.description = QStringLiteral(
                "Turns off the WAN, dial-up and built-in VPN miniport drivers (PPTP, L2TP, "
                "SSTP, IKEv2, PPPoE) and routing.");
            t.iconName = QStringLiteral("route");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Windows' built-in VPN stops working. VPN apps with their own driver are not affected."),
            };
            t.registryOps = svcOff({ "NdisWan", "PptpMiniport", "RasAgileVpn", "Rasl2tp",
                "RasPppoe", "RasSstp", "wanarp", "Wanarpv6", "AsyncMac", "SstpSvc",
                "RemoteAccess", "RasAuto" });
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.drv.acpi-misc");
            t.category = QStringLiteral("advanced");
            t.subCategory = QStringLiteral("System drivers");
            t.name = QStringLiteral("Disable assorted system drivers");
            t.description = QStringLiteral(
                "Turns off a set of niche system drivers: the ACPI processor aggregator, the "
                "trusted-runtime proxy, the container filesystem, and the WARP just-in-time "
                "compiler service.");
            t.iconName = QStringLiteral("cpu");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "niche drivers; benefit is negligible and a couple can affect containers or "
                "software rendering on specific setups. Never recommend unprompted.");
            t.registryOps = svcOff({ "acpipagr", "WindowsTrustedRTProxy",
                                     "wcifs", "WarpJITSvc" });
            c.push_back(t);
        }

        // Service groups.
        {
            TweakSpec t;
            t.id = QStringLiteral("op.svc.telemetry-diagnostics");
            t.category = QStringLiteral("privacy");
            t.subCategory = QStringLiteral("Diagnostics");
            t.name = QStringLiteral("Disable diagnostics and telemetry services");
            t.description = QStringLiteral(
                "Turns off the diagnostic tracking, problem-reporting, troubleshooting, "
                "data-usage and performance-logging services.");
            t.iconName = QStringLiteral("eye-off");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.effects = {
                QStringLiteral("Windows troubleshooters stop working."),
            };
            t.registryOps = svcOff({ "DiagTrack", "diagsvc", "DPS", "WdiServiceHost",
                "WdiSystemHost", "pla", "TroubleshootingSvc",
                "diagnosticshub.standardcollector.service", "SgrmAgent", "DusmSvc", "WerSvc",
                "PcaSvc", "Wecsvc", "dmwappushservice", "Telemetry", "Ndu" });
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.svc.xbox");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Xbox");
            t.name = QStringLiteral("Disable Xbox services");
            t.description = QStringLiteral(
                "Turns off the Xbox Live sign-in, save, networking and game-input services and "
                "the game DVR broadcast service.");
            t.iconName = QStringLiteral("gamepad-2");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("The Xbox app, Game Pass and cloud saves stop working."),
            };
            t.registryOps = svcOff({ "XblGameSave", "XboxNetApiSvc", "XboxGipSvc",
                                     "XblAuthManager", "BcastDVRUserService" });
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.svc.sensors-perception");
            t.category = QStringLiteral("privacy");
            t.subCategory = QStringLiteral("Sensors");
            t.name = QStringLiteral("Disable sensor and location services");
            t.description = QStringLiteral(
                "Turns off the sensor, geolocation and mixed-reality perception services.");
            t.iconName = QStringLiteral("world");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.effects = {
                QStringLiteral("Auto-brightness and location features stop working."),
            };
            t.registryOps = svcOff({ "SensorDataService", "SensrSvc", "SensorService", "lfsvc",
                "spectrum", "perceptionsimulation", "SharedRealitySvc" });
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.svc.sync-userdata");
            t.category = QStringLiteral("privacy");
            t.subCategory = QStringLiteral("Sync");
            t.name = QStringLiteral("Disable sync and user-data services");
            t.description = QStringLiteral(
                "Turns off the contact and message sync, clipboard history and "
                "connected-devices user services.");
            t.iconName = QStringLiteral("refresh-cw");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("New Bluetooth devices cannot be paired. Ones already paired keep working."),
            };
            t.effects = {
                QStringLiteral("Clipboard history and cross-device features stop working."),
            };
            t.registryOps = svcOff({ "OneSyncSvc", "UnistoreSvc", "UserDataSvc",
                "PimIndexMaintenanceSvc", "MessagingService", "cbdhsvc", "CDPSvc", "CDPUserSvc",
                "DevicesFlowUserSvc", "ConsentUxUserSvc", "DevicePickerUserSvc" });
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.svc.media-discovery");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Media and discovery");
            t.name = QStringLiteral("Disable media sharing and network discovery");
            t.description = QStringLiteral(
                "Turns off media streaming, UPnP/SSDP discovery, peer-to-peer networking, "
                "internet connection sharing and the related discovery services.");
            t.iconName = QStringLiteral("network");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.effects = {
                QStringLiteral("Casting and network media sharing stop working."),
            };
            t.registryOps = svcOff({ "WMPNetworkSvc", "QWAVE", "SSDPSRV", "upnphost", "FDResPub",
                "PeerDistSvc", "p2psvc", "p2pimsvc", "PNRPsvc", "Wcncsvc", "WFDSConMgrSvc",
                "NcdAutoSetup", "NcbService", "NcaSvc", "SharedAccess" });
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.svc.storage-backup");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Storage");
            t.name = QStringLiteral("Disable extra storage services");
            t.description = QStringLiteral(
                "Turns off volume shadow copy, storage tiering, drive optimization, offline "
                "files, still-image acquisition and the iSCSI initiator.");
            t.iconName = QStringLiteral("database");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Restore points, many backup tools, scanners and iSCSI storage stop working."),
            };
            t.concern = QStringLiteral(
                "Volume Shadow Copy and drive-optimize disables affect restore points and the "
                "defrag UI; the storage service is on the protected list and is refused.");
            t.registryOps = svcOff({ "VSS", "TieringEngineService", "StorSvc", "defragsvc",
                "CscService", "CloudBackupRestoreSvc", "MSiSCSI", "ShellHWDetection", "WiaRpc",
                "stisvc" });
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.svc.cellular-mobile");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Mobile");
            t.name = QStringLiteral("Disable cellular and mobile hotspot");
            t.description = QStringLiteral(
                "Turns off the mobile broadband and internet-sharing (mobile hotspot) services.");
            t.iconName = QStringLiteral("wifi");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.effects = {
                QStringLiteral("Cellular data and the mobile hotspot stop working."),
            };
            t.registryOps = svcOff({ "WwanSvc", "icssvc" });
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.svc.fax-telephony");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Telephony");
            t.name = QStringLiteral("Disable fax and telephony services");
            t.description = QStringLiteral(
                "Turns off fax, telephony, the phone service, NFC payments and the portable-device "
                "enumerator.");
            t.iconName = QStringLiteral("phone");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.effects = {
                QStringLiteral("Fax, tap-to-pay and some phone and camera connections stop working."),
            };
            t.registryOps = svcOff({ "Fax", "TapiSrv", "PhoneSvc", "SEMgrSvc", "WPDBusEnum" });
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.svc.printing-extras");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Printing");
            t.name = QStringLiteral("Disable printer extras");
            t.description = QStringLiteral(
                "Turns off printer notifications and the print-workflow service (leaving the "
                "main spooler alone).");
            t.iconName = QStringLiteral("printer");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.registryOps = svcOff({ "PrintNotify", "PrintWorkflowUserSvc" });
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.svc.remote-hardening");
            t.category = QStringLiteral("security");
            t.subCategory = QStringLiteral("Remote");
            t.name = QStringLiteral("Disable remote and legacy management services");
            t.description = QStringLiteral(
                "Turns off Remote Registry, the SNMP trap listener, the application-layer "
                "gateway and the Remote Desktop configuration service.");
            t.iconName = QStringLiteral("shield");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.effects = {
                QStringLiteral("Incoming Remote Desktop connections stop."),
            };
            t.registryOps = svcOff({ "RemoteRegistry", "SNMPTRAP", "ALG", "SessionEnv" });
            c.push_back(t);
        }
        {
            TweakSpec t;
            t.id = QStringLiteral("op.svc.assorted-background");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Background");
            t.name = QStringLiteral("Disable assorted background services");
            t.description = QStringLiteral(
                "Turns off background services most PCs never use: offline maps, the wallet, "
                "work-account enrollment, the Insider program, retail demo and kiosk mode, "
                "parental controls, Dynamic Lock, the keyboard filter, the SSH agent, adaptive "
                "display brightness and the graphics performance monitor.");
            t.iconName = QStringLiteral("settings");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.effects = {
                QStringLiteral("Adaptive brightness, Dynamic Lock, offline map updates and the SSH agent stop working."),
            };
            t.concern = QStringLiteral(
                "every service here is optional on a home PC; the features they back are named "
                "in the warning.");
            t.registryOps = svcOff({ "MapsBroker", "WalletService",
                "DmEnrollmentSvc", "wisvc", "WEPHOSTSVC", "AxInstSV", "AJRouter", "WpcMonSvc",
                "shpamsvc", "NaturalAuthentication", "MsKeyboardFilter", "ssh-agent",
                "DisplayEnhancementService", "RetailDemo", "AssignedAccessManagerSvc", "VacSvc",
                "GraphicsPerfSvc" });
            c.push_back(t);
        }
    }
}

namespace vth::tweaks::detail
{
namespace
{
}
}

