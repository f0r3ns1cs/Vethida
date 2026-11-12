#pragma once

#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>

#include <functional>
#include <optional>

namespace vth::win::installer
{
    // The publisher's own download, used when the Windows Package Manager can't help.
    struct DirectSource {
        enum class Locate {
            Fixed,          // url always serves the current release
            GitHubLatest,   // url is "owner/repo"; pattern picks the asset
            PageLink,       // url is a page; pattern finds the download link
            NodeLts,        // nodejs.org's release index, newest LTS
        };
        enum class Kind { Exe, Msi, Zip };

        Locate      locate = Locate::Fixed;
        QString     url;
        QString     pattern;
        Kind        kind = Kind::Exe;
        QStringList args;        // silent switches for Exe; extra properties for Msi
        QString     signer;      // required in the signer's name; empty if the publisher doesn't sign
        QString     zipFolder;   // Zip only: folder under Program Files
        QString     zipLaunch;   // Zip only: program for the Start menu shortcut
    };

    struct PackageDef {
        QString id;            // exact winget package id, e.g. "Mozilla.Firefox"
        QString name;
        QString category;
        QString description;
        QString installedName; // pattern for the DisplayName in the installed programs list
        std::optional<DirectSource> direct;
    };

    enum class InstallResult {
        Ok,
        AlreadyInstalled,
        InvalidId,
        Failed,
    };

    struct InstallOutcome {
        InstallResult result = InstallResult::Failed;
        QString       message;
        bool          needsRestart = false;
    };

    // Progress text, called on the worker thread.
    using StatusFn = std::function<void(const QString&)>;

    const QVector<PackageDef>& catalog();

    QSet<QString> installedAmong(const QStringList& candidates);

    // winget, then the publisher's installer, then winget's catalog directly (SHA-256
    // checked). Blocking.
    InstallOutcome install(const QString& id, const StatusFn& status = {});
}
