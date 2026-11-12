#pragma once

#include "../Models/LicenseModels.h"

#include <QtCore/QFuture>
#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QString>

class QTimer;

namespace vth::services
{
    class ILicenseApi;
    class DeviceIdentityService;

    // License state: activate / heartbeat / deactivate and the heartbeat timer.
    // Tokens go through vth::license::update (verify + cache + seal).
    // Lives on the UI thread, network calls run on the thread pool.
    class LicenseService : public QObject
    {
        Q_OBJECT
    public:
        LicenseService(ILicenseApi* api,
                       DeviceIdentityService* identity,
                       QObject* parent = nullptr);
        ~LicenseService() override;

        // Snapshot of the current license status. Mutex-guarded.
        vth::models::LicenseInfo currentInfo() const;

        // key masked, for the about panel / data export
        vth::models::LicenseInfo getSafeInfo() const;

        QFuture<vth::models::LicenseOpResult> activateAsync(const QString& key);

        // also used by the manual re-verify button
        QFuture<vth::models::LicenseOpResult> heartbeatAsync();

        // always wipes local state, result = whether the server freed the slot
        QFuture<vth::models::LicenseOpResult> deactivateAsync();

        // Browser sign-in: begin returns the URL + code, poll checks once (the UI
        // does the repeating). Ends up in the same state as a typed key, plus the email.
        struct SignInStart {
            bool    ok = false;
            vth::models::LicenseOpError error = vth::models::LicenseOpError::None;
            QString userCode;
            QString verificationUrl;
            QString deviceCode;
            int     pollIntervalMs = 2000;
            qint64  expiresAtUtcMs = 0;
        };
        QFuture<SignInStart> beginSignInAsync();

        struct SignInPoll {
            enum class State { Pending, Approved, Failed };
            State state = State::Pending;
            vth::models::LicenseOpError error = vth::models::LicenseOpError::None;
        };
        QFuture<SignInPoll> pollSignInAsync(const QString& deviceCode);

    Q_SIGNALS:
        void licenseStateChanged();

    private:
        ILicenseApi* m_api;
        DeviceIdentityService* m_identity;

        mutable QMutex m_lock;
        vth::models::LicenseInfo m_info;

        QTimer* m_heartbeatTimer = nullptr;
        // backoff in ms, 0 after a real answer. UI thread only.
        qint64  m_backoffMs = 0;

        // UI-thread only.
        void scheduleHeartbeat();
        void stopHeartbeat();
        // UI thread only
        void onHeartbeatSucceeded();
        void onHeartbeatFailedTransiently();
    };
}
