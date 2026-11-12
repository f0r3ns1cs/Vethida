#include "SentrySecurityEventSink.h"

#include "../SentryBridge.h"
#include "../../Runtime/Sentry.h"
#include "../../Runtime/ServiceRegistry.h"

#include <QtCore/QStringList>

namespace vth::services::observability
{
    namespace
    {
        using vth::services::SentryBridge;

        // names for the Sentry tags
        const char* eventTypeName(SecurityEventType t)
        {
            switch (t) {
                case SecurityEventType::SpkiMismatch:          return "SpkiMismatch";
                case SecurityEventType::TlsPinMatch:           return "TlsPinMatch";
                case SecurityEventType::TlsChainValidationFailed: return "TlsChainValidationFailed";
                case SecurityEventType::DohAllProvidersFailed: return "DohAllProvidersFailed";
                case SecurityEventType::DohDisagreement:       return "DohDisagreement";
                case SecurityEventType::DeviceAuthRejected:    return "DeviceAuthRejected";
                case SecurityEventType::JwsVerificationFailed: return "JwsVerificationFailed";
                case SecurityEventType::NativeIntegrityFail:   return "NativeIntegrityFail";
                case SecurityEventType::ManagedIntegrityFail:  return "ManagedIntegrityFail";
                case SecurityEventType::AntiDebugDetected:     return "AntiDebugDetected";
                case SecurityEventType::LicenseTamper:         return "LicenseTamper";
                case SecurityEventType::AuditChainBroken:      return "AuditChainBroken";
            }
            return "Unknown";
        }

        // full capture vs breadcrumb only
        bool shouldCapture(SecurityEventType type)
        {
            switch (type) {
                // Security signals: page immediately.
                case SecurityEventType::SpkiMismatch:
                case SecurityEventType::JwsVerificationFailed:
                case SecurityEventType::NativeIntegrityFail:
                case SecurityEventType::ManagedIntegrityFail:
                case SecurityEventType::DeviceAuthRejected:
                case SecurityEventType::DohAllProvidersFailed:
                case SecurityEventType::AntiDebugDetected:
                case SecurityEventType::LicenseTamper:
                case SecurityEventType::AuditChainBroken:
                    return true;

                // routine stuff, breadcrumb only
                case SecurityEventType::DohDisagreement:
                case SecurityEventType::TlsPinMatch:
                case SecurityEventType::TlsChainValidationFailed:
                    return false;
            }
            return false;
        }

        SentryBridge::Level captureLevelFor(SecurityEventType type)
        {
            switch (type) {
                case SecurityEventType::SpkiMismatch:
                case SecurityEventType::NativeIntegrityFail:
                case SecurityEventType::ManagedIntegrityFail:
                case SecurityEventType::AuditChainBroken:
                    return SentryBridge::Level::Fatal;

                case SecurityEventType::JwsVerificationFailed:
                case SecurityEventType::LicenseTamper:
                case SecurityEventType::AntiDebugDetected:
                    return SentryBridge::Level::Error;

                case SecurityEventType::DeviceAuthRejected:
                case SecurityEventType::DohAllProvidersFailed:
                case SecurityEventType::TlsChainValidationFailed:
                    return SentryBridge::Level::Warning;

                case SecurityEventType::DohDisagreement:
                case SecurityEventType::TlsPinMatch:
                    return SentryBridge::Level::Info;
            }
            return SentryBridge::Level::Info;
        }

        SentryBridge::Level breadcrumbLevelFor(SecurityEventType type)
        {
            switch (type) {
                case SecurityEventType::SpkiMismatch:
                case SecurityEventType::NativeIntegrityFail:
                case SecurityEventType::ManagedIntegrityFail:
                case SecurityEventType::JwsVerificationFailed:
                case SecurityEventType::LicenseTamper:
                case SecurityEventType::AuditChainBroken:
                case SecurityEventType::AntiDebugDetected:
                    return SentryBridge::Level::Error;

                case SecurityEventType::DeviceAuthRejected:
                case SecurityEventType::DohAllProvidersFailed:
                case SecurityEventType::DohDisagreement:
                case SecurityEventType::TlsChainValidationFailed:
                    return SentryBridge::Level::Warning;

                case SecurityEventType::TlsPinMatch:
                    return SentryBridge::Level::Info;
            }
            return SentryBridge::Level::Info;
        }

        QString formatMessage(const SecurityEvent& evt)
        {
            const char* typeName = eventTypeName(evt.type);
            if (evt.details.isEmpty()) {
                return QStringLiteral("[SecurityEvent:%1] %2")
                    .arg(QLatin1String(typeName), evt.message);
            }

            QStringList parts;
            parts.reserve(evt.details.size());
            for (auto it = evt.details.constBegin(); it != evt.details.constEnd(); ++it) {
                parts.append(QStringLiteral("%1=%2").arg(it.key(), it.value()));
            }
            return QStringLiteral("[SecurityEvent:%1] %2 [%3]")
                .arg(QLatin1String(typeName),
                     evt.message,
                     parts.join(u' '));
        }

        // uses the crash/diagnostics consent, not the usage one
        bool crashDiagnosticsConsented()
        {
            if (auto* s = vth::ServiceRegistry::instance().get<vth::sentry::Sentry>())
                return s->isCrashEnabled();
            return false;
        }
    }

    SentrySecurityEventSink::SentrySecurityEventSink() = default;
    SentrySecurityEventSink::~SentrySecurityEventSink() = default;

    void SentrySecurityEventSink::submit(const SecurityEvent& event)
    {
        if (!crashDiagnosticsConsented()) return;

        if (!vth::services::SentryBridge::isInitialized()) return;

        const QString message = formatMessage(event);
        const char* typeName = eventTypeName(event.type);
        const QString category = QStringLiteral("security.%1").arg(QLatin1String(typeName));

        // Breadcrumb always: cheap, attaches to whatever the SDK captures next.
        SentryBridge::addBreadcrumb(
            message,
            category,
            QStringLiteral("security"),
            breadcrumbLevelFor(event.type),
            event.details);

        if (!shouldCapture(event.type)) return;

        // event_type as a tag, details as extras (tags are limited to ~200 chars)
        QHash<QString, QString> tags;
        tags.insert(QStringLiteral("security.event_type"),
                    QString::fromLatin1(typeName));

        QHash<QString, QString> extras;
        extras.reserve(event.details.size());
        for (auto it = event.details.constBegin(); it != event.details.constEnd(); ++it) {
            extras.insert(QStringLiteral("detail.%1").arg(it.key()), it.value());
        }

        SentryBridge::captureMessage(
            message,
            captureLevelFor(event.type),
            tags,
            extras);
    }
}
