#pragma once

#include <QtCore/QDateTime>
#include <QtCore/QFuture>
#include <QtCore/QHash>
#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtNetwork/QHostAddress>

#include <chrono>
#include <stdexcept>
#include <vector>

namespace vth::services
{
    struct DohResolutionResult {
        QString hostname;
        std::vector<QHostAddress> addresses;
        QDateTime resolvedAt;
        QDateTime expiresAt;
    };

    struct DohEndpoint {
        QString displayName;
        QString urlTemplate;
    };

    // Thrown when every configured DoH provider failed.
    class DohResolutionException : public std::runtime_error {
    public:
        explicit DohResolutionException(const QString& message)
            : std::runtime_error(message.toStdString()) {}
    };

    // DNS-over-HTTPS for our own hosts. Asks two providers and prefers the answers
    // they agree on. Fails open (the SPKI pin is the real check).
    class DoHResolver : public QObject
    {
        Q_OBJECT
    public:
        // Floor/ceiling on the TTL we honor, and the per-query HTTP timeout.
        static constexpr std::chrono::seconds MinimumTtl{60};
        static constexpr std::chrono::seconds MaximumTtl{3600};
        static constexpr std::chrono::seconds QueryTimeout{5};

        // Default endpoints: Cloudflare and Google.
        static std::vector<DohEndpoint> defaultEndpoints();

        explicit DoHResolver(QObject* parent = nullptr);
        DoHResolver(std::vector<DohEndpoint> endpoints, QObject* parent = nullptr);
        ~DoHResolver() override;

        // cached until the TTL runs out
        QFuture<DohResolutionResult> resolveAsync(const QString& hostname);

        void invalidate(const QString& hostname);

    private:
        std::vector<DohEndpoint> m_endpoints;
        QMutex m_cacheLock;
        QHash<QString, DohResolutionResult> m_cache;
    };
}
