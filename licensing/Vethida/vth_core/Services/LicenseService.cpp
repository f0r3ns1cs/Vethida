#include "LicenseService.h"

#include "../Logger.h"
#include "../Runtime/LicenseCache.h"
#include "DeviceIdentityService.h"
#include "HardwareFingerprint.h"
#include "ILicenseApi.h"
#include "LicenseStore.h"
#include "LicenseTokenCache.h"
#include "SecurityConfig.h"

#include <QtConcurrent/QtConcurrent>
#include <QtCore/QByteArray>
#include <QtCore/QDateTime>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QMetaObject>
#include <QtCore/QMutexLocker>
#include <QtCore/QTimer>

namespace vth::services
{
    namespace
    {
        VTH_DEFINE_LOG_CATEGORY(catLicenseSvc, "vth.license.service")

        // collect + hash every field. Empty fields are fine.
        void collectFingerprint(vth::models::api::HardwareFingerprintVector& out)
        {
            HardwareFingerprintCollector collector;
            const HardwareFingerprintVectorPathC fp = hashFingerprint(collector.collect());
            out.motherboardUuid = fp.motherboardUuid.value_or(QString());
            out.biosSerial = fp.biosSerial.value_or(QString());
            out.cpuId = fp.cpuId.value_or(QString());
            out.diskSerial = fp.diskSerial.value_or(QString());
            out.volumeSerial = fp.volumeSerial.value_or(QString());
            out.primaryMac = fp.primaryMac.value_or(QString());
        }
    }

    LicenseService::LicenseService(ILicenseApi* api,
                                   DeviceIdentityService* identity,
                                   QObject* parent)
        : QObject(parent)
        , m_api(api), m_identity(identity)
    {
        m_info = LicenseStore::load();
        scheduleHeartbeat();
    }

    LicenseService::~LicenseService() = default;

    vth::models::LicenseInfo LicenseService::currentInfo() const
    {
        QMutexLocker lock(&m_lock);
        return m_info;
    }

    vth::models::LicenseInfo LicenseService::getSafeInfo() const
    {
        QMutexLocker lock(&m_lock);
        auto safe = m_info;
        if (!safe.key.isEmpty())
            safe.key = vth::models::LicenseValidation::maskKey(safe.key);
        return safe;
    }

    // shared by activate and heartbeat so they map errors the same way
    static vth::models::LicenseOpResult failureFrom(
        const vth::models::api::ApiResponse<QByteArray>& resp)
    {
        using vth::models::LicenseOpError;
        using vth::models::LicenseOpResult;

        if (!resp.error.has_value())
            return LicenseOpResult::failure(LicenseOpError::Unknown);

        const QString code = resp.error->code;
        const LicenseOpError mapped = vth::models::licenseErrorFromCode(code);
        if (mapped == LicenseOpError::Unknown && !code.isEmpty())
            qCWarning(catLicenseSvc) << "unmapped license error code:" << code;
        return LicenseOpResult::failure(mapped, code);
    }

    QFuture<vth::models::LicenseOpResult> LicenseService::activateAsync(const QString& key)
    {
        using vth::models::LicenseOpError;
        using vth::models::LicenseOpResult;

        return QtConcurrent::run([this, key]() -> LicenseOpResult {
            try {
                // check locally first, the server only allows 10 tries / 15 min
                if (!vth::models::LicenseValidation::isValidKeyFormat(key))
                    return LicenseOpResult::failure(LicenseOpError::KeyFormat);

                // bind here too, a Free install has no device key at boot
                vth::license::bindToDevice(m_identity->devicePubkeyId());

                vth::models::api::LicenseActivateRequest req;
                req.licenseKey = key;
                req.devicePublicKey = m_identity->publicKeyBase64Url();
                collectFingerprint(req.hardwareFingerprint);

                const auto resp = m_api->activateAsync(req).result();
                if (!resp.success || !resp.data.has_value())
                    return failureFrom(resp);

                const auto obj = QJsonDocument::fromJson(*resp.data).object();
                const QString token = obj.value(QStringLiteral("licenseToken")).toString();
                // verify + cache + seal. A 2xx with a bad token is TokenRejected.
                if (!vth::license::update(token))
                    return LicenseOpResult::failure(LicenseOpError::TokenRejected);

                {
                    QMutexLocker lock(&m_lock);
                    m_info.key = key;
                    m_info.status = vth::models::LicenseStatus::Valid;
                    m_info.tier = obj.value(QStringLiteral("tier")).toString(QString());
                    m_info.trial = obj.value(QStringLiteral("trial")).toBool(false);
                    // licenseExpiresAt (null for lifetime -> empty), not tokenExpiresAt
                    m_info.expiresAt = obj.value(QStringLiteral("licenseExpiresAt")).toString();
                    m_info.activatedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
                    m_info.lastVerifiedUtcMs = QDateTime::currentMSecsSinceEpoch();
                    m_info.devicePubkeyId = m_identity->devicePubkeyId();
                    m_info.fingerprintStatus = obj.value(QStringLiteral("fingerprintStatus")).toString();
                    m_info.resetsRemaining = obj.value(QStringLiteral("resetsRemaining")).toInt();
                    // activate doesn't send this, the next heartbeat will
                    m_info.accountDeleting = false;
                    m_info.accountHardDeleteAt.clear();
                    LicenseStore::save(m_info);
                }
                QMetaObject::invokeMethod(this, [this]() { scheduleHeartbeat(); },
                                          Qt::QueuedConnection);
                Q_EMIT licenseStateChanged();
                return LicenseOpResult::success();
            } catch (const std::exception& ex) {
                qCWarning(catLicenseSvc) << "activate failed:" << ex.what();
                return LicenseOpResult::failure(LicenseOpError::Unknown);
            }
        });
    }

    QFuture<LicenseService::SignInStart> LicenseService::beginSignInAsync()
    {
        using vth::models::LicenseOpError;

        return QtConcurrent::run([this]() -> SignInStart {
            SignInStart out;
            try {
                vth::models::api::DesktopAuthStartRequest req;
                req.devicePublicKey = m_identity->publicKeyBase64Url();
                collectFingerprint(req.hardwareFingerprint);

                const auto resp = m_api->authStartAsync(req).result();
                if (!resp.success || !resp.data.has_value()) {
                    out.error = failureFrom(resp).error;
                    return out;
                }

                const auto obj = QJsonDocument::fromJson(*resp.data).object();
                out.userCode = obj.value(QStringLiteral("userCode")).toString();
                out.deviceCode = obj.value(QStringLiteral("deviceCode")).toString();
                out.verificationUrl =
                    obj.value(QStringLiteral("verificationUrl")).toString();
                out.pollIntervalMs =
                    obj.value(QStringLiteral("pollIntervalMs")).toInt(2000);
                const QDateTime expires = QDateTime::fromString(
                    obj.value(QStringLiteral("expiresAt")).toString(), Qt::ISODate);
                out.expiresAtUtcMs = expires.isValid() ? expires.toMSecsSinceEpoch() : 0;

                // need both
                if (out.deviceCode.isEmpty() || out.verificationUrl.isEmpty()) {
                    out.error = LicenseOpError::Server;
                    return out;
                }
                out.ok = true;
                return out;
            } catch (const std::exception& ex) {
                qCWarning(catLicenseSvc) << "sign-in start failed:" << ex.what();
                out.error = LicenseOpError::Unknown;
                return out;
            }
        });
    }

    QFuture<LicenseService::SignInPoll> LicenseService::pollSignInAsync(const QString& deviceCode)
    {
        using vth::models::LicenseOpError;

        return QtConcurrent::run([this, deviceCode]() -> SignInPoll {
            SignInPoll out;
            try {
                vth::models::api::DesktopAuthPollRequest req;
                req.deviceCode = deviceCode;

                const auto resp = m_api->authPollAsync(req).result();
                if (!resp.success || !resp.data.has_value()) {
                    out.state = SignInPoll::State::Failed;
                    out.error = failureFrom(resp).error;
                    return out;
                }

                const auto obj = QJsonDocument::fromJson(*resp.data).object();
                if (obj.value(QStringLiteral("status")).toString()
                    != QLatin1String("approved")) {
                    return out;   // still pending; the caller polls again
                }

                // same as activate from here
                vth::license::bindToDevice(m_identity->devicePubkeyId());

                const QString token = obj.value(QStringLiteral("licenseToken")).toString();
                if (!vth::license::update(token)) {
                    out.state = SignInPoll::State::Failed;
                    out.error = LicenseOpError::TokenRejected;
                    return out;
                }

                {
                    QMutexLocker lock(&m_lock);
                    m_info.key = obj.value(QStringLiteral("licenseKey")).toString();
                    m_info.status = vth::models::LicenseStatus::Valid;
                    m_info.tier = obj.value(QStringLiteral("tier")).toString(QString());
                    m_info.trial = obj.value(QStringLiteral("trial")).toBool(false);
                    // licenseExpiresAt (null for lifetime -> empty), not tokenExpiresAt
                    m_info.expiresAt = obj.value(QStringLiteral("licenseExpiresAt")).toString();
                    m_info.activatedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
                    m_info.lastVerifiedUtcMs = QDateTime::currentMSecsSinceEpoch();
                    m_info.devicePubkeyId = m_identity->devicePubkeyId();
                    m_info.fingerprintStatus =
                        obj.value(QStringLiteral("fingerprintStatus")).toString();
                    m_info.resetsRemaining =
                        obj.value(QStringLiteral("resetsRemaining")).toInt();
                    m_info.accountEmail = obj.value(QStringLiteral("accountEmail")).toString();
                    m_info.accountDeleting = false;
                    m_info.accountHardDeleteAt.clear();
                    LicenseStore::save(m_info);
                }
                QMetaObject::invokeMethod(this, [this]() { scheduleHeartbeat(); },
                                          Qt::QueuedConnection);
                Q_EMIT licenseStateChanged();
                out.state = SignInPoll::State::Approved;
                return out;
            } catch (const std::exception& ex) {
                qCWarning(catLicenseSvc) << "sign-in poll failed:" << ex.what();
                out.state = SignInPoll::State::Failed;
                out.error = LicenseOpError::Unknown;
                return out;
            }
        });
    }

    QFuture<vth::models::LicenseOpResult> LicenseService::heartbeatAsync()
    {
        using vth::models::LicenseOpError;
        using vth::models::LicenseOpResult;

        return QtConcurrent::run([this]() -> LicenseOpResult {
            try {
                vth::models::api::LicenseHeartbeatRequest req;
                collectFingerprint(req.hardwareFingerprint);

                const auto r = m_api->heartbeatAsync(req).result();
                if (!r.success || !r.data.has_value()) {
                    const LicenseOpResult failure = failureFrom(r);

                    // definite answers clear the cache now (the token would keep working
                    // offline otherwise). Network/rate limit/server errors don't.
                    switch (failure.error) {
                    case LicenseOpError::KeyRevoked:
                    case LicenseOpError::KeyExpired:
                    case LicenseOpError::KeyUnknown:
                    case LicenseOpError::AccountSuspended: {
                        qCWarning(catLicenseSvc).noquote()
                            << "heartbeat returned a terminal verdict; clearing "
                               "local license state";
                        {
                            QMutexLocker lock(&m_lock);
                            m_info = vth::models::LicenseInfo{};
                            LicenseStore::deleteLicense();
                        }
                        vth::license::clear();
                        QMetaObject::invokeMethod(
                            this, [this]() { stopHeartbeat(); },
                            Qt::QueuedConnection);
                        Q_EMIT licenseStateChanged();
                        break;
                    }
                    default:
                        break;
                    }
                    // transient, back off
                    switch (failure.error) {
                    case LicenseOpError::KeyRevoked:
                    case LicenseOpError::KeyExpired:
                    case LicenseOpError::KeyUnknown:
                    case LicenseOpError::AccountSuspended:
                        break;
                    default:
                        QMetaObject::invokeMethod(
                            this, [this]() { onHeartbeatFailedTransiently(); },
                            Qt::QueuedConnection);
                        break;
                    }
                    return failure;
                }

                const auto obj = QJsonDocument::fromJson(*r.data).object();
                const QString token = obj.value(QStringLiteral("licenseToken")).toString();
                if (!vth::license::update(token))
                    return LicenseOpResult::failure(LicenseOpError::TokenRejected);
                {
                    QMutexLocker lock(&m_lock);
                    m_info.status = vth::models::LicenseStatus::Valid;
                    m_info.tier = obj.value(QStringLiteral("tier")).toString(m_info.tier);
                    m_info.trial = obj.value(QStringLiteral("trial")).toBool(m_info.trial);
                    m_info.expiresAt = obj.value(QStringLiteral("licenseExpiresAt")).toString();
                    m_info.lastVerifiedUtcMs = QDateTime::currentMSecsSinceEpoch();
                    m_info.fingerprintStatus = obj.value(QStringLiteral("fingerprintStatus")).toString();
                    m_info.resetsRemaining = obj.value(QStringLiteral("resetsRemaining")).toInt(m_info.resetsRemaining);
                    // missing = not being deleted (that's how a canceled deletion clears)
                    m_info.accountDeleting =
                        obj.value(QStringLiteral("accountDeleting")).toBool(false);
                    m_info.accountHardDeleteAt = m_info.accountDeleting
                        ? obj.value(QStringLiteral("accountHardDeleteAt")).toString()
                        : QString();
                    LicenseStore::save(m_info);
                }
                // sealed, the offline cutoff is measured from this
                vth::services::LicenseTokenCache::saveLastHeartbeatUtcMs(
                    QDateTime::currentMSecsSinceEpoch());
                QMetaObject::invokeMethod(
                    this, [this]() { onHeartbeatSucceeded(); },
                    Qt::QueuedConnection);
                Q_EMIT licenseStateChanged();
                return LicenseOpResult::success();
            } catch (const std::exception& ex) {
                qCWarning(catLicenseSvc) << "heartbeat failed:" << ex.what();
                return LicenseOpResult::failure(LicenseOpError::Unknown);
            }
        });
    }

    QFuture<vth::models::LicenseOpResult> LicenseService::deactivateAsync()
    {
        using vth::models::LicenseOpError;
        using vth::models::LicenseOpResult;

        return QtConcurrent::run([this]() -> LicenseOpResult {
            try {
                const auto r = m_api->deactivateAsync().result();
                // wipe local state whatever the server said
                {
                    QMutexLocker lock(&m_lock);
                    m_info = vth::models::LicenseInfo{};
                    LicenseStore::deleteLicense();
                }
                vth::license::clear();
                QMetaObject::invokeMethod(this, [this]() { stopHeartbeat(); },
                                          Qt::QueuedConnection);
                Q_EMIT licenseStateChanged();
                // result = whether the server freed the slot
                return r.success ? LicenseOpResult::success() : failureFrom(r);
            } catch (const std::exception& ex) {
                qCWarning(catLicenseSvc) << "deactivate failed:" << ex.what();
                return LicenseOpResult::failure(LicenseOpError::Unknown);
            }
        });
    }

    void LicenseService::scheduleHeartbeat()
    {
#ifdef VETHIDA_DEV_BUILD
        // dev builds don't heartbeat (devUnlockAll)
        return;
#else
        QString key;
        {
            QMutexLocker lock(&m_lock);
            key = m_info.key;
        }
        if (key.isEmpty())
            return;   // nothing to heartbeat until the license is activated

        if (m_heartbeatTimer == nullptr) {
            m_heartbeatTimer = new QTimer(this);
            m_heartbeatTimer->setSingleShot(true);
            connect(m_heartbeatTimer, &QTimer::timeout, this, [this]() { heartbeatAsync(); });
        }

        // Single-shot, rescheduled after each answer so it can back off. First beat
        // is immediate unless the stamp says one happened inside the interval.
        const qint64 last = vth::services::LicenseTokenCache::loadLastHeartbeatUtcMs();
        const qint64 intervalMs =
            static_cast<qint64>(SecurityConfig::LicenseVerifyInterval.count()) * 1000;
        const qint64 sinceMs = last == 0
            ? intervalMs
            : QDateTime::currentMSecsSinceEpoch() - last;

        m_backoffMs = 0;
        if (sinceMs >= intervalMs)
            heartbeatAsync();
        else
            m_heartbeatTimer->start(static_cast<int>(intervalMs - sinceMs));
#endif
    }

    void LicenseService::onHeartbeatSucceeded()
    {
        m_backoffMs = 0;
        if (m_heartbeatTimer) {
            m_heartbeatTimer->start(
                static_cast<int>(SecurityConfig::LicenseVerifyInterval.count()) * 1000);
        }
    }

    void LicenseService::onHeartbeatFailedTransiently()
    {
        // 30s doubling up to the normal interval. Doesn't touch entitlement.
        const qint64 ceilingMs =
            static_cast<qint64>(SecurityConfig::BackoffMaxMinutes) * 60 * 1000;
        m_backoffMs = m_backoffMs == 0
            ? static_cast<qint64>(SecurityConfig::BackoffInitialSeconds) * 1000
            : qMin(m_backoffMs * 2, ceilingMs);
        if (m_heartbeatTimer)
            m_heartbeatTimer->start(static_cast<int>(m_backoffMs));
    }

    void LicenseService::stopHeartbeat()
    {
        if (m_heartbeatTimer)
            m_heartbeatTimer->stop();
    }
}
