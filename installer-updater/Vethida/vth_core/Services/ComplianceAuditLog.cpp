#include "ComplianceAuditLog.h"

#include "../Runtime/Paths.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>
#include <QMutexLocker>

namespace vth::services
{
    namespace
    {
        QMutex g_lock;

        QString logPath()
        {
            return QDir(vth::paths::userData()).filePath(QStringLiteral("audit.log"));
        }
    }

    QString ComplianceAuditLog::filePath()
    {
        return logPath();
    }

    void ComplianceAuditLog::record(const QString& event, const QString& detail)
    {
        QMutexLocker lock(&g_lock);
        vth::paths::ensureDir(vth::paths::userData());

        QJsonObject obj;
        obj[QStringLiteral("ts")] = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
        obj[QStringLiteral("event")] = event;
        if (!detail.isEmpty())
            obj[QStringLiteral("detail")] = detail;

        QFile f(logPath());
        if (f.open(QIODevice::Append | QIODevice::Text)) {
            f.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
            f.write("\n");
        }
    }

    QVector<AuditEntry> ComplianceAuditLog::readAll()
    {
        QMutexLocker lock(&g_lock);
        QVector<AuditEntry> out;
        QFile f(logPath());
        if (!f.exists() || !f.open(QIODevice::ReadOnly | QIODevice::Text))
            return out;

        while (!f.atEnd()) {
            const QByteArray line = f.readLine().trimmed();
            if (line.isEmpty())
                continue;
            const QJsonDocument doc = QJsonDocument::fromJson(line);
            if (!doc.isObject())
                continue;
            const QJsonObject obj = doc.object();
            AuditEntry e;
            e.timestampUtc = obj.value(QStringLiteral("ts")).toString();
            e.event = obj.value(QStringLiteral("event")).toString();
            e.detail = obj.value(QStringLiteral("detail")).toString();
            out.push_back(e);
        }
        return out;
    }

    void ComplianceAuditLog::clear()
    {
        {
            QMutexLocker lock(&g_lock);
            QFile::remove(logPath());
        }
        record(QStringLiteral("audit.cleared"),
               QStringLiteral("Local audit log cleared by data erasure."));
    }
}
