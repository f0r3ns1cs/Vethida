#pragma once

#include <QDate>
#include <QImage>
#include <QMetaType>
#include <QString>
#include <QVector>
#include <QtTypes>

namespace vth::win::apps
{
    enum class Type {
        Desktop,
        UWP,
    };

    enum class Source {
        PerMachine,
        PerUser,
        UwpPerUser,
    };

    enum class Architecture {
        X64,
        X86,
        Arm64,
        Neutral,
        Unknown,
    };

    QString typeLabel(Type t);
    QString sourceLabel(Source s);
    QString architectureLabel(Architecture a);

    struct AppInfo {
        Type    type = Type::Desktop;

        QString displayName;
        QString publisher;
        QString version;
        QString installLocation;

        QImage  iconImage;

        QString uninstallString;
        QString quietUninstallString;

        QString packageName;       // identity, e.g. "Microsoft.GetHelp"
        QString packageFullName;
        bool    nonRemovable    = false;
        bool    systemComponent = false;   // signed as part of Windows

        QDate         installDate;
        quint64       estimatedBytes = 0;
        Source        source         = Source::PerMachine;
        Architecture  architecture   = Architecture::Unknown;
        QString comments;
        QString helpLink;
        QString contactInfo;
        QString modifyPath;
        QString displayIcon;
        QString registryKey;
        QString registryHive;
        QString registryPath;
        bool    windowsInstaller = false;
        QString packageFamilyName;
    };

    // The icon Explorer shows for a program without one.
    QImage defaultAppIcon();

    QVector<AppInfo> enumerate();
    QVector<AppInfo> enumerateDesktop();
    QVector<AppInfo> enumerateStore();

    bool launchUninstall(const AppInfo& app);

    enum class UwpRemoveResult {
        Ok,
        AccessDenied,
        NotFound,
        InvalidName,
        OtherError,
    };
    QString          uwpRemoveResultLabel(UwpRemoveResult r);
    UwpRemoveResult  removeUwpPackage(const QString& packageFullName);

    bool openInstallLocation(const AppInfo& app);
}

Q_DECLARE_METATYPE(vth::win::apps::AppInfo)

namespace vth::win::apps
{
    enum class Category {
        User,
        Bloatware,
        System,
        Microsoft,
    };
    QString categoryLabel(Category c);
    Category classify(const AppInfo& app);
}
