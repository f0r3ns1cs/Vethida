#pragma once

#include <QtCore/QString>
#include <QtCore/QHash>

namespace vth::services
{
    // sentry-native, loaded at runtime. Without sentry.dll everything here does
    // nothing, so development builds don't need it or its dependencies.
    class SentryBridge
    {
    public:
        enum class Level : int {
            Debug   = -1,
            Info    = 0,
            Warning = 1,
            Error   = 2,
            Fatal   = 3,
        };
        static void addBreadcrumb(const QString& message,
                                  const QString& category,
                                  const QString& type,
                                  Level level,
                                  const QHash<QString, QString>& data);
        static void captureMessage(const QString& message,
                                   Level level,
                                   const QHash<QString, QString>& tags,
                                   const QHash<QString, QString>& extras);
        static bool isInitialized();
        static bool tryLoad();
        // False if the DLL is missing or incomplete. Later calls do nothing.
        static bool initialize(const QString& dsn,
                               const QString& release,
                               const QString& environment,
                               const QString& databasePath,
                               bool debug);

        static void shutdown();

        static void setTag(const QString& key, const QString& value);
    };
}
