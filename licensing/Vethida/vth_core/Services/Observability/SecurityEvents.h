#pragma once

#include <QtCore/QDateTime>
#include <QtCore/QHash>
#include <QtCore/QString>

#include <functional>
#include <memory>

namespace vth::services::observability
{
    enum class SecurityEventType {
        SpkiMismatch,
        TlsPinMatch,
        TlsChainValidationFailed,
        DohAllProvidersFailed,
        DohDisagreement,
        DeviceAuthRejected,
        JwsVerificationFailed,
        NativeIntegrityFail,
        ManagedIntegrityFail,
        AntiDebugDetected,
        LicenseTamper,
        AuditChainBroken,
    };

    struct SecurityEvent {
        SecurityEventType type;
        QString message;
        QHash<QString, QString> details;
        QDateTime timestamp;
    };

    // sinks must never throw
    class ISecurityEventSink {
    public:
        virtual ~ISecurityEventSink() = default;
        virtual void submit(const SecurityEvent& event) = 0;
    };

    // global event bus, call sites use emit_
    class SecurityEvents {
    public:
        // Replaces the active sink. nullptr installs a no-op sink.
        static void setSink(std::shared_ptr<ISecurityEventSink> sink);

        // Emits an event with an optional structured detail map.
        static void emit_(SecurityEventType type, const QString& message,
                          std::function<void(QHash<QString, QString>&)> detailsFiller = {});
    };
}
