#pragma once

#include "../Models/Api/ApiContracts.h"

#include <QtCore/QByteArray>
#include <QtCore/QFuture>

namespace vth::services
{
    // /v1/desktop/* calls used by LicenseService. Real one is LicenseApiService, the
    // interface is there so tests can fake it.
    class ILicenseApi
    {
    public:
        virtual ~ILicenseApi() = default;

        virtual QFuture<vth::models::api::ApiResponse<QByteArray>> activateAsync(
            const vth::models::api::LicenseActivateRequest& body) = 0;

        virtual QFuture<vth::models::api::ApiResponse<QByteArray>> heartbeatAsync(
            const vth::models::api::LicenseHeartbeatRequest& body) = 0;

        virtual QFuture<vth::models::api::ApiResponse<QByteArray>> deactivateAsync() = 0;

        // browser sign-in, start + poll
        virtual QFuture<vth::models::api::ApiResponse<QByteArray>> authStartAsync(
            const vth::models::api::DesktopAuthStartRequest& body) = 0;

        virtual QFuture<vth::models::api::ApiResponse<QByteArray>> authPollAsync(
            const vth::models::api::DesktopAuthPollRequest& body) = 0;
    };
}
