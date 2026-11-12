#include "CanonicalRequestSigner.h"

#include "DeviceIdentityService.h"

#include <QtCore/QCryptographicHash>

#include <stdexcept>
#include <utility>

namespace vth::services
{
    CanonicalRequestSigner::CanonicalRequestSigner(DeviceIdentityService* identity)
        : m_identity(identity)
        , m_clock([]() { return QDateTime::currentDateTimeUtc(); })
    {
        if (m_identity == nullptr)
            throw std::invalid_argument("CanonicalRequestSigner requires DeviceIdentityService");
    }

    CanonicalRequestSigner::CanonicalRequestSigner(DeviceIdentityService* identity,
                                                   std::function<QDateTime()> clock)
        : m_identity(identity), m_clock(std::move(clock))
    {
    }

    QString CanonicalRequestSigner::computeBodyHash(const QByteArray& body)
    {
        if (body.isEmpty())
            return QString::fromLatin1(EmptyBodySha256Hex);
        return QString::fromLatin1(
            QCryptographicHash::hash(body, QCryptographicHash::Sha256).toHex());
    }

    QString CanonicalRequestSigner::buildCanonicalRequestInternal(
        const QString& uppercaseMethod, const QString& path,
        qint64 timestampMs, const QString& bodyHashHex)
    {
        // must match the server's device-auth.ts byte for byte
        QString out;
        out.reserve(uppercaseMethod.length() + path.length() + 20 + bodyHashHex.length() + 3);
        out.append(uppercaseMethod);
        out.append(u'\n');
        out.append(path);
        out.append(u'\n');
        out.append(QString::number(timestampMs));
        out.append(u'\n');
        out.append(bodyHashHex);
        return out;
    }

    QString CanonicalRequestSigner::buildCanonicalRequest(
        const QString& method, const QString& path,
        qint64 timestampMs, const QByteArray& body)
    {
        return buildCanonicalRequestInternal(method.toUpper(), path, timestampMs, computeBodyHash(body));
    }

    SignedRequestHeaders CanonicalRequestSigner::sign(
        const QString& method, const QString& path, const QByteArray& body)
    {
        const qint64 timestampMs = m_clock().toMSecsSinceEpoch();
        const QString bodyHash = computeBodyHash(body);
        const QString canonical = buildCanonicalRequestInternal(method.toUpper(), path, timestampMs, bodyHash);

        SignedRequestHeaders h;
        h.signature = m_identity->signUtf8Base64Url(canonical);
        h.pubkeyId = m_identity->devicePubkeyId();
        h.timestampMs = QString::number(timestampMs);
        h.canonicalRequest = canonical;
        return h;
    }
}
