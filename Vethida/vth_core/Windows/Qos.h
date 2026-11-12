#pragma once

#include <QString>
#include <QVector>

namespace vth::win::qos
{
    enum class Direction { Inbound, Outbound, Both };
    enum class Protocol  { TCP, UDP, Both };

    QString directionLabel(Direction d);
    QString protocolLabel(Protocol p);

    struct Policy {
        QString   id;            // empty for unsaved
        QString   name;
        QString   application;   // path or name; empty matches everything
        Direction direction      = Direction::Outbound;
        Protocol  protocol       = Protocol::TCP;
        QString   portRange;     // a single port; empty matches any
        int       dscp           = 0;
        QString   throttleRate   = QStringLiteral("Unlimited");
        bool      enabled        = true;
    };

    struct Preset {
        QString    id;
        QString    name;
        QString    description;
        int        dscp         = 0;
        Direction  direction    = Direction::Outbound;
        Protocol   protocol     = Protocol::TCP;
        QString    iconKey;
    };

    struct Summary {
        int total  = 0;
        int active = 0;
        int    disabled         = 0;
        double totalManagedMbps = 0.0;
    };

    QVector<Policy> loadStore();

    Policy  createPolicy(Policy templ);
    bool    deletePolicy(const QString& id);
    bool    togglePolicy(const QString& id);

    Summary summarize(const QVector<Policy>& policies);

    enum class Result {
        Ok,
        AccessDenied,
        NotFound,
        InvalidArgument,
        OtherError,
    };

    QString resultLabel(Result r);

    // Creates the Windows QoS rule, or updates the one with this name. Needs elevation.
    Result applyToSystem(const Policy& p, QString* errorOut = nullptr);

    // Needs elevation.
    Result removeFromSystem(const QString& policyName, QString* errorOut = nullptr);

    // "10 Mbps" or "1.5 Mbps". Returns 0 for anything else, including "Unlimited".
    double parseThrottleMbps(const QString& rate);

    const QVector<Preset>& presets();
}

namespace vth::win::qos
{
    bool    updatePolicy(const Policy& updated);
}
