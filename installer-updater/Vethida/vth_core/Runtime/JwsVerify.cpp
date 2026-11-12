#include "JwsVerify.h"

#include "../Services/Signature.h"

#include <QtCore/QDateTime>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QList>

namespace vth::security
{
    namespace
    {
        namespace p256 = vth::services::p256;

        bool decodeSegment(const QByteArray& segment, QByteArray& out)
        {
            const auto result = QByteArray::fromBase64Encoding(
                segment,
                QByteArray::Base64UrlEncoding | QByteArray::AbortOnBase64DecodingErrors);
            if (!result)
                return false;
            out = *result;
            return true;
        }

        bool parseJson(const QByteArray& raw, QJsonObject& out)
        {
            QJsonParseError error{};
            const QJsonDocument doc = QJsonDocument::fromJson(raw, &error);
            if (error.error != QJsonParseError::NoError || !doc.isObject())
                return false;
            out = doc.object();
            return true;
        }

        bool audienceMatches(const QJsonValue& aud, const QString& expected)
        {
            if (aud.isString())
                return aud.toString() == expected;
            if (aud.isArray()) {
                const QJsonArray entries = aud.toArray();
                for (const QJsonValue& entry : entries)
                    if (entry.isString() && entry.toString() == expected)
                        return true;
            }
            return false;
        }

        TokenStatus checkClaims(const QJsonObject& claims,
                                const char* expectedIssuer,
                                const char* expectedAudience,
                                int clockSkewSeconds)
        {
            const qint64 now  = QDateTime::currentSecsSinceEpoch();
            const qint64 skew = static_cast<qint64>(clockSkewSeconds);

            if (const QJsonValue exp = claims.value(QStringLiteral("exp")); exp.isDouble()) {
                if (now > static_cast<qint64>(exp.toDouble()) + skew)
                    return TokenStatus::Expired;
            }
            if (const QJsonValue nbf = claims.value(QStringLiteral("nbf")); nbf.isDouble()) {
                if (now + skew < static_cast<qint64>(nbf.toDouble()))
                    return TokenStatus::NotYetValid;
            }

            if (expectedIssuer && *expectedIssuer) {
                const QJsonValue iss = claims.value(QStringLiteral("iss"));
                if (!iss.isString() || iss.toString() != QString::fromUtf8(expectedIssuer))
                    return TokenStatus::IssuerMismatch;
            }
            if (expectedAudience && *expectedAudience) {
                if (!audienceMatches(claims.value(QStringLiteral("aud")),
                                     QString::fromUtf8(expectedAudience)))
                    return TokenStatus::AudienceMismatch;
            }
            return TokenStatus::Valid;
        }

        TokenStatus verifyWithKey(const QByteArray& jws,
                                  const QByteArray& key,
                                  const char* expectedIssuer,
                                  const char* expectedAudience,
                                  int clockSkewSeconds) noexcept
        {
            if (key.size() != p256::PublicKeySize)
                return TokenStatus::KeyNotInstalled;

            const QList<QByteArray> parts = jws.split('.');
            if (parts.size() != 3)
                return TokenStatus::Malformed;

            QByteArray headerRaw;
            QByteArray payloadRaw;
            QByteArray signature;
            if (!decodeSegment(parts.at(0), headerRaw)
                || !decodeSegment(parts.at(1), payloadRaw)
                || !decodeSegment(parts.at(2), signature))
                return TokenStatus::Malformed;

            QJsonObject header;
            if (!parseJson(headerRaw, header))
                return TokenStatus::Malformed;
            // only ES256, never trust the alg from the header
            if (header.value(QStringLiteral("alg")).toString() != QLatin1String("ES256"))
                return TokenStatus::Malformed;
            if (signature.size() != p256::SignatureSize)
                return TokenStatus::Malformed;

            const QByteArray signingInput = parts.at(0) + '.' + parts.at(1);
            if (!p256::verify(key, signingInput, signature))
                return TokenStatus::SignatureMismatch;

            QJsonObject claims;
            if (!parseJson(payloadRaw, claims))
                return TokenStatus::Malformed;

            return checkClaims(claims, expectedIssuer, expectedAudience, clockSkewSeconds);
        }
    }

    TokenStatus verifyJwsWithRotation(const QByteArray& jws,
                                      const QByteArray& primaryKey,
                                      const QByteArray& backupKey,
                                      const char* expectedIssuer,
                                      const char* expectedAudience,
                                      int clockSkewSeconds) noexcept
    {
        const TokenStatus primary = verifyWithKey(
            jws, primaryKey, expectedIssuer, expectedAudience, clockSkewSeconds);
        if (primary != TokenStatus::SignatureMismatch)
            return primary;

        return verifyWithKey(
            jws, backupKey, expectedIssuer, expectedAudience, clockSkewSeconds);
    }
}
