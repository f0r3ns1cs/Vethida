#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QDateTime>
#include <QtCore/QString>

#include <functional>

namespace vth::services
{
    class DeviceIdentityService;

    // device-auth headers, plus the exact string that was signed (for logs/tests)
    struct SignedRequestHeaders {
        QString signature;
        QString pubkeyId;
        QString timestampMs;
        QString canonicalRequest;
    };

    // Signs METHOD\npath\nutc_ms\nsha256_hex(body) with the device key.
    // Path without host or query, hex lowercase. Must match the server exactly.
    class CanonicalRequestSigner
    {
    public:
        // Empty-body SHA-256, cached as lowercase hex.
        static constexpr const char* EmptyBodySha256Hex =
            "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";

        explicit CanonicalRequestSigner(DeviceIdentityService* identity);

        // Test constructor accepting a clock injector.
        CanonicalRequestSigner(DeviceIdentityService* identity, std::function<QDateTime()> clock);

        // Builds the canonical request string without signing it.
        static QString buildCanonicalRequest(const QString& method, const QString& path,
                                             qint64 timestampMs, const QByteArray& body);

        // Signs a request with the device's private key.
        SignedRequestHeaders sign(const QString& method, const QString& path,
                                  const QByteArray& body);

        // Lowercase hex SHA-256 of the request body.
        static QString computeBodyHash(const QByteArray& body);

    private:
        DeviceIdentityService* m_identity;
        std::function<QDateTime()> m_clock;

        static QString buildCanonicalRequestInternal(const QString& uppercaseMethod,
                                                     const QString& path,
                                                     qint64 timestampMs,
                                                     const QString& bodyHashHex);
    };
}
