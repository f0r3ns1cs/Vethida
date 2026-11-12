#include "pch.h"
#include "ActivationController.h"

#include "../../vth_core/Models/LicenseModels.h"
#include "../../vth_core/Models/ProductTier.h"
#include "../../vth_core/Services/LicenseService.h"
#include "../../vth_core/SiteUrls.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QFutureWatcher>
#include <QLineEdit>
#include <QTimer>

namespace vth::ui
{
    namespace
    {
        // Five groups of five, plus the four separators between them.
        constexpr int kKeyCharacters = 25;
        constexpr int kKeyGroup = 5;
        constexpr int kKeyFieldLength = kKeyCharacters + (kKeyCharacters / kKeyGroup) - 1;
    }

    ActivationController::ActivationController(vth::services::LicenseService& service,
                                               QObject* parent)
        : QObject(parent)
        , m_service(service)
        , m_pollTimer(new QTimer(this))
    {
        connect(m_pollTimer, &QTimer::timeout, this, &ActivationController::poll);
    }

    bool ActivationController::keyIsSubmittable(const QString& key)
    {
        return vth::models::LicenseValidation::isValidKeyFormat(key);
    }

    QString ActivationController::formatKey(const QString& raw)
    {
        // normalize to XXXXX-XXXXX-... however it was typed or pasted
        QString compact;
        for (const QChar c : raw)
            if (c.isLetterOrNumber())
                compact.append(c.toUpper());
        compact.truncate(kKeyCharacters);

        QString grouped;
        for (int i = 0; i < compact.size(); ++i) {
            if (i > 0 && i % kKeyGroup == 0)
                grouped.append(QLatin1Char('-'));
            grouped.append(compact.at(i));
        }
        return grouped;
    }

    void ActivationController::wireKeyField(QLineEdit* field)
    {
        if (!field)
            return;

        field->setMaxLength(kKeyFieldLength);

        // textEdited, because setText below would re-enter textChanged.
        QObject::connect(field, &QLineEdit::textEdited, field, [field](const QString& typed) {
            // The caret position, counted in key characters.
            const int caret = field->cursorPosition();
            int keyCharsBefore = 0;
            for (int i = 0; i < caret && i < typed.size(); ++i)
                if (typed.at(i).isLetterOrNumber())
                    ++keyCharsBefore;

            const QString grouped = formatKey(typed);
            if (grouped == typed)
                return;
            field->setText(grouped);

            // put the caret back after the same number of key characters
            int pos = 0;
            int seen = 0;
            while (pos < grouped.size() && seen < keyCharsBefore) {
                if (grouped.at(pos).isLetterOrNumber())
                    ++seen;
                ++pos;
            }
            field->setCursorPosition(pos);
        });
    }

    int ActivationController::keyFieldLength()
    {
        return kKeyFieldLength;
    }

    QString ActivationController::portalUrl() const
    {
        return QString(vth::site::kPortalLicenses);
    }

    void ActivationController::openPortal() const
    {
        QDesktopServices::openUrl(QUrl(portalUrl()));
    }

    QString ActivationController::successTitle() const
    {
        const vth::models::LicenseInfo now = m_service.currentInfo();
        return tr("%1 is active").arg(
            vth::models::tierProductName(vth::models::tierFromString(now.tier)));
    }

    QString ActivationController::successBody() const
    {
        const vth::models::LicenseInfo now = m_service.currentInfo();
        // no account email for a typed key
        return now.accountEmail.isEmpty()
            ? tr("This PC is licensed and everything in your plan is unlocked.")
            : tr("Signed in as %1. This PC is licensed and everything in your "
                 "plan is unlocked.").arg(now.accountEmail);
    }

    void ActivationController::submitKey(const QString& key)
    {
        if (m_busy)
            return;
        if (!keyIsSubmittable(key)) {
            Q_EMIT failed(tr("That key doesn't look right. A Vethida key is five "
                             "groups of five characters."));
            return;
        }

        setBusy(true);

        using vth::models::LicenseOpResult;
        auto* watcher = new QFutureWatcher<LicenseOpResult>(this);
        connect(watcher, &QFutureWatcher<LicenseOpResult>::finished, this,
                [this, watcher]() {
            const LicenseOpResult result = watcher->result();
            watcher->deleteLater();
            setBusy(false);

            if (result.ok) {
                m_activated = true;
                Q_EMIT activated();
                return;
            }
            Q_EMIT failed(vth::models::licenseErrorLabel(result.error));
        });
        watcher->setFuture(m_service.activateAsync(key));
    }

    void ActivationController::beginSignIn()
    {
        if (m_busy)
            return;

        Q_EMIT noticeCleared();
        setBusy(true);

        using Start = vth::services::LicenseService::SignInStart;
        auto* watcher = new QFutureWatcher<Start>(this);
        connect(watcher, &QFutureWatcher<Start>::finished, this, [this, watcher]() {
            const Start start = watcher->result();
            watcher->deleteLater();
            setBusy(false);

            if (!start.ok) {
                Q_EMIT failed(vth::models::licenseErrorLabel(start.error));
                return;
            }

            m_deviceCode = start.deviceCode;
            m_verificationUrl = QUrl(start.verificationUrl);
            m_signInDeadlineMs = start.expiresAtUtcMs;

            Q_EMIT signInStarted(start.userCode);
            QDesktopServices::openUrl(m_verificationUrl);

            // interval comes from the server
            m_pollTimer->setInterval(qMax(1000, start.pollIntervalMs));
            m_pollTimer->start();
        });
        watcher->setFuture(m_service.beginSignInAsync());
    }

    void ActivationController::cancelSignIn()
    {
        stopPolling();
        Q_EMIT signInEnded();
    }

    void ActivationController::reopenSignInPage()
    {
        if (!m_verificationUrl.isEmpty())
            QDesktopServices::openUrl(m_verificationUrl);
    }

    void ActivationController::poll()
    {
        if (m_pollInFlight)
            return;   // a slow answer must not stack a second request behind it
        if (m_signInDeadlineMs > 0
            && QDateTime::currentMSecsSinceEpoch() > m_signInDeadlineMs) {
            stopPolling();
            Q_EMIT signInEnded();
            Q_EMIT failed(vth::models::licenseErrorLabel(
                vth::models::LicenseOpError::SignInExpired));
            return;
        }

        m_pollInFlight = true;
        using Poll = vth::services::LicenseService::SignInPoll;
        auto* watcher = new QFutureWatcher<Poll>(this);
        connect(watcher, &QFutureWatcher<Poll>::finished, this, [this, watcher]() {
            const Poll result = watcher->result();
            watcher->deleteLater();
            m_pollInFlight = false;

            switch (result.state) {
            case Poll::State::Approved:
                stopPolling();
                m_activated = true;
                Q_EMIT activated();
                return;
            case Poll::State::Failed:
                // keep polling through network errors, everything else ends it
                if (result.error == vth::models::LicenseOpError::Network) {
                    Q_EMIT notice(tr("Lost contact with the licensing server. Still trying."));
                    return;
                }
                stopPolling();
                Q_EMIT signInEnded();
                Q_EMIT failed(vth::models::licenseErrorLabel(result.error));
                return;
            case Poll::State::Pending:
                Q_EMIT noticeCleared();
                return;
            }
        });
        watcher->setFuture(m_service.pollSignInAsync(m_deviceCode));
    }

    void ActivationController::stopPolling()
    {
        m_pollTimer->stop();
        m_pollInFlight = false;
    }

    void ActivationController::setBusy(bool busy)
    {
        if (m_busy == busy)
            return;
        m_busy = busy;
        Q_EMIT busyChanged(busy);
    }
}
