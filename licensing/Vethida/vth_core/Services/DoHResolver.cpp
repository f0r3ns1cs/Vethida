#include "DoHResolver.h"

#include "../Logger.h"

#include <QtConcurrent/QtConcurrent>
#include <QtCore/QEventLoop>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QMutexLocker>
#include <QtCore/QSet>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include <algorithm>
#include <limits>

namespace vth::services
{
    namespace
    {
        VTH_DEFINE_LOG_CATEGORY(catDoh, "vth.doh")

        struct DnsAnswer {
            QHostAddress address;
            int ttlSeconds = 0;
        };

        struct EndpointResult {
            DohEndpoint endpoint;
            std::vector<DnsAnswer> answers;
            bool error = false;
        };

        // blocking GET on the worker thread, empty on failure/timeout
        QByteArray fetchDohJson(const QUrl& url, std::chrono::milliseconds timeout,
                                int& httpStatusOut)
        {
            httpStatusOut = 0;
            QNetworkAccessManager http;
            QNetworkRequest req(url);
            req.setRawHeader("Accept", "application/dns-json");
            QNetworkReply* reply = http.get(req);
            if (reply == nullptr) return {};

            // hard timeout so a hung provider can't stall licensing
            QEventLoop loop;
            QTimer guard;
            guard.setSingleShot(true);
            QObject::connect(&guard, &QTimer::timeout, &loop, &QEventLoop::quit);
            QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            guard.start(static_cast<int>(timeout.count()));
            loop.exec();

            QByteArray body;
            if (reply->isFinished() && !guard.isActive()) {
                const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                httpStatusOut = status;
                if (reply->error() == QNetworkReply::NoError && status >= 200 && status < 300)
                    body = reply->readAll();
            }
            if (!reply->isFinished())
                reply->abort();
            reply->deleteLater();
            return body;
        }

        // application/dns-json (Cloudflare and Google), A/AAAA records only
        std::vector<DnsAnswer> parseDohJson(const QByteArray& body)
        {
            std::vector<DnsAnswer> out;
            if (body.isEmpty()) return out;
            const QJsonDocument doc = QJsonDocument::fromJson(body);
            if (!doc.isObject()) return out;
            const QJsonObject obj = doc.object();
            // Status 0 = NOERROR (RFC 1035). Anything else is rejected.
            if (obj.value(QStringLiteral("Status")).toInt(-1) != 0) return out;

            const QJsonArray answers = obj.value(QStringLiteral("Answer")).toArray();
            for (const QJsonValue& v : answers) {
                if (!v.isObject()) continue;
                const QJsonObject a = v.toObject();
                const int type = a.value(QStringLiteral("type")).toInt(0);
                // Type 1 = A, Type 28 = AAAA. CNAMEs are ignored.
                if (type != 1 && type != 28) continue;
                const QString data = a.value(QStringLiteral("data")).toString();
                if (data.isEmpty()) continue;
                QHostAddress ip;
                if (!ip.setAddress(data)) continue;
                DnsAnswer ans;
                ans.address = ip;
                ans.ttlSeconds = std::max(0, a.value(QStringLiteral("TTL")).toInt(0));
                out.push_back(ans);
            }
            return out;
        }

        QUrl buildEndpointUrl(const DohEndpoint& ep, const QString& hostname, const QString& type)
        {
            QString filled = ep.urlTemplate;
            filled.replace(QStringLiteral("{name}"), QString::fromLatin1(QUrl::toPercentEncoding(hostname)));
            filled.replace(QStringLiteral("{type}"), type);
            return QUrl(filled);
        }
    }

    std::vector<DohEndpoint> DoHResolver::defaultEndpoints()
    {
        return {
            DohEndpoint{QStringLiteral("Cloudflare"),
                        QStringLiteral("https://cloudflare-dns.com/dns-query?name={name}&type={type}")},
            DohEndpoint{QStringLiteral("Google"),
                        QStringLiteral("https://dns.google/resolve?name={name}&type={type}")},
        };
    }

    DoHResolver::DoHResolver(QObject* parent)
        : DoHResolver(defaultEndpoints(), parent)
    {
    }

    DoHResolver::DoHResolver(std::vector<DohEndpoint> endpoints, QObject* parent)
        : QObject(parent)
        , m_endpoints(std::move(endpoints))
    {
        if (m_endpoints.size() < 2) {
            // Cross-checking requires at least two independent providers.
            throw std::invalid_argument("DoHResolver requires at least two endpoints for cross-validation.");
        }
    }

    DoHResolver::~DoHResolver() = default;

    QFuture<DohResolutionResult> DoHResolver::resolveAsync(const QString& hostname)
    {
        return QtConcurrent::run([this, hostname]() -> DohResolutionResult {
            const QDateTime now = QDateTime::currentDateTimeUtc();

            // cache hit if not expired
            {
                QMutexLocker cacheLock(&m_cacheLock);
                if (m_cache.contains(hostname)) {
                    const auto cached = m_cache.value(hostname);
                    if (cached.expiresAt > now)
                        return cached;
                }
            }

            // A and AAAA from every provider, sequential is fine off the UI thread
            std::vector<EndpointResult> results;
            results.reserve(m_endpoints.size());

            for (const DohEndpoint& ep : m_endpoints) {
                EndpointResult r;
                r.endpoint = ep;
                int aStatus = 0, quadStatus = 0;
                const QByteArray aBody = fetchDohJson(
                    buildEndpointUrl(ep, hostname, QStringLiteral("A")),
                    std::chrono::milliseconds(QueryTimeout), aStatus);
                const QByteArray aaaaBody = fetchDohJson(
                    buildEndpointUrl(ep, hostname, QStringLiteral("AAAA")),
                    std::chrono::milliseconds(QueryTimeout), quadStatus);
                if (aBody.isEmpty() && aaaaBody.isEmpty()) {
                    r.error = true;
                } else {
                    auto aAns = parseDohJson(aBody);
                    auto aaaaAns = parseDohJson(aaaaBody);
                    r.answers.insert(r.answers.end(), aAns.begin(), aAns.end());
                    r.answers.insert(r.answers.end(), aaaaAns.begin(), aaaaAns.end());
                }
                results.push_back(std::move(r));
            }

            // Filter to providers that returned at least one address.
            std::vector<EndpointResult> healthy;
            healthy.reserve(results.size());
            for (auto& r : results) {
                if (!r.error && !r.answers.empty())
                    healthy.push_back(std::move(r));
            }

            if (healthy.empty()) {
                QStringList failedNames;
                for (const auto& r : results) failedNames << r.endpoint.displayName;
                throw DohResolutionException(
                    QStringLiteral("DoH resolution failed for %1: all providers errored (%2).")
                        .arg(hostname, failedNames.join(QStringLiteral(", "))));
            }

            // per-provider IP sets, TTL = smallest one seen
            std::vector<std::pair<DohEndpoint, QSet<QString>>> providerSets;
            QHash<QString, int> ipTtls;
            for (const auto& h : healthy) {
                QSet<QString> set;
                for (const auto& ans : h.answers) {
                    const QString key = ans.address.toString();
                    set.insert(key);
                    if (ipTtls.contains(key))
                        ipTtls[key] = std::min(ipTtls.value(key), ans.ttlSeconds);
                    else
                        ipTtls[key] = ans.ttlSeconds;
                }
                providerSets.emplace_back(h.endpoint, std::move(set));
            }

            QSet<QString> chosen;
            QString degradationReason;
            DohEndpoint chosenProvider = providerSets.front().first;

            if (healthy.size() >= 2) {
                // providers agree
                QSet<QString> intersection = providerSets[0].second;
                for (size_t i = 1; i < providerSets.size(); ++i)
                    intersection.intersect(providerSets[i].second);

                if (!intersection.isEmpty()) {
                    chosen = intersection;
                } else {
                    // they disagree, use the primary (the SPKI pin still protects us)
                    chosen = providerSets[0].second;
                    degradationReason = QStringLiteral("disagreement");
                }
            } else {
                // Exactly one healthy provider.
                chosen = providerSets[0].second;
                degradationReason = QStringLiteral("single_provider");
            }

            // log it, this is what DNS tampering would look like
            if (!degradationReason.isEmpty()) {
                QStringList answers;
                for (const auto& [ep, ips] : providerSets) {
                    QStringList sorted(ips.cbegin(), ips.cend());
                    std::sort(sorted.begin(), sorted.end());
                    answers << (ep.displayName + QLatin1Char('=')
                                + sorted.join(QLatin1Char(',')));
                }
                qCWarning(catDoh).noquote()
                    << "doh: degraded resolution for" << hostname
                    << "- reason" << degradationReason
                    << "- used" << chosenProvider.displayName
                    << "- answers" << answers.join(QStringLiteral("; "));
            }

            // clamp the TTL (max 1 hour)
            int minTtlSeconds = std::numeric_limits<int>::max();
            for (const QString& ip : chosen)
                minTtlSeconds = std::min(minTtlSeconds, ipTtls.value(ip, 0));
            if (minTtlSeconds == std::numeric_limits<int>::max()) minTtlSeconds = 0;
            if (minTtlSeconds < static_cast<int>(MinimumTtl.count()))
                minTtlSeconds = static_cast<int>(MinimumTtl.count());
            if (minTtlSeconds > static_cast<int>(MaximumTtl.count()))
                minTtlSeconds = static_cast<int>(MaximumTtl.count());

            // IPv4 first, then sorted
            std::vector<QHostAddress> ordered;
            ordered.reserve(static_cast<size_t>(chosen.size()));
            for (const QString& s : chosen) {
                QHostAddress h;
                if (h.setAddress(s)) ordered.push_back(h);
            }
            std::sort(ordered.begin(), ordered.end(), [](const QHostAddress& a, const QHostAddress& b) {
                const bool aV4 = (a.protocol() == QAbstractSocket::IPv4Protocol);
                const bool bV4 = (b.protocol() == QAbstractSocket::IPv4Protocol);
                if (aV4 != bV4) return aV4;
                return a.toString() < b.toString();
            });

            DohResolutionResult fresh;
            fresh.hostname = hostname;
            fresh.addresses = std::move(ordered);
            fresh.resolvedAt = QDateTime::currentDateTimeUtc();
            fresh.expiresAt = fresh.resolvedAt.addSecs(minTtlSeconds);

            {
                QMutexLocker cacheLock(&m_cacheLock);
                m_cache.insert(hostname, fresh);
            }
            return fresh;
        });
    }

    void DoHResolver::invalidate(const QString& hostname)
    {
        QMutexLocker cacheLock(&m_cacheLock);
        m_cache.remove(hostname);
    }
}
