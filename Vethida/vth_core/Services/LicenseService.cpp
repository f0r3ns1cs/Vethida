#include "LicenseService.h"

#include "LicenseApi.h"
#include "SecureStorage.h"

#include "../Logger.h"
#include "../Runtime/LicenseCache.h"
#include "../Runtime/Tier.h"

#include <QtCore/QDateTime>
#include <QtCore/QJsonDocument>
#include <QtCore/QPromise>
#include <QtCore/QTimer>
#include <QtCore/QUrlQuery>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include <memory>

namespace vth::services
{
    using vth::models::LicenseInfo;
    using vth::models::LicenseOpError;
    using vth::models::LicenseOpResult;
    using vth::models::LicenseStatus;
    namespace ls = vth::licensing;

    namespace
    {
        VTH_DEFINE_LOG_CATEGORY(catLicense, "vth.license.service")

        const QString kStorageKey = QStringLiteral("license");
        constexpr int kRequestTimeoutMs = 15000;
        constexpr int kServerRetryMs = 10 * 60 * 1000;

        qint64 nowMs() { return QDateTime::currentMSecsSinceEpoch(); }

        // A finished future, for answers decided before any request.
        QFuture<LicenseOpResult> ready(const LicenseOpResult& r)
        {
            QPromise<LicenseOpResult> p;
            p.start();
            p.addResult(r);
            p.finish();
            return p.future();
        }

        QString tierName(vth::models::ProductTier t)
        {
            switch (t) {
            case vth::models::ProductTier::Ultimate: return QStringLiteral("ultimate");
            case vth::models::ProductTier::Pro:      return QStringLiteral("pro");
            default:                                 return {};
            }
        }

        // Turns a refusal into the outcome the customer acts on. The API
        // reports refusals as an `error` sentence plus the key's status.
        LicenseOpError classify(int http, const QJsonObject& body)
        {
            if (http == 429)
                return LicenseOpError::RateLimited;
            if (http >= 500)
                return LicenseOpError::Server;

            const QString status =
                body.value(QStringLiteral("license_key")).toObject()
                    .value(QStringLiteral("status")).toString();
            if (status == QLatin1String("expired"))
                return LicenseOpError::KeyExpired;
            if (status == QLatin1String("disabled"))
                return LicenseOpError::KeyRevoked;

            const QString error = body.value(QStringLiteral("error")).toString().toLower();
            if (error.contains(QLatin1String("activation limit")))
                return LicenseOpError::MachineLimitReached;
            if (error.contains(QLatin1String("expired")))
                return LicenseOpError::KeyExpired;
            if (error.contains(QLatin1String("disabled")))
                return LicenseOpError::KeyRevoked;
            if (http == 404 || error.contains(QLatin1String("not found")))
                return LicenseOpError::KeyUnknown;
            return LicenseOpError::Unknown;
        }
    }

    LicenseService::LicenseService(QObject* parent)
        : QObject(parent)
        , m_net(new QNetworkAccessManager(this))
        , m_timer(new QTimer(this))
    {
        m_timer->setInterval(ls::kRecheckIntervalMs);
        connect(m_timer, &QTimer::timeout, this, [this]() { validateAsync(); });

        m_retry = new QTimer(this);
        m_retry->setSingleShot(true);
        connect(m_retry, &QTimer::timeout, this, [this]() { validateAsync(); });
    }

    LicenseService::~LicenseService() = default;

    void LicenseService::start()
    {
        load();
        applyEntitlement();
        m_timer->start();
        if (!m_info.key.isEmpty())
            validateAsync();
    }

    void LicenseService::post(const char* endpoint,
                              const QList<QPair<QString, QString>>& form,
                              std::function<void(const Reply&)> done)
    {
        QNetworkRequest req(QUrl(QString::fromLatin1(ls::kApiBase) + QLatin1String(endpoint)));
        req.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/x-www-form-urlencoded"));
        req.setRawHeader("Accept", "application/json");
        req.setTransferTimeout(kRequestTimeoutMs);

        QUrlQuery query;
        query.setQueryItems(form);
        QNetworkReply* reply =
            m_net->post(req, query.toString(QUrl::FullyEncoded).toUtf8());

        connect(reply, &QNetworkReply::finished, this, [reply, done = std::move(done)]() {
            reply->deleteLater();
            Reply r;
            r.httpStatus =
                reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            r.body = QJsonDocument::fromJson(reply->readAll()).object();
            // An HTTP error status still carries the API's answer; only a
            // request that got no response at all is a network failure.
            r.networkError = r.httpStatus == 0;
            if (r.networkError)
                qCWarning(catLicense) << "licensing API unreachable:" << reply->errorString();
            done(r);
        });
    }

    bool LicenseService::absorb(const QJsonObject& body)
    {
        const QJsonObject meta = body.value(QStringLiteral("meta")).toObject();
        const QJsonObject key  = body.value(QStringLiteral("license_key")).toObject();
        const QJsonObject inst = body.value(QStringLiteral("instance")).toObject();

        const QString product = meta.value(QStringLiteral("product")).toString();
        const vth::models::ProductTier tier =
            ls::tierFromName(meta.value(QStringLiteral("tier")).toString());
        if (product != QLatin1String(ls::kProduct) || tier == vth::models::ProductTier::None) {
            qCWarning(catLicense) << "key is for product" << product;
            return false;
        }

        m_info.tier   = tierName(tier);
        m_info.status = LicenseStatus::Valid;
        m_info.lastVerifiedUtcMs = nowMs();
        if (!inst.isEmpty()) {
            m_info.instanceId  = inst.value(QStringLiteral("id")).toString();
            m_info.activatedAt = inst.value(QStringLiteral("created_at")).toString();
        }
        m_info.expiresAt       = key.value(QStringLiteral("expires_at")).toString();
        m_info.activationLimit = key.value(QStringLiteral("activation_limit")).toInt();
        m_info.activationUsage = key.value(QStringLiteral("activation_usage")).toInt();
        m_info.customerEmail   = meta.value(QStringLiteral("customer_email")).toString();
        m_info.productName     = meta.value(QStringLiteral("plan")).toString();
        return true;
    }

    QFuture<LicenseOpResult> LicenseService::activateAsync(const QString& rawKey)
    {
        const QString key = vth::models::LicenseValidation::formatKey(rawKey);
        if (!vth::models::LicenseValidation::isValidKeyFormat(key))
            return ready(LicenseOpResult::failure(LicenseOpError::KeyFormat));

        auto promise = std::make_shared<QPromise<LicenseOpResult>>();
        promise->start();

        post("activate",
             { { QStringLiteral("license_key"), key },
               // Not the computer name, which often carries a person's name.
               { QStringLiteral("instance_name"), QStringLiteral("Windows PC") } },
             [this, key, promise](const Reply& r) {
            LicenseOpResult result;
            if (r.networkError) {
                result = LicenseOpResult::failure(LicenseOpError::Network);
            } else if (!r.body.value(QStringLiteral("activated")).toBool()) {
                result = LicenseOpResult::failure(
                    classify(r.httpStatus, r.body),
                    r.body.value(QStringLiteral("error")).toString());
            } else {
                const LicenseInfo before = m_info;
                m_info = LicenseInfo{};
                m_info.key = key;
                if (absorb(r.body)) {
                    persist();
                    applyEntitlement();
                    result = LicenseOpResult::success();
                    Q_EMIT licenseStateChanged();
                    // A new key replaced an old one (an upgrade, say): give the
                    // old key's activation back so it does not hold a slot.
                    if (!before.key.isEmpty() && before.key != key
                        && !before.instanceId.isEmpty()) {
                        post("deactivate",
                             { { QStringLiteral("license_key"), before.key },
                               { QStringLiteral("instance_id"), before.instanceId } },
                             [](const Reply&) {});
                    }
                } else {
                    // Not ours. Give the activation back so the slot on that
                    // other product is not used up by a mistaken paste.
                    const QString instance = r.body.value(QStringLiteral("instance"))
                        .toObject().value(QStringLiteral("id")).toString();
                    post("deactivate",
                         { { QStringLiteral("license_key"), key },
                           { QStringLiteral("instance_id"), instance } },
                         [](const Reply&) {});
                    m_info = before;
                    result = LicenseOpResult::failure(LicenseOpError::WrongProduct);
                }
            }
            promise->addResult(result);
            promise->finish();
        });
        return promise->future();
    }

#ifdef VETHIDA_DEV_BUILD
    void LicenseService::devSimulate(const QString& state)
    {
        m_simulated = true;
        vth::setDevUnlockSuppressed(true);
        m_info = LicenseInfo{};
        m_offline = false;
        if (state != QLatin1String("free")) {
            m_info.key             = QStringLiteral("11111111-2222-3333-4444-555555555555");
            m_info.instanceId      = QStringLiteral("simulated");
            m_info.tier            = state == QLatin1String("pro") ? QStringLiteral("pro")
                                                                   : QStringLiteral("ultimate");
            m_info.status          = LicenseStatus::Valid;
            m_info.customerEmail   = QStringLiteral("customer@example.com");
            m_info.activationLimit = 3;
            m_info.activationUsage = 1;
            m_info.activatedAt     = QDateTime::currentDateTimeUtc().addDays(-9).toString(Qt::ISODate);
            m_info.lastVerifiedUtcMs = nowMs() - 5 * 60 * 1000;
            if (state == QLatin1String("expired"))    m_info.status = LicenseStatus::Expired;
            if (state == QLatin1String("revoked"))    m_info.status = LicenseStatus::Revoked;
            if (state == QLatin1String("unverified")) m_info.status = LicenseStatus::Unverified;
            if (state == QLatin1String("offline"))    m_offline = true;
            if (state == QLatin1String("notactive"))  m_info.instanceId.clear();
        }
        applyEntitlement();
        Q_EMIT licenseStateChanged();
    }
#endif

    QFuture<LicenseOpResult> LicenseService::validateAsync()
    {
#ifdef VETHIDA_DEV_BUILD
        if (m_simulated)
            return ready(LicenseOpResult::success());
#endif
        if (m_info.key.isEmpty() || m_info.instanceId.isEmpty())
            return ready(LicenseOpResult::failure(LicenseOpError::KeyUnknown));

        auto promise = std::make_shared<QPromise<LicenseOpResult>>();
        promise->start();
        m_checking = true;

        post("validate",
             { { QStringLiteral("license_key"), m_info.key },
               { QStringLiteral("instance_id"), m_info.instanceId } },
             [this, promise](const Reply& r) {
            m_checking = false;
            LicenseOpResult result;
            if (r.networkError) {
                // No connection: no plan. The saved license is kept, so the
                // plan returns by itself once the API answers.
                result = LicenseOpResult::failure(LicenseOpError::Network);
                setOffline(true);
                m_retry->start(ls::kOfflineRetryMs);
            } else if (r.httpStatus == 429 || r.httpStatus >= 500) {
                // The server is the one having trouble, not this PC: the
                // plan stays as it was and the check is tried again soon.
                result = LicenseOpResult::failure(classify(r.httpStatus, r.body));
                setOffline(false);
                m_retry->start(kServerRetryMs);
            } else if (r.body.value(QStringLiteral("valid")).toBool() && absorb(r.body)) {
                setOffline(false);
                result = LicenseOpResult::success();
            } else {
                setOffline(false);
                // A definite no. Keep the key so the Account page can say what
                // happened, but grant nothing.
                const LicenseOpError why = classify(r.httpStatus, r.body);
                m_info.status = why == LicenseOpError::KeyExpired ? LicenseStatus::Expired
                              : why == LicenseOpError::KeyRevoked ? LicenseStatus::Revoked
                                                                  : LicenseStatus::Unverified;
                if (why == LicenseOpError::KeyUnknown)
                    m_info.instanceId.clear();   // deactivated elsewhere; activate again
                result = LicenseOpResult::failure(
                    why, r.body.value(QStringLiteral("error")).toString());
            }
            persist();
            applyEntitlement();
            Q_EMIT licenseStateChanged();
            promise->addResult(result);
            promise->finish();
        });
        return promise->future();
    }

    void LicenseService::validateIfStale()
    {
        if (m_info.key.isEmpty() || m_info.instanceId.isEmpty() || m_checking)
            return;
        // Offline, every return to the window is a chance to get the plan back.
        if (!m_offline && nowMs() - m_info.lastVerifiedUtcMs < ls::kStaleAfterMs)
            return;
        validateAsync();
    }

    void LicenseService::setOffline(bool offline)
    {
        if (m_offline == offline)
            return;
        m_offline = offline;
        qCInfo(catLicense) << (offline ? "offline: running as Free until the license can be checked"
                                       : "back online: license confirmed");
    }

    QFuture<LicenseOpResult> LicenseService::reactivateAsync()
    {
        if (m_info.key.isEmpty())
            return ready(LicenseOpResult::failure(LicenseOpError::KeyUnknown));
        return activateAsync(m_info.key);
    }

    QFuture<LicenseOpResult> LicenseService::deactivateAsync()
    {
        const QString key = m_info.key;
        const QString instance = m_info.instanceId;
        clearLocal();
        Q_EMIT licenseStateChanged();

        if (key.isEmpty() || instance.isEmpty())
            return ready(LicenseOpResult::success());

        auto promise = std::make_shared<QPromise<LicenseOpResult>>();
        promise->start();
        post("deactivate",
             { { QStringLiteral("license_key"), key },
               { QStringLiteral("instance_id"), instance } },
             [promise](const Reply& r) {
            LicenseOpResult result;
            if (r.networkError)
                result = LicenseOpResult::failure(LicenseOpError::Network);
            else if (r.body.value(QStringLiteral("deactivated")).toBool())
                result = LicenseOpResult::success();
            else
                result = LicenseOpResult::failure(classify(r.httpStatus, r.body),
                                                  r.body.value(QStringLiteral("error")).toString());
            promise->addResult(result);
            promise->finish();
        });
        return promise->future();
    }

    // The saved answer is used until the first check finishes, so startup doesn't flash Free.
    void LicenseService::applyEntitlement()
    {
        if (m_info.status == LicenseStatus::Valid && !m_offline)
            vth::license::setTier(m_info.tier);
        else
            vth::license::clear();
    }

    void LicenseService::load()
    {
        const std::optional<QByteArray> json = SecureStorage::getJson(kStorageKey);
        if (!json)
            return;
        const QJsonObject o = QJsonDocument::fromJson(*json).object();
        m_info.key               = o.value(QStringLiteral("key")).toString();
        m_info.instanceId        = o.value(QStringLiteral("instanceId")).toString();
        m_info.tier              = o.value(QStringLiteral("tier")).toString();
        m_info.status            = static_cast<LicenseStatus>(o.value(QStringLiteral("status")).toInt());
        m_info.activatedAt       = o.value(QStringLiteral("activatedAt")).toString();
        m_info.expiresAt         = o.value(QStringLiteral("expiresAt")).toString();
        m_info.lastVerifiedUtcMs = o.value(QStringLiteral("lastVerified")).toVariant().toLongLong();
        m_info.customerEmail     = o.value(QStringLiteral("email")).toString();
        m_info.productName       = o.value(QStringLiteral("product")).toString();
        m_info.activationLimit   = o.value(QStringLiteral("limit")).toInt();
        m_info.activationUsage   = o.value(QStringLiteral("usage")).toInt();
    }

    void LicenseService::persist() const
    {
        QJsonObject o;
        o.insert(QStringLiteral("key"),          m_info.key);
        o.insert(QStringLiteral("instanceId"),   m_info.instanceId);
        o.insert(QStringLiteral("tier"),         m_info.tier);
        o.insert(QStringLiteral("status"),       static_cast<int>(m_info.status));
        o.insert(QStringLiteral("activatedAt"),  m_info.activatedAt);
        o.insert(QStringLiteral("expiresAt"),    m_info.expiresAt);
        o.insert(QStringLiteral("lastVerified"), m_info.lastVerifiedUtcMs);
        o.insert(QStringLiteral("email"),        m_info.customerEmail);
        o.insert(QStringLiteral("product"),      m_info.productName);
        o.insert(QStringLiteral("limit"),        m_info.activationLimit);
        o.insert(QStringLiteral("usage"),        m_info.activationUsage);
        SecureStorage::setJson(kStorageKey, QJsonDocument(o).toJson(QJsonDocument::Compact));
    }

    void LicenseService::clearLocal()
    {
        m_info = LicenseInfo{};
        m_offline = false;
        SecureStorage::deleteKey(kStorageKey);
        vth::license::clear();
    }
}
