#pragma once

#include "../Models/Api/ApiContracts.h"
#include "ApiClient.h"
#include "ILicenseApi.h"

#include <QtCore/QByteArray>
#include <QtCore/QFuture>

namespace vth::services
{
    // ILicenseApi over ApiClient (signing happens in ApiClient)
    class LicenseApiService final : public ILicenseApi
    {
    public:
        explicit LicenseApiService(ApiClient* client);

        QFuture<vth::models::api::ApiResponse<QByteArray>> activateAsync(
            const vth::models::api::LicenseActivateRequest& body) override;

        QFuture<vth::models::api::ApiResponse<QByteArray>> heartbeatAsync(
            const vth::models::api::LicenseHeartbeatRequest& body) override;

        QFuture<vth::models::api::ApiResponse<QByteArray>> deactivateAsync() override;

        QFuture<vth::models::api::ApiResponse<QByteArray>> authStartAsync(
            const vth::models::api::DesktopAuthStartRequest& body) override;

        QFuture<vth::models::api::ApiResponse<QByteArray>> authPollAsync(
            const vth::models::api::DesktopAuthPollRequest& body) override;

    private:
        ApiClient* m_client;
    };
}
