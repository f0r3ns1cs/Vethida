#include "Http.h"

#include "Runtime/Threading.h"

#include <QByteArray>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QString>
#include <QThread>
#include <QTimer>
#include <QUrl>
#include <QVariant>

namespace vth::http
{
    namespace
    {
        constexpr const char* kUserAgent = "Vethida/1.0";
        constexpr const char* kJsonContentType = "application/json";

        QNetworkAccessManager* threadLocalManager() {
            thread_local QNetworkAccessManager* manager = nullptr;
            if (!manager) {
                manager = new QNetworkAccessManager();
                manager->setAutoDeleteReplies(false);
                manager->setRedirectPolicy(QNetworkRequest::SameOriginRedirectPolicy);
            }
            return manager;
        }

        bool hasHeader(const std::vector<Header>& headers, const QString& name) {
            for (const auto& h : headers) {
                if (h.name.compare(name, Qt::CaseInsensitive) == 0) return true;
            }
            return false;
        }

        QNetworkRequest buildRequest(const QString& url,
            const std::vector<Header>& headers,
            bool isPost) {
            QNetworkRequest req((QUrl(url)));
            req.setHeader(QNetworkRequest::UserAgentHeader,
                QString::fromLatin1(kUserAgent));

            if (isPost && !hasHeader(headers, QStringLiteral("Content-Type"))) {
                req.setHeader(QNetworkRequest::ContentTypeHeader,
                    QString::fromLatin1(kJsonContentType));
            }

            req.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);

            for (const auto& h : headers) {
                req.setRawHeader(h.name.toUtf8(), h.value.toUtf8());
            }
            return req;
        }

        Response runRequest(QNetworkReply* reply,
            std::chrono::milliseconds timeout) {
            Response resp;
            if (!reply) {
                resp.errorString = QStringLiteral("network manager returned null reply");
                return resp;
            }

            QEventLoop loop;
            QTimer timer;
            timer.setSingleShot(true);
            QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
            QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            timer.start(static_cast<int>(timeout.count()));
            loop.exec();

            if (timer.isActive()) {
                timer.stop();
            }
            else {
                reply->abort();
                resp.errorString = QStringLiteral("request timed out after %1 ms")
                    .arg(timeout.count());
                reply->deleteLater();
                return resp;
            }

            resp.statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            resp.body = reply->readAll();
            for (const auto& pair : reply->rawHeaderPairs()) {
                resp.headers.push_back({ QString::fromUtf8(pair.first),
                                        QString::fromUtf8(pair.second) });
            }

            if (reply->error() != QNetworkReply::NoError) {
                resp.errorString = reply->errorString();
                resp.ok = false;
            }
            else {
                resp.ok = (resp.statusCode >= 200 && resp.statusCode < 300);
                if (!resp.ok) {
                    resp.errorString = QStringLiteral("HTTP %1").arg(resp.statusCode);
                }
            }

            reply->deleteLater();
            return resp;
        }
    }

    Response post(const QString& url,
        const QByteArray& body,
        const std::vector<Header>& headers,
        std::chrono::milliseconds timeout) noexcept {
        VTH_ASSERT_WORKER_THREAD();

        if (url.isEmpty()) {
            Response r;
            r.errorString = QStringLiteral("post called with empty url");
            return r;
        }

        QNetworkAccessManager* nam = threadLocalManager();
        QNetworkRequest req = buildRequest(url, headers, /*isPost=*/true);
        QNetworkReply* reply = nam->post(req, body);
        return runRequest(reply, timeout);
    }

    Response get(const QString& url,
        const std::vector<Header>& headers,
        std::chrono::milliseconds timeout) noexcept {
        VTH_ASSERT_WORKER_THREAD();

        if (url.isEmpty()) {
            Response r;
            r.errorString = QStringLiteral("get called with empty url");
            return r;
        }

        QNetworkAccessManager* nam = threadLocalManager();
        QNetworkRequest req = buildRequest(url, headers, /*isPost=*/false);
        QNetworkReply* reply = nam->get(req);
        return runRequest(reply, timeout);
    }

    Download download(const QString& url,
        const ByteSink& sink,
        const CancelFn& canceled,
        const ProgressFn& progress,
        std::chrono::milliseconds stallTimeout) noexcept {
        VTH_ASSERT_WORKER_THREAD();

        Download d;
        if (url.isEmpty()) {
            d.errorString = QStringLiteral("download called with empty url");
            return d;
        }
        if (!sink) {
            d.errorString = QStringLiteral("download called without a sink");
            return d;
        }

        QNetworkAccessManager* nam = threadLocalManager();
        QNetworkRequest req = buildRequest(url, {}, /*isPost=*/false);

        // Downloads redirect to a CDN, which the manager's same-origin policy refuses.
        req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
            QNetworkRequest::NoLessSafeRedirectPolicy);

        QNetworkReply* reply = nam->get(req);
        if (!reply) {
            d.errorString = QStringLiteral("network manager returned null reply");
            return d;
        }
        reply->setReadBufferSize(4 * 1024 * 1024);

        QEventLoop loop;
        bool stalled = false;
        bool aborted = false;
        bool refused = false;
        qint64 total = -1;

        QTimer stall;
        stall.setSingleShot(true);
        QObject::connect(&stall, &QTimer::timeout, &loop, [&]() {
            stalled = true;
            reply->abort();
        });

        QTimer poll;
        if (canceled) {
            QObject::connect(&poll, &QTimer::timeout, &loop, [&]() {
                if (!canceled()) return;
                aborted = true;
                reply->abort();
            });
            poll.start(100);
        }

        const auto drain = [&]() {
            const QByteArray chunk = reply->readAll();
            if (chunk.isEmpty()) return;
            if (!sink(chunk.constData(), chunk.size())) {
                refused = true;
                reply->abort();
                return;
            }
            d.bytes += chunk.size();
            if (progress) progress(d.bytes, total);
        };

        QObject::connect(reply, &QNetworkReply::readyRead, &loop, [&]() {
            stall.start(static_cast<int>(stallTimeout.count()));
            if (total < 0) {
                const QVariant len = reply->header(QNetworkRequest::ContentLengthHeader);
                if (len.isValid()) total = len.toLongLong();
            }
            drain();
        });
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

        stall.start(static_cast<int>(stallTimeout.count()));
        loop.exec();
        stall.stop();
        poll.stop();

        // Whatever finished() left buffered.
        if (!refused && !aborted && !stalled) drain();

        d.statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        d.canceled = aborted;
        d.sinkRefused = refused;

        if (aborted) {
            d.errorString = QStringLiteral("canceled");
        }
        else if (refused) {
            d.errorString = QStringLiteral("the sink refused a block");
        }
        else if (stalled) {
            d.errorString = QStringLiteral("no data for %1 ms").arg(stallTimeout.count());
        }
        else if (reply->error() != QNetworkReply::NoError) {
            d.errorString = reply->errorString();
        }
        else {
            d.ok = (d.statusCode >= 200 && d.statusCode < 300);
            if (!d.ok) d.errorString = QStringLiteral("HTTP %1").arg(d.statusCode);
        }

        reply->deleteLater();
        return d;
    }
}
