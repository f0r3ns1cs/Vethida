#include "UpdateFeed.h"

#include "../SiteUrls.h"

#include "Dispatcher.h"
#include "ServiceRegistry.h"
#include "Settings.h"
#include "Updater.h"
#include "../Http.h"
#include "../Tweaks/TweakEngine.h"

#include <QtCore/QDateTime>
#include <QtCore/QHash>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QPointer>

#include <chrono>

namespace vth::core
{
    namespace
    {
        constexpr const char* kChannelKey   = "updater/channel";
        constexpr const char* kLastCheckKey = "updater/lastCheckMs";
        constexpr qint64      kThrottleMs   = 6 * 60 * 60 * 1000;   // 6 hours
    }

    UpdateFeed::UpdateFeed(Updater* updater, QObject* parent)
        : QObject(parent), updater_(updater) {}

    void UpdateFeed::checkNow(bool force)
    {
        if (checking_) return;

        auto* settings = ServiceRegistry::instance().get<Settings>();

        QString channel = QStringLiteral("stable");
        if (settings) {
            channel = settings->value(QString::fromLatin1(kChannelKey),
                                      QStringLiteral("stable")).toString();
        }
        if (channel == QStringLiteral("off")) return;

        // hardcoded on purpose, the settings file is user-writable
        const QString feedUrl = QString(vth::site::kUpdateFeed);

        if (!force && settings) {
            const qint64 last = settings->value(
                QString::fromLatin1(kLastCheckKey), QStringLiteral("0")).toLongLong();
            const qint64 now = QDateTime::currentMSecsSinceEpoch();
            if (last > 0 && (now - last) < kThrottleMs) return;
        }

        checking_ = true;

        QPointer<UpdateFeed> self(this);
        Dispatcher::instance().dispatch([self, feedUrl]() {
            const auto resp = vth::http::get(feedUrl, {},
                                             std::chrono::milliseconds{ 20'000 });
            const QByteArray body = resp.ok ? resp.body : QByteArray();
            const QString err = resp.ok
                ? QString()
                : (resp.errorString.isEmpty() ? QStringLiteral("update check failed")
                                              : resp.errorString);

            Dispatcher::instance().dispatchUi([self, body, err]() {
                if (!self) return;
                self->checking_ = false;
                if (!err.isEmpty() || body.isEmpty()) {
                    self->lastError_ = err.isEmpty() ? QStringLiteral("empty response")
                                                     : err;
                    Q_EMIT self->checkFinished(false);
                    return;
                }
                self->lastError_.clear();
                self->applyPayload(body);
                if (auto* s = ServiceRegistry::instance().get<Settings>())
                    s->setValue(QString::fromLatin1(kLastCheckKey),
                                QString::number(QDateTime::currentMSecsSinceEpoch()));
                Q_EMIT self->checkFinished(true);
            });
        });
    }

    void UpdateFeed::applyPayload(const QByteArray& body)
    {
        const QJsonDocument doc = QJsonDocument::fromJson(body);
        if (!doc.isObject()) {
            lastError_ = QStringLiteral("malformed feed");
            return;
        }
        const QJsonObject root = doc.object();

        // use the channel's object if there is one, else the whole document
        QString channel = QStringLiteral("stable");
        if (auto* settings = ServiceRegistry::instance().get<Settings>())
            channel = settings->value(QString::fromLatin1(kChannelKey),
                                      QStringLiteral("stable")).toString();
        QJsonObject payload = root;
        if (root.value(channel).isObject())
            payload = root.value(channel).toObject();

        if (updater_ && payload.value(QStringLiteral("update")).isObject()) {
            const QJsonObject u = payload.value(QStringLiteral("update")).toObject();
            const QString version     = u.value(QStringLiteral("version")).toString();
            const QString downloadUrl = u.value(QStringLiteral("download_url")).toString();
            const QString sha256      = u.value(QStringLiteral("sha256")).toString();
            if (!version.isEmpty() && !downloadUrl.isEmpty() && !sha256.isEmpty()) {
                updater_->onAvailableUpdate(
                    version,
                    u.value(QStringLiteral("channel")).toString(QStringLiteral("stable")),
                    downloadUrl, sha256,
                    static_cast<qint64>(u.value(QStringLiteral("size_bytes")).toDouble()),
                    u.value(QStringLiteral("manifest_jws")).toString());
            }
        }

        // only if the key exists, a missing key shouldn't re-enable anything
        if (payload.contains(QStringLiteral("disabled_tweaks"))) {
            QHash<QString, QString> withdrawn;
            const QJsonArray disabled =
                payload.value(QStringLiteral("disabled_tweaks")).toArray();
            for (const QJsonValue& v : disabled) {
                if (v.isString()) {
                    const QString id = v.toString();
                    if (!id.isEmpty())
                        withdrawn.insert(id, QString());
                    continue;
                }
                if (!v.isObject()) continue;
                const QJsonObject o = v.toObject();
                const QString id = o.value(QStringLiteral("id")).toString();
                if (!id.isEmpty())
                    withdrawn.insert(id, o.value(QStringLiteral("reason")).toString());
            }
            vth::tweaks::setRemotelyDisabled(withdrawn);
        }

    }
}
