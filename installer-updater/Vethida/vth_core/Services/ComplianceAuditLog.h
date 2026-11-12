#pragma once

#include <QString>
#include <QVector>

namespace vth::services
{
    // Local audit log (JSON lines) for consent changes, data export/erase and license
    // activation. Nothing else gets logged here.
    struct AuditEntry
    {
        QString timestampUtc;   // ISO-8601 UTC
        QString event;          // stable machine key, e.g. "consent.telemetry.granted"
        QString detail;         // optional human-readable context
    };

    class ComplianceAuditLog
    {
    public:
        static void record(const QString& event, const QString& detail = QString());

        static QVector<AuditEntry> readAll(); // empty if missing or unreadable
        static QString filePath();
        static void clear(); // leaves a single "audit.cleared" entry behind
    };
}
