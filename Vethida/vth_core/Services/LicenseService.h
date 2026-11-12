#pragma once

#include "../Models/LicenseModels.h"

#include <QtCore/QFuture>
#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QString>

#include <functional>

class QNetworkAccessManager;
class QTimer;

namespace vth::services
{
    // The license on this PC, checked against the licensing API.
    class LicenseService : public QObject
    {
        Q_OBJECT
    public:
        explicit LicenseService(QObject* parent = nullptr);
        ~LicenseService() override;

        // Loads the saved license, grants its tier until the first check
        // answers, and starts the hourly check. Call once at startup.
        void start();

        vth::models::LicenseInfo currentInfo() const { return m_info; }

        // True when the last check could not reach the licensing API. The plan is
        // not granted while this is set.
        bool isOffline() const { return m_offline; }

        QFuture<vth::models::LicenseOpResult> activateAsync(const QString& key);
        // Also the Account page's "Check now".
        QFuture<vth::models::LicenseOpResult> validateAsync();
        // Checks again when the last confirmation is older than the stale
        // window. Called when the app window is brought forward.
        void validateIfStale();

        // Activates the saved key on this PC again, for a key whose activation
        // here was removed (by support, or from another PC).
        QFuture<vth::models::LicenseOpResult> reactivateAsync();
        // Clears this PC's license whatever the API answers; the result
        // says whether the activation slot was freed there too.
        QFuture<vth::models::LicenseOpResult> deactivateAsync();

#ifdef VETHIDA_DEV_BUILD
        // Shows the app in a license state without a key, for screenshots: free, pro,
        // ultimate, expired, revoked, offline, unverified or notactive.
        void devSimulate(const QString& state);
#endif

    Q_SIGNALS:
        void licenseStateChanged();

    private:
        struct Reply {
            int         httpStatus = 0;     // 0 when nothing came back
            bool        networkError = false;
            QJsonObject body;
        };
        void post(const char* endpoint, const QList<QPair<QString, QString>>& form,
                  std::function<void(const Reply&)> done);

        void setOffline(bool offline);

        // Copies key, instance, plan and dates out of a successful answer.
        // False when the key is not for this store or none of its variants.
        bool absorb(const QJsonObject& body);
        void applyEntitlement();
        void load();
        void persist() const;
        void clearLocal();

        QNetworkAccessManager*   m_net   = nullptr;
        QTimer*                  m_timer = nullptr;
        // A sooner check after a failed one, so a PC that was offline gets its
        // plan back soon after the connection returns.
        QTimer*                  m_retry = nullptr;
        vth::models::LicenseInfo m_info;
        bool                     m_checking = false;
#ifdef VETHIDA_DEV_BUILD
        bool                     m_simulated = false;
#endif
        bool                     m_offline = false;
    };
}
