#pragma once

#include "../Models/Api/ApiContracts.h"

#include <QtCore/QByteArray>
#include <QtCore/QFuture>
#include <QtCore/QString>

class QNetworkAccessManager;

namespace vth::services
{
    class CanonicalRequestSigner;

    // Client for api.vethida.com. No shared secret, /v1/desktop/* requests are
    // signed with the device key (CanonicalRequestSigner). Use the QNAM from
    // ApiHttpClientFactory::createHardenedClient.
    class ApiClient
    {
    public:
        // Path prefix that triggers device-auth signing.
        static constexpr const char* DeviceAuthPathPrefix = "/v1/desktop/";

        ApiClient(QNetworkAccessManager* http, CanonicalRequestSigner* signer = nullptr);
        ~ApiClient();

        QFuture<vth::models::api::ApiResponse<QByteArray>> getAsync(const QString& path);
        QFuture<vth::models::api::ApiResponse<QByteArray>> postAsync(const QString& path, const QByteArray& body);
        QFuture<vth::models::api::ApiResponse<QByteArray>> putAsync(const QString& path, const QByteArray& body);
        QFuture<vth::models::api::ApiResponse<QByteArray>> deleteAsync(const QString& path);

        static bool pathRequiresDeviceAuth(const QString& path);

    private:
        QNetworkAccessManager* m_http;
        CanonicalRequestSigner* m_signer;
    };
}
