#pragma once

#include <QtCore/QJsonValue>
#include <QtCore/QString>
#include <optional>

namespace vth::models::api
{
    // {success, data, error}. Licensing calls get the raw body and parse it themselves.
    struct ApiError {
        QString code;
        QString message;

        // raw `details` from the server, e.g. { matchCount, delta } on a fingerprint error
        std::optional<QJsonValue> details;
    };

    template <class T>
    struct ApiResponse {
        bool success = false;
        std::optional<T> data;
        std::optional<ApiError> error;
    };

    // keys must match FINGERPRINT_SOURCES on the server. Empty = couldn't read it.
    struct HardwareFingerprintVector {
        QString motherboardUuid;
        QString biosSerial;
        QString cpuId;
        QString diskSerial;      // the drive's own firmware serial
        QString volumeSerial;    // the filesystem's, which a format changes
        QString primaryMac;
    };

    // POST /v1/desktop/activate (signed by the device key), returns the JWS token
    struct LicenseActivateRequest {
        QString licenseKey;
        QString devicePublicKey;   // base64url, 43 chars
        HardwareFingerprintVector hardwareFingerprint;
    };

    struct LicenseActivateResponse {
        QString licenseToken;
        QString tier;
        QString tokenExpiresAt;
        bool rebound = false;
        int resetsRemaining = 0;
        QString fingerprintStatus;   // "accept"
    };

    // POST /v1/desktop/heartbeat, returns a fresh token every time
    struct LicenseHeartbeatRequest {
        HardwareFingerprintVector hardwareFingerprint;
    };

    struct LicenseHeartbeatResponse {
        QString licenseToken;
        QString tier;
        QString tokenExpiresAt;
        int resetsRemaining = 0;
        QString fingerprintStatus;   // "accept" | "flag"
    };

    // POST /v1/desktop/deactivate. Frees the machine binding.
    struct LicenseDeactivateResponse {
        bool deactivated = false;
    };

    // POST /v1/desktop/auth/start, browser sign-in (RFC 8628 device flow). Signed
    // like activate.
    struct DesktopAuthStartRequest {
        QString devicePublicKey;   // base64url, 43 chars
        HardwareFingerprintVector hardwareFingerprint;
    };

    struct DesktopAuthStartResponse {
        // shown in the app and on the approval page so the user can compare
        QString userCode;
        // The bearer secret. Never displayed, never logged.
        QString deviceCode;
        QString verificationUrl;
        QString expiresAt;
        int pollIntervalMs = 2000;
    };

    // POST /v1/desktop/auth/poll, signed by the same key that started the session
    struct DesktopAuthPollRequest {
        QString deviceCode;
    };

    // "pending" until approved, then the same fields activate returns
    struct DesktopAuthPollResponse {
        QString status;   // "pending" | "approved"
        QString licenseToken;
        QString licenseKey;
        QString tier;
        QString tokenExpiresAt;
        int resetsRemaining = 0;
        QString fingerprintStatus;
        QString accountEmail;
    };

}
