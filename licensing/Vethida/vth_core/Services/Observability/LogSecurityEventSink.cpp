#include "LogSecurityEventSink.h"

#include "../../Logger.h"

#include <QtCore/QStringList>

namespace vth::services::observability
{
    namespace
    {
        VTH_DEFINE_LOG_CATEGORY(catSecEvent, "vth.security")

        const char* typeName(SecurityEventType t)
        {
            switch (t) {
                case SecurityEventType::SpkiMismatch:             return "SpkiMismatch";
                case SecurityEventType::TlsPinMatch:              return "TlsPinMatch";
                case SecurityEventType::TlsChainValidationFailed: return "TlsChainValidationFailed";
                case SecurityEventType::DohAllProvidersFailed:    return "DohAllProvidersFailed";
                case SecurityEventType::DohDisagreement:          return "DohDisagreement";
                case SecurityEventType::DeviceAuthRejected:       return "DeviceAuthRejected";
                case SecurityEventType::JwsVerificationFailed:    return "JwsVerificationFailed";
                case SecurityEventType::NativeIntegrityFail:      return "NativeIntegrityFail";
                case SecurityEventType::ManagedIntegrityFail:     return "ManagedIntegrityFail";
                case SecurityEventType::AntiDebugDetected:        return "AntiDebugDetected";
                case SecurityEventType::LicenseTamper:            return "LicenseTamper";
                case SecurityEventType::AuditChainBroken:         return "AuditChainBroken";
            }
            return "Unknown";
        }

        // Info for the routine success/soft signals; warning for everything else.
        bool isRoutine(SecurityEventType t)
        {
            return t == SecurityEventType::TlsPinMatch
                || t == SecurityEventType::DohDisagreement
                || t == SecurityEventType::TlsChainValidationFailed;
        }

        QString formatDetails(const QHash<QString, QString>& details)
        {
            if (details.isEmpty())
                return {};
            QStringList parts;
            parts.reserve(details.size());
            for (auto it = details.constBegin(); it != details.constEnd(); ++it)
                parts.append(QStringLiteral("%1=%2").arg(it.key(), it.value()));
            return QStringLiteral(" [%1]").arg(parts.join(u' '));
        }
    }

    void LogSecurityEventSink::submit(const SecurityEvent& event)
    {
        const QString line = QStringLiteral("[%1] %2%3")
            .arg(QLatin1String(typeName(event.type)),
                 event.message,
                 formatDetails(event.details));
        if (isRoutine(event.type))
            qCInfo(catSecEvent).noquote() << line;
        else
            qCWarning(catSecEvent).noquote() << line;
    }
}
