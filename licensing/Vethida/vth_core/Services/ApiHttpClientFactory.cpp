#include "ApiHttpClientFactory.h"

#include "../Logger.h"
#include "DoHResolver.h"
#include "Observability/SecurityEvents.h"
#include "SecurityConfig.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QFuture>
#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslConfiguration>
#include <QtNetwork/QSslError>
#include <QtNetwork/QSslKey>

#include <stdexcept>

namespace vth::services
{
    namespace
    {
        VTH_DEFINE_LOG_CATEGORY(catFactory, "vth.api.tls")

        // used by dashboards, don't rename
        const QLatin1String kValidateCategory("ApiHttpClientFactory.ValidateServerCertificate");
        const QLatin1String kTlsMismatchTag("security:tls_mismatch");
        const QLatin1String kTlsChainTag("security:tls_chain_validation");

        // SHA-256 of the leaf SPKI (QSslKey::toDer). Empty on failure = no match.
        QByteArray spkiSha256Hex(const QSslCertificate& cert)
        {
            if (cert.isNull()) return QByteArray();
            const QSslKey key = cert.publicKey();
            if (key.isNull()) return QByteArray();
            const QByteArray spkiDer = key.toDer();
            if (spkiDer.isEmpty()) return QByteArray();
            return QCryptographicHash::hash(spkiDer, QCryptographicHash::Sha256).toHex();
        }

        // callers skip this when no pins are configured (check-release-ready blocks that)
        bool leafMatchesPin(const QSslCertificate& leaf)
        {
            const QByteArray presentedHex = spkiSha256Hex(leaf);
            if (presentedHex.isEmpty()) return false;
            const QString presented = QString::fromLatin1(presentedHex);
            for (const QString& pin : SecurityConfig::apiSpkiPins()) {
                if (pin.compare(presented, Qt::CaseInsensitive) == 0)
                    return true;
            }
            return false;
        }

        bool isApiHost(const QString& host)
        {
            return host.compare(QStringLiteral("api.vethida.com"), Qt::CaseInsensitive) == 0
                || host.compare(QStringLiteral("api.vethida.com."), Qt::CaseInsensitive) == 0;
        }

        QString canonicalHost(const QString& host)
        {
            if (host.endsWith(u'.')) return host.left(host.size() - 1);
            return host;
        }

        void emitDohFailure(const QString& host, const QString& errorMessage)
        {
            observability::SecurityEvents::emit_(
                observability::SecurityEventType::DohAllProvidersFailed,
                QStringLiteral("All DoH providers failed resolving %1; falling back to OS DNS "
                               "(SPKI pin at TLS remains authoritative).").arg(host),
                [host, errorMessage](QHash<QString, QString>& d) {
                    d.insert(QStringLiteral("host"), host);
                    d.insert(QStringLiteral("error"), errorMessage);
                });
        }

        // Resolves the API host through DoH in createRequest and swaps the URL to the IP,
        // keeping the hostname for Host and cert verification. Falls back to OS DNS if
        // DoH fails, the SPKI pin is what actually protects us.
        class DoHRoutedNetworkAccessManager : public QNetworkAccessManager
        {
        public:
            DoHRoutedNetworkAccessManager(DoHResolver* doh, QObject* parent)
                : QNetworkAccessManager(parent), m_doh(doh) {}

        protected:
            QNetworkReply* createRequest(Operation op,
                                         const QNetworkRequest& req,
                                         QIODevice* outgoingData = nullptr) override
            {
                const QUrl url = req.url();
                const QString host = url.host();

                if (!isApiHost(host) || m_doh == nullptr)
                    return QNetworkAccessManager::createRequest(op, req, outgoingData);

                const QString origHost = canonicalHost(host);

                // short wait since this runs on the UI thread, a slow lookup keeps going
                // in the background and fills the cache
                constexpr int kResolveBudgetMs = 250;

                DohResolutionResult resolution;
                bool dohSucceeded = false;
                try {
                    QFuture<DohResolutionResult> fut = m_doh->resolveAsync(origHost);
                    QDeadlineTimer deadline(kResolveBudgetMs);
                    while (!fut.isFinished() && !deadline.hasExpired())
                        QThread::msleep(1);

                    if (!fut.isFinished()) {
                        qCInfo(catFactory).noquote()
                            << "DoH did not answer for" << origHost << "within"
                            << kResolveBudgetMs << "ms - using OS DNS for this request";
                        return QNetworkAccessManager::createRequest(op, req, outgoingData);
                    }

                    resolution = fut.result();
                    dohSucceeded = !resolution.addresses.empty();
                    if (!dohSucceeded)
                        emitDohFailure(origHost, QStringLiteral("DoH resolver returned no addresses."));
                } catch (const std::exception& ex) {
                    // all DoH providers failed, fall back to OS DNS
                    qCWarning(catFactory).noquote()
                        << "DoH resolve failed for" << origHost
                        << "- falling back to OS DNS:" << ex.what();
                    emitDohFailure(origHost, QString::fromUtf8(ex.what()));
                }

                if (!dohSucceeded) {
                    // OS DNS fallback: the peer-verify name is already correct.
                    return QNetworkAccessManager::createRequest(op, req, outgoingData);
                }

                // just the first IP
                const QHostAddress& ip = resolution.addresses.front();
                QUrl rewritten = url;
                rewritten.setHost(ip.toString());

                QNetworkRequest rebuilt = req;
                rebuilt.setUrl(rewritten);

                // keep the hostname for Host and the cert name check
                rebuilt.setRawHeader("Host", origHost.toUtf8());

                QSslConfiguration cfg = req.sslConfiguration();
                if (cfg == QSslConfiguration())
                    cfg = QSslConfiguration::defaultConfiguration();
                rebuilt.setSslConfiguration(cfg);
                rebuilt.setPeerVerifyName(origHost);

                return QNetworkAccessManager::createRequest(op, rebuilt, outgoingData);
            }

        private:
            DoHResolver* m_doh = nullptr;
        };
    }

    QNetworkAccessManager* ApiHttpClientFactory::createHardenedClient(
        DoHResolver* doh, std::chrono::milliseconds timeout, QObject* parent)
    {
        if (doh == nullptr)
            throw std::invalid_argument("ApiHttpClientFactory: DoHResolver is required.");

        auto* nam = new DoHRoutedNetworkAccessManager(doh, parent);
        nam->setTransferTimeout(static_cast<int>(timeout.count()));

        // TLS 1.2+
        QSslConfiguration cfg = QSslConfiguration::defaultConfiguration();
        cfg.setProtocol(QSsl::TlsV1_2OrLater);
        QSslConfiguration::setDefaultConfiguration(cfg);

        // A forgotten cert rotation would otherwise ship silently.
        if (!SecurityConfig::apiCertPinsArePopulated()) {
            qCWarning(catFactory).noquote()
                << "SPKI pins are not populated; release builds must set them.";
        }

        // sslErrors: chain failures are fatal (never ignored). encrypted: check the
        // SPKI pin before anything is sent.
        QObject::connect(nam, &QNetworkAccessManager::sslErrors, nam,
            [](QNetworkReply* reply, const QList<QSslError>& errors) {
                if (reply == nullptr) return;
                QString detail;
                for (const QSslError& e : errors)
                    detail += QStringLiteral("[") + e.errorString() + QStringLiteral("] ");
                qCWarning(catFactory).noquote()
                    << "TLS chain validation failed:" << detail.trimmed();

                // usually clock skew or a TLS-inspecting proxy, breadcrumb only
                const QString host = reply->url().host();
                observability::SecurityEvents::emit_(
                    observability::SecurityEventType::TlsChainValidationFailed,
                    QStringLiteral("TLS chain validation failed; connection not trusted."),
                    [host, detail](QHash<QString, QString>& d) {
                        d.insert(QStringLiteral("host"),             host);
                        d.insert(QStringLiteral("errors"),           detail.trimmed());
                        d.insert(QStringLiteral("category"),         kValidateCategory);
                        d.insert(QStringLiteral("securityAuditTag"), kTlsChainTag);
                    });
            });

        QObject::connect(nam, &QNetworkAccessManager::encrypted, nam,
            [](QNetworkReply* reply) {
                if (reply == nullptr) return;
                const QSslConfiguration replyCfg = reply->sslConfiguration();
                const QSslCertificate leaf = replyCfg.peerCertificate();
                const QString host = reply->url().host();

                // no pins = normal chain validation only (dev builds)
                if (!SecurityConfig::apiCertPinsArePopulated())
                    return;

                if (leaf.isNull() || !leafMatchesPin(leaf)) {
                    // no pin matched, abort before sending anything
                    const QString presentedSpki = QString::fromLatin1(spkiSha256Hex(leaf));
                    const QString subject = leaf.isNull()
                        ? QStringLiteral("<unavailable>")
                        : leaf.subjectInfo(QSslCertificate::CommonName).join(QChar::fromLatin1(','));
                    const QString issuer  = leaf.isNull()
                        ? QStringLiteral("<unavailable>")
                        : leaf.issuerInfo(QSslCertificate::CommonName).join(QChar::fromLatin1(','));
                    const int pinCount = SecurityConfig::apiSpkiPins().size();

                    qCWarning(catFactory).noquote()
                        << "SPKI pin mismatch; aborting request to" << host;

                    observability::SecurityEvents::emit_(
                        observability::SecurityEventType::SpkiMismatch,
                        QStringLiteral("TLS leaf certificate SPKI did not match any configured pin; "
                                       "connection refused."),
                        [presentedSpki, subject, issuer, host, pinCount](QHash<QString, QString>& d) {
                            d.insert(QStringLiteral("host"),             host);
                            d.insert(QStringLiteral("presentedSpki"),    presentedSpki);
                            d.insert(QStringLiteral("subject"),          subject);
                            d.insert(QStringLiteral("issuer"),           issuer);
                            d.insert(QStringLiteral("pinCount"),         QString::number(pinCount));
                            d.insert(QStringLiteral("category"),         kValidateCategory);
                            d.insert(QStringLiteral("securityAuditTag"), kTlsMismatchTag);
                        });
                    reply->abort();
                    return;
                }

                // pin matched (breadcrumb for the match ratio)
                observability::SecurityEvents::emit_(
                    observability::SecurityEventType::TlsPinMatch,
                    QStringLiteral("TLS leaf SPKI matched a configured pin."),
                    [host](QHash<QString, QString>& d) {
                        d.insert(QStringLiteral("host"),     host);
                        d.insert(QStringLiteral("category"), kValidateCategory);
                    });
            });

        return nam;
    }
}
