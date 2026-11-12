#include "Paths.h"

#include "../Windows/Privileges.h"
#include "../Logger.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

#include <windows.h>
#include <aclapi.h>
#include <sddl.h>
#include <shlobj.h>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")

namespace vth::paths
{
    namespace
    {
        QString cachedInstallDir;
        QString cachedUserData;
        QString cachedProtected;

        VTH_DEFINE_LOG_CATEGORY(catPaths, "vth.paths")

        // Full control for SYSTEM and Administrators, nothing inherited from the
        // parent (P), inherited by everything created inside (OICI).
        constexpr const wchar_t* kProtectedSddl = L"D:P(A;OICI;FA;;;SY)(A;OICI;FA;;;BA)";

        struct Descriptor {
            PSECURITY_DESCRIPTOR sd = nullptr;
            ~Descriptor() { if (sd) LocalFree(sd); }
            bool build() {
                return ConvertStringSecurityDescriptorToSecurityDescriptorW(
                    kProtectedSddl, SDDL_REVISION_1, &sd, nullptr) != FALSE;
            }
        };

        bool ownedByAdministrators(const std::wstring& path) {
            PSID owner = nullptr;
            PSECURITY_DESCRIPTOR sd = nullptr;
            if (GetNamedSecurityInfoW(path.c_str(), SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION,
                                      &owner, nullptr, nullptr, nullptr, &sd) != ERROR_SUCCESS)
                return false;
            const bool ok = owner && (IsWellKnownSid(owner, WinBuiltinAdministratorsSid)
                                      || IsWellKnownSid(owner, WinLocalSystemSid));
            LocalFree(sd);
            return ok;
        }

        // Creates `path` with the protected access. False if it exists and is
        // not owned by Administrators or SYSTEM, or cannot be created.
        bool createProtected(const QString& path) {
            const std::wstring native = QDir::toNativeSeparators(path).toStdWString();
            Descriptor d;
            if (!d.build())
                return false;
            SECURITY_ATTRIBUTES sa{ sizeof(sa), d.sd, FALSE };
            if (CreateDirectoryW(native.c_str(), &sa))
                return true;
            if (GetLastError() != ERROR_ALREADY_EXISTS)
                return false;
            if (!ownedByAdministrators(native))
                return false;
            // Ours from an earlier run: reapply the protected access.
            PACL dacl = nullptr;
            BOOL present = FALSE, defaulted = FALSE;
            if (!GetSecurityDescriptorDacl(d.sd, &present, &dacl, &defaulted) || !present)
                return false;
            return SetNamedSecurityInfoW(const_cast<wchar_t*>(native.c_str()), SE_FILE_OBJECT,
                                         DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
                                         nullptr, nullptr, dacl, nullptr) == ERROR_SUCCESS;
        }

        QString userStateDir() {
            const QString p = QDir::cleanPath(userData() + QStringLiteral("/state"));
            ensureDir(p);
            return p;
        }

        QString computeProtectedData() {
            // Without an organization name, the path below would be %ProgramData%
            // itself, and its access must not be rewritten.
            if (!vth::win::isElevated() || QCoreApplication::organizationName().isEmpty())
                return userStateDir();

            PWSTR programData = nullptr;
            QString base;
            if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_ProgramData, 0, nullptr, &programData)))
                base = QString::fromWCharArray(programData);
            CoTaskMemFree(programData);
            if (base.isEmpty())
                base = qEnvironmentVariable("ProgramData", QStringLiteral("C:/ProgramData"));

            const QString org  = QDir::cleanPath(base + QLatin1Char('/')
                                                 + QCoreApplication::organizationName());
            const QString root = org + QLatin1Char('/') + QStringLiteral("Vethida");

            bool ok = createProtected(org) && createProtected(root);
            if (!ok) {
                // Someone else may have made it first. Set that copy aside and retry.
                for (const QString& dir : { root, org }) {
                    if (QFileInfo::exists(dir) && !ownedByAdministrators(
                            QDir::toNativeSeparators(dir).toStdWString())) {
                        QDir().rename(dir, dir + QStringLiteral(".untrusted-")
                                      + QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMddHHmmss")));
                    }
                }
                ok = createProtected(org) && createProtected(root);
            }
            if (!ok) {
                qCWarning(catPaths) << "could not create the protected data directory; "
                                       "falling back to the user profile";
                return userStateDir();
            }
            return root;
        }

        QString computeInstallDir() {
            QFileInfo fi(QCoreApplication::applicationDirPath());
            return QDir::cleanPath(fi.absoluteFilePath());
        }

        QString computeUserData() {
            QString p = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
            if (p.isEmpty()) {
                p = QDir::homePath() + QStringLiteral("/AppData/Local/Vethida");
            }
            return QDir::cleanPath(p);
        }
    }

    QString installDir() {
        if (cachedInstallDir.isEmpty()) {
            cachedInstallDir = computeInstallDir();
        }
        return cachedInstallDir;
    }

    QString userData() {
        if (cachedUserData.isEmpty()) {
            cachedUserData = computeUserData();
            ensureDir(cachedUserData);
        }
        return cachedUserData;
    }

    QString protectedData() {
        if (cachedProtected.isEmpty())
            cachedProtected = computeProtectedData();
        return cachedProtected;
    }

    QString userLogs() {
        QString p = protectedData() + QStringLiteral("/logs");
        ensureDir(p);
        return p;
    }

    QString userCrashes() {
        QString p = protectedData() + QStringLiteral("/crashes");
        ensureDir(p);
        return p;
    }

    QString userCache() {
        QString p = userData() + QStringLiteral("/cache");
        ensureDir(p);
        return p;
    }

    QString shippedCfg() {
        return installDir() + QStringLiteral("/cfg");
    }

    bool ensureDir(const QString& absolutePath) {
        return QDir().mkpath(absolutePath);
    }
}

namespace vth::paths
{
    QString shippedThemes() {
        return installDir() + QStringLiteral("/themes");
    }
    QString shippedLicenses() {
        return installDir() + QStringLiteral("/licenses");
    }
    QString lgplLicenseFile() {
        return shippedLicenses() + QStringLiteral("/LGPL-3.0.txt");
    }
    QString gplLicenseFile() {
        return shippedLicenses() + QStringLiteral("/GPL-3.0.txt");
    }
    QString thirdPartyNoticesFile() {
        return shippedLicenses() + QStringLiteral("/THIRD_PARTY_NOTICES.txt");
    }
}
