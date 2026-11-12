#include "Preflight.h"

#include "../Windows/Privileges.h"

#include <QStandardPaths>
#include <QStorageInfo>

#include <windows.h>
#  include <windows.h>

namespace vth::preflight
{
    namespace
    {
        CheckResult checkAdmin() {
            CheckResult r;
            if (vth::win::isElevated()) {
                r.message = QStringLiteral("Running with administrator privileges.");
            } else {
                r.outcome = Outcome::Block;
                r.message = QStringLiteral("Vethida is running without administrator privileges. "
                                           "Most registry, service, and BCD writes will fail.");
                r.remediation = QStringLiteral("Right-click vethida.exe, then choose Run as administrator.");
            }
            return r;
        }

        CheckResult checkDiskSpace() {
            constexpr qint64 kMinBytes = 1024LL * 1024 * 1024;

            CheckResult r;
            const QString home = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
            QStorageInfo si(home);
            if (!si.isValid() || !si.isReady()) {
                r.outcome = Outcome::Warn;
                r.message = QStringLiteral("Could not query disk space at %1.").arg(home);
                return r;
            }

            const qint64 free = si.bytesAvailable();
            if (free < kMinBytes) {
                r.outcome = Outcome::Block;
                r.message = QStringLiteral(
                    "Only %1 MB free on the drive backing Vethida's data directory; %2 MB is required for "
                    "backup files and the system-restore-point margin.")
                    .arg(free / (1024 * 1024))
                    .arg(kMinBytes / (1024 * 1024));
                r.remediation = QStringLiteral("Free up disk space and try again.");
            } else {
                r.message = QStringLiteral("%1 GB free on the data drive.").arg(free / (1024.0 * 1024.0 * 1024.0), 0, 'f', 1);
            }
            return r;
        }

        CheckResult checkBattery() {
            constexpr int kMinPercent = 30;

            CheckResult r;
            SYSTEM_POWER_STATUS sps{};
            if (!::GetSystemPowerStatus(&sps)) {
                r.message = QStringLiteral("Power status unavailable; assuming AC.");
                return r;
            }
            if (sps.BatteryFlag == 128) {   // no battery
                r.message = QStringLiteral("No battery (desktop system).");
                return r;
            }
            if (sps.ACLineStatus == 1) {
                r.message = QStringLiteral("On AC power.");
                return r;
            }
            const int pct = sps.BatteryLifePercent == 255 ? -1 : sps.BatteryLifePercent;
            if (pct >= 0 && pct < kMinPercent) {
                r.outcome = Outcome::Block;
                r.message = QStringLiteral("On battery at %1%; minimum is %2% to run an apply.").arg(pct).arg(kMinPercent);
                r.remediation = QStringLiteral("Plug in the laptop and try again.");
            } else {
                r.outcome = Outcome::Warn;
                r.message = pct >= 0
                    ? QStringLiteral("On battery at %1%. Plugging in is recommended.").arg(pct)
                    : QStringLiteral("On battery; charge unknown. Plugging in is recommended.");
                r.remediation = QStringLiteral("Plug in to avoid mid-apply power loss.");
            }
            return r;
        }

        CheckResult checkPendingUpdate() {
            CheckResult r;
            HKEY key = nullptr;
            static const wchar_t* kPaths[] = {
                L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\Auto Update\\RebootRequired",
                L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Component Based Servicing\\RebootPending",
            };
            for (const wchar_t* p : kPaths) {
                if (::RegOpenKeyExW(HKEY_LOCAL_MACHINE, p, 0, KEY_READ, &key) == ERROR_SUCCESS) {
                    ::RegCloseKey(key);
                    r.outcome = Outcome::Warn;
                    r.message = QStringLiteral("A Windows update is pending a reboot. Applying tweaks during a "
                                               "mid-install state can produce inconsistent results.");
                    r.remediation = QStringLiteral("Reboot first, let the update finish, then run Vethida.");
                    return r;
                }
            }
            r.message = QStringLiteral("No pending Windows Update reboot.");
            return r;
        }
    }

    QVector<CheckResult> Report::blockers() const {
        QVector<CheckResult> v;
        for (const auto& r : results) if (r.outcome == Outcome::Block) v.append(r);
        return v;
    }

    Report check() {
        Report rep;
        rep.results.append(checkAdmin());
        rep.results.append(checkDiskSpace());
        rep.results.append(checkBattery());
        rep.results.append(checkPendingUpdate());
        return rep;
    }
}

namespace vth::preflight
{
    QVector<CheckResult> Report::warnings() const {
        QVector<CheckResult> v;
        for (const auto& r : results) if (r.outcome == Outcome::Warn) v.append(r);
        return v;
    }
}
