#include "RetentionEnforcementService.h"

#include "../Runtime/Paths.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace vth::services
{
    namespace
    {
        void pruneOlderThan(const QString& dir, int days)
        {
            QDir d(dir);
            if (!d.exists())
                return;
            const QDateTime cutoff = QDateTime::currentDateTimeUtc().addDays(-days);
            for (const QFileInfo& fi : d.entryInfoList(QDir::Files | QDir::NoDotAndDotDot)) {
                if (fi.lastModified().toUTC() < cutoff)
                    QFile::remove(fi.absoluteFilePath());
            }
        }
    }

    RetentionEnforcementService::RetentionEnforcementService(QObject* parent)
        : QObject(parent)
    {
        m_timer.setInterval(24 * 60 * 60 * 1000);   // 24 hours
        connect(&m_timer, &QTimer::timeout, this, &RetentionEnforcementService::runOnce);
        m_timer.start();
        runOnce();
    }

    void RetentionEnforcementService::runOnce()
    {
        pruneOlderThan(vth::paths::userLogs(), LogRetentionDays);
        pruneOlderThan(vth::paths::userCrashes(), CrashRetentionDays);
        pruneOlderThan(vth::paths::userCache(), CacheRetentionDays);
    }
}
