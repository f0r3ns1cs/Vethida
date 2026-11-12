#pragma once

#include <QString>
#include <QVector>

namespace vth::win::bloatware
{
    enum class Severity {
        Recommended,
        Optional,
        Risky,
    };

    QString severityLabel(Severity s);

    struct Entry {
        QString  name;
        QString  displayName;
        QString  publisher;
        Severity severity;
        QString  description;
        QString  impact;
    };

    const QVector<Entry>& catalog();

    const Entry* findByName(const QString& packageName);
}
