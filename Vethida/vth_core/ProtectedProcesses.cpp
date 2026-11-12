#include "ProtectedProcesses.h"

#include <QHash>
#include <QStringLiteral>

#include <algorithm>

namespace vth::win::processes
{
    namespace
    {

        // Keys keep Windows' spelling for display. Lookups go through lowerIndex().
        const QHash<QString, QString>& protectedTable() {
            static const QHash<QString, QString> kTable = {
                {QStringLiteral("System"),
                 QStringLiteral("The kernel's own process container. It has no image to stop.")},
                {QStringLiteral("Registry"),
                 QStringLiteral("The registry hive host. Terminating it bugchecks the machine.")},
                {QStringLiteral("MemCompression"),
                 QStringLiteral("The compressed memory store. Terminating it bugchecks the machine.")},
                {QStringLiteral("Memory Compression"),
                 QStringLiteral("The compressed memory store, under the name older builds report. Same effect.")},
                {QStringLiteral("smss.exe"),
                 QStringLiteral("Session Manager. Terminating it bugchecks the machine.")},
                {QStringLiteral("csrss.exe"),
                 QStringLiteral("Client/Server Runtime. Terminating it bugchecks the machine.")},
                {QStringLiteral("wininit.exe"),
                 QStringLiteral("Windows Initialization. Owns the session-0 service tree; terminating it bugchecks the machine.")},
                {QStringLiteral("winlogon.exe"),
                 QStringLiteral("Logon. Terminating it ends the desktop session and signs the user out.")},
                {QStringLiteral("LogonUI.exe"),
                 QStringLiteral("Logon interface. Draws the sign-in and lock screens; without it a locked machine cannot be unlocked.")},
                {QStringLiteral("lsass.exe"),
                 QStringLiteral("Local Security Authority. Holds every authentication token; terminating it bugchecks the machine.")},
                {QStringLiteral("LsaIso.exe"),
                 QStringLiteral("Credential Guard's isolated LSA. Same effect as lsass.exe on a machine that runs it.")},
                {QStringLiteral("services.exe"),
                 QStringLiteral("Service Control Manager. Terminating it bugchecks the machine.")},
                {QStringLiteral("svchost.exe"),
                 QStringLiteral("Shared service host. One image name covers dozens of unrelated Windows services, so it can never be a safe target.")},
                {QStringLiteral("WUDFHost.exe"),
                 QStringLiteral("User-mode driver host. Terminating it drops the devices whose drivers it hosts, including input.")},
                {QStringLiteral("dwm.exe"),
                 QStringLiteral("Desktop Window Manager. Compositing stops and every window goes black until Windows starts it again.")},
                {QStringLiteral("explorer.exe"),
                 QStringLiteral("The desktop shell. Genesis has one manager that stops it deliberately and starts it again on deactivate; nothing else may.")},
                {QStringLiteral("sihost.exe"),
                 QStringLiteral("Shell Infrastructure Host. The taskbar, the tray and the Start menu stop responding without it.")},
                {QStringLiteral("ctfmon.exe"),
                 QStringLiteral("Text services. Keyboard layouts, IME input and the touch keyboard stop working.")},
                {QStringLiteral("fontdrvhost.exe"),
                 QStringLiteral("Font driver host. Text rendering across the desktop degrades until Windows starts it again.")},
                {QStringLiteral("conhost.exe"),
                 QStringLiteral("Console host. Terminating it kills whatever console application it is hosting, including a Vethida operation.")},
                {QStringLiteral("audiodg.exe"),
                 QStringLiteral("Audio Device Graph. Audio output stops for every application on the machine.")},
                {QStringLiteral("TrustedInstaller.exe"),
                 QStringLiteral("Windows Modules Installer. Terminating it mid-servicing can leave the component store needing a repair install.")},
                {QStringLiteral("MsMpEng.exe"),
                 QStringLiteral("Microsoft Defender Antivirus engine. Stopping real-time protection is malware behavior; no Vethida path may do it.")},
                {QStringLiteral("NisSrv.exe"),
                 QStringLiteral("Defender Network Inspection. Tied to the antivirus engine; the same rule applies.")},
                {QStringLiteral("SecurityHealthService.exe"),
                 QStringLiteral("Windows Security service. Without it the Windows Security interface cannot open and its state cannot be read back.")},
                {QStringLiteral("SecurityHealthSystray.exe"),
                 QStringLiteral("Windows Security tray. The user's route into Windows Security.")},

                {QStringLiteral("vgc.exe"),
                 QStringLiteral("Riot Vanguard. Required to play Valorant and League of Legends; stopping it locks the account out of a match in progress.")},
                {QStringLiteral("vgtray.exe"),
                 QStringLiteral("Riot Vanguard tray. Vanguard treats its absence as tampering.")},
                {QStringLiteral("EasyAntiCheat.exe"),
                 QStringLiteral("EasyAntiCheat. Stopping it ends the match and can register as a kick.")},
                {QStringLiteral("EasyAntiCheat_EOS.exe"),
                 QStringLiteral("EasyAntiCheat, Epic Online Services build. Same effect.")},
                {QStringLiteral("BEService.exe"),
                 QStringLiteral("BattlEye. Stopping it ends the match and can register as a kick.")},
                {QStringLiteral("BEService_x64.exe"),
                 QStringLiteral("BattlEye, 64-bit build. Same effect.")},
                {QStringLiteral("FACEITService.exe"),
                 QStringLiteral("FACEIT Anti-Cheat. Stopping it forfeits the match.")},
                {QStringLiteral("FACEITClient.exe"),
                 QStringLiteral("FACEIT client. Required alongside the anti-cheat service.")},
                {QStringLiteral("eslwire.exe"),
                 QStringLiteral("ESL Wire anti-cheat. Stopping it forfeits the match.")},
                {QStringLiteral("Ricochet.exe"),
                 QStringLiteral("Call of Duty RICOCHET anti-cheat. Stopping it ends the match.")},

                {QStringLiteral("vethida.exe"),
                 QStringLiteral("Vethida. Closing it mid-session leaves the session's changes on the machine with no process left to take them off.")},
            };
            return kTable;
        }

        const QHash<QString, QString>& lowerIndex() {
            static const QHash<QString, QString> kIndex = [] {
                QHash<QString, QString> out;
                out.reserve(protectedTable().size());
                for (auto it = protectedTable().cbegin(); it != protectedTable().cend(); ++it)
                    out.insert(it.key().toLower(), it.key());
                return out;
            }();
            return kIndex;
        }

        QString lookupKey(QStringView imageName) {
            return imageName.toString().trimmed().toLower();
        }

    }

    bool isProtected(QStringView imageName) {
        return lowerIndex().contains(lookupKey(imageName));
    }

    QString protectedReason(QStringView imageName) {
        const QString display = lowerIndex().value(lookupKey(imageName));
        return display.isEmpty() ? QString{} : protectedTable().value(display);
    }

    const QStringList& protectedNames() {
        static const QStringList kNames = [] {
            QStringList out = protectedTable().keys();
            std::sort(out.begin(), out.end(),
                      [](const QString& a, const QString& b) {
                          return a.compare(b, Qt::CaseInsensitive) < 0;
                      });
            return out;
        }();
        return kNames;
    }
}
