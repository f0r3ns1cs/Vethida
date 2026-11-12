#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

class QLineEdit;
class QTimer;

namespace vth::services { class LicenseService; }

namespace vth::ui
{
    // Activation logic shared by ActivationWindow and the in-app modal: key submit,
    // browser sign-in + polling, error messages.
    class ActivationController final : public QObject
    {
        Q_OBJECT
    public:
        explicit ActivationController(vth::services::LicenseService& service,
                                      QObject* parent = nullptr);

        Q_PROPERTY(bool activated READ isActivated NOTIFY activated)
        Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)
        [[nodiscard]] bool isActivated() const { return m_activated; }
        [[nodiscard]] bool isBusy() const { return m_busy; }

        // Where a customer finds the key they bought.
        Q_PROPERTY(QString portalUrl READ portalUrl CONSTANT)
        [[nodiscard]] QString portalUrl() const;
        Q_INVOKABLE void openPortal() const;

        // Whether the text as typed is a whole key.
        Q_INVOKABLE static bool keyIsSubmittable(const QString& key);

        // Strips separators, upper-cases and regroups in fives.
        Q_INVOKABLE static QString formatKey(const QString& raw);

        // formats the key as you type
        static void wireKeyField(QLineEdit* field);

        // Five groups of five plus the four separators, for the field's cap.
        Q_INVOKABLE static int keyFieldLength();

        // The receipt text, for a dialog that shows one.
        Q_PROPERTY(QString successTitle READ successTitle NOTIFY activated)
        Q_PROPERTY(QString successBody READ successBody NOTIFY activated)
        [[nodiscard]] QString successTitle() const;
        [[nodiscard]] QString successBody() const;

    public Q_SLOTS:
        // bad format fails locally (saves a rate-limited request)
        void submitKey(const QString& key);
        void beginSignIn();
        // local only, the server session just expires (no cancel endpoint on purpose)
        void cancelSignIn();
        void reopenSignInPage();

    Q_SIGNALS:
        // A request is in flight. Surfaces disable their controls on true.
        void busyChanged(bool busy);
        // The attempt is over and the customer has to do something about it.
        void failed(const QString& message);
        // The sign-in is still live and this is only a notice about it.
        void notice(const QString& message);
        void noticeCleared();
        // The browser half has begun: show the code and the way to abandon it.
        void signInStarted(const QString& userCode);
        // Back to the form, whether canceled, expired or refused.
        void signInEnded();
        void activated();

    private:
        void setBusy(bool busy);
        void poll();
        void stopPolling();

        vth::services::LicenseService& m_service;
        QTimer* m_pollTimer = nullptr;
        QString m_deviceCode;
        QUrl    m_verificationUrl;
        qint64  m_signInDeadlineMs = 0;
        bool    m_pollInFlight = false;
        bool    m_busy = false;
        bool    m_activated = false;
    };
}
