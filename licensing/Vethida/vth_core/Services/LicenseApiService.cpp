#include "LicenseApiService.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace vth::services
{
    LicenseApiService::LicenseApiService(ApiClient* client) : m_client(client) {}

    namespace
    {
        QJsonObject fingerprintToJson(const vth::models::api::HardwareFingerprintVector& fp)
        {
            QJsonObject o;
            o[QStringLiteral("motherboardUuid")] = fp.motherboardUuid;
            o[QStringLiteral("biosSerial")] = fp.biosSerial;
            o[QStringLiteral("cpuId")] = fp.cpuId;
            o[QStringLiteral("diskSerial")] = fp.diskSerial;
            o[QStringLiteral("volumeSerial")] = fp.volumeSerial;
            o[QStringLiteral("primaryMac")] = fp.primaryMac;
            return o;
        }
    }

    QFuture<vth::models::api::ApiResponse<QByteArray>> LicenseApiService::activateAsync(
        const vth::models::api::LicenseActivateRequest& req)
    {
        QJsonObject body;
        body[QStringLiteral("licenseKey")] = req.licenseKey;
        body[QStringLiteral("devicePublicKey")] = req.devicePublicKey;
        body[QStringLiteral("hardwareFingerprint")] = fingerprintToJson(req.hardwareFingerprint);
        return m_client->postAsync(QStringLiteral("/v1/desktop/activate"),
                                   QJsonDocument(body).toJson(QJsonDocument::Compact));
    }

    QFuture<vth::models::api::ApiResponse<QByteArray>> LicenseApiService::heartbeatAsync(
        const vth::models::api::LicenseHeartbeatRequest& req)
    {
        QJsonObject body;
        body[QStringLiteral("hardwareFingerprint")] = fingerprintToJson(req.hardwareFingerprint);
        return m_client->postAsync(QStringLiteral("/v1/desktop/heartbeat"),
                                   QJsonDocument(body).toJson(QJsonDocument::Compact));
    }

    QFuture<vth::models::api::ApiResponse<QByteArray>> LicenseApiService::deactivateAsync()
    {
        return m_client->postAsync(QStringLiteral("/v1/desktop/deactivate"),
                                   QByteArrayLiteral("{}"));
    }

    QFuture<vth::models::api::ApiResponse<QByteArray>> LicenseApiService::authStartAsync(
        const vth::models::api::DesktopAuthStartRequest& req)
    {
        QJsonObject body;
        body[QStringLiteral("devicePublicKey")] = req.devicePublicKey;
        body[QStringLiteral("hardwareFingerprint")] = fingerprintToJson(req.hardwareFingerprint);
        return m_client->postAsync(QStringLiteral("/v1/desktop/auth/start"),
                                   QJsonDocument(body).toJson(QJsonDocument::Compact));
    }

    QFuture<vth::models::api::ApiResponse<QByteArray>> LicenseApiService::authPollAsync(
        const vth::models::api::DesktopAuthPollRequest& req)
    {
        QJsonObject body;
        body[QStringLiteral("deviceCode")] = req.deviceCode;
        return m_client->postAsync(QStringLiteral("/v1/desktop/auth/poll"),
                                   QJsonDocument(body).toJson(QJsonDocument::Compact));
    }
}
