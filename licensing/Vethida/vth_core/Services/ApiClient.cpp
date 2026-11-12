#include "ApiClient.h"

#include "CanonicalRequestSigner.h"
#include "SecurityConfig.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QMetaObject>
#include <QtCore/QPointer>
#include <QtCore/QPromise>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include <memory>

namespace vth::services
{
    using vth::models::api::ApiError;
    using vth::models::api::ApiResponse;

    ApiClient::ApiClient(QNetworkAccessManager* http, CanonicalRequestSigner* signer)
        : m_http(http), m_signer(signer)
    {
    }

    ApiClient::~ApiClient() = default;

    bool ApiClient::pathRequiresDeviceAuth(const QString& path)
    {
        if (path.isEmpty())
            return false;
        return path.startsWith(QString::fromLatin1(DeviceAuthPathPrefix), Qt::CaseInsensitive);
    }

    namespace
    {
        ApiResponse<QByteArray> errorResponse(const QString& code, const QString& message)
        {
            ApiResponse<QByteArray> r;
            r.success = false;
            r.error = ApiError{code, message, std::nullopt};
            return r;
        }

        // { "error": { code, message, details? } }, or http_<status> for anything else
        ApiError parseErrorBody(const QByteArray& body, int status)
        {
            const QString fallbackCode =
                QStringLiteral("http_") + QString::number(status);
            const QString fallbackMessage =
                QStringLiteral("Server returned ") + QString::number(status);

            QJsonParseError parseError{};
            const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
            if (parseError.error != QJsonParseError::NoError || !doc.isObject())
                return ApiError{fallbackCode, fallbackMessage, std::nullopt};

            const QJsonValue errorValue = doc.object().value(QStringLiteral("error"));
            if (!errorValue.isObject())
                return ApiError{fallbackCode, fallbackMessage, std::nullopt};

            const QJsonObject errorObject = errorValue.toObject();
            const QString code = errorObject.value(QStringLiteral("code")).toString();
            if (code.isEmpty())
                return ApiError{fallbackCode, fallbackMessage, std::nullopt};

            ApiError out;
            out.code = code;
            out.message = errorObject.value(QStringLiteral("message"))
                              .toString(fallbackMessage);
            if (const QJsonValue details = errorObject.value(QStringLiteral("details"));
                !details.isUndefined()) {
                out.details = details;
            }
            return out;
        }

        QFuture<ApiResponse<QByteArray>> sendRequest(
            QNetworkAccessManager* http,
            CanonicalRequestSigner* signer,
            const QString& method,
            const QString& path,
            const QByteArray& body)
        {
            auto promise = std::make_shared<QPromise<ApiResponse<QByteArray>>>();
            promise->start();
            QFuture<ApiResponse<QByteArray>> future = promise->future();
            QPointer<QNetworkAccessManager> manager(http);

            const bool queued = QMetaObject::invokeMethod(http, [=]() mutable {
                if (manager.isNull()) {
                    promise->addResult(errorResponse(QStringLiteral("network_unavailable"),
                        QStringLiteral("Network service is unavailable.")));
                    promise->finish();
                    return;
                }

                const QUrl url(QString::fromLatin1(SecurityConfig::ApiBaseUrl) + path);
                QNetworkRequest req(url);
                req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
                req.setRawHeader("Accept", "application/json");

                // Device-auth signing for /v1/desktop/* paths.
                if (signer != nullptr && ApiClient::pathRequiresDeviceAuth(path)) {
                    const auto signed_ = signer->sign(method, url.path(), body);
                    req.setRawHeader("X-Device-Signature", signed_.signature.toUtf8());
                    req.setRawHeader("X-Device-Pubkey-Id", signed_.pubkeyId.toUtf8());
                    req.setRawHeader("X-Request-Timestamp", signed_.timestampMs.toUtf8());
                }

                QNetworkReply* reply = nullptr;
                const QByteArray verb = method.toUtf8();
                if (verb == "GET") {
                    reply = manager->get(req);
                } else if (verb == "POST") {
                    reply = manager->post(req, body);
                } else if (verb == "PUT") {
                    reply = manager->put(req, body);
                } else if (verb == "DELETE") {
                    reply = manager->deleteResource(req);
                } else {
                    promise->addResult(errorResponse(QStringLiteral("unsupported_method"), method));
                    promise->finish();
                    return;
                }
                if (reply == nullptr) {
                    promise->addResult(errorResponse(QStringLiteral("network_unavailable"),
                        QStringLiteral("Network request could not be started.")));
                    promise->finish();
                    return;
                }
                QObject::connect(reply, &QNetworkReply::finished, reply, [reply, promise]() mutable {
                    const QByteArray responseBody = reply->readAll();
                    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                    const QNetworkReply::NetworkError networkError = reply->error();
                    const QString networkErrorText = reply->errorString();

                    ApiResponse<QByteArray> response;
                    if (networkError != QNetworkReply::NoError && status == 0) {
                        response = errorResponse(QStringLiteral("network_error"), networkErrorText);
                    } else {
                        response.success = status >= 200 && status < 300;
                        if (response.success) {
                            response.data = responseBody;
                        } else {
                            // the real code is in the body
                            response.error = parseErrorBody(responseBody, status);
                        }
                    }
                    reply->deleteLater();
                    promise->addResult(std::move(response));
                    promise->finish();
                });
            }, Qt::QueuedConnection);

            if (!queued) {
                promise->addResult(errorResponse(QStringLiteral("network_unavailable"),
                    QStringLiteral("Network service is unavailable.")));
                promise->finish();
            }
            return future;
        }
    }

    QFuture<ApiResponse<QByteArray>> ApiClient::getAsync(const QString& path)
    {
        return sendRequest(m_http, m_signer, QStringLiteral("GET"), path, {});
    }

    QFuture<ApiResponse<QByteArray>> ApiClient::postAsync(const QString& path, const QByteArray& body)
    {
        return sendRequest(m_http, m_signer, QStringLiteral("POST"), path, body);
    }

    QFuture<ApiResponse<QByteArray>> ApiClient::putAsync(const QString& path, const QByteArray& body)
    {
        return sendRequest(m_http, m_signer, QStringLiteral("PUT"), path, body);
    }

    QFuture<ApiResponse<QByteArray>> ApiClient::deleteAsync(const QString& path)
    {
        return sendRequest(m_http, m_signer, QStringLiteral("DELETE"), path, {});
    }
}
