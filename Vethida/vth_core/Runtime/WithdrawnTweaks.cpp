#include "WithdrawnTweaks.h"

#include "../Logger.h"
#include "../SiteUrls.h"
#include "../Tweaks/TweakEngine.h"

#include <QtCore/QHash>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QTimer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

namespace vth
{
    namespace
    {
        VTH_DEFINE_LOG_CATEGORY(catWithdrawn, "vth.withdrawn")
        constexpr int kRecheckMs = 24 * 60 * 60 * 1000;
    }

    WithdrawnTweaks::WithdrawnTweaks(QObject* parent)
        : QObject(parent)
        , m_net(new QNetworkAccessManager(this))
        , m_timer(new QTimer(this))
    {
        m_timer->setInterval(kRecheckMs);
        connect(m_timer, &QTimer::timeout, this, [this]() { fetch(); });
    }

    void WithdrawnTweaks::start()
    {
        m_timer->start();
        fetch();
    }

    void WithdrawnTweaks::fetch()
    {
        QNetworkRequest req{ QUrl(QString(vth::site::kWithdrawnTweaks)) };
        req.setTransferTimeout(15000);
        QNetworkReply* reply = m_net->get(req);
        connect(reply, &QNetworkReply::finished, this, [reply]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                qCInfo(catWithdrawn) << "withdrawn tweaks not fetched:" << reply->errorString();
                return;
            }
            const QJsonObject root = QJsonDocument::fromJson(reply->readAll()).object();
            if (!root.contains(QStringLiteral("disabled_tweaks")))
                return;

            QHash<QString, QString> withdrawn;
            for (const QJsonValue& v : root.value(QStringLiteral("disabled_tweaks")).toArray()) {
                if (v.isString()) {
                    if (!v.toString().isEmpty())
                        withdrawn.insert(v.toString(), QString());
                } else if (v.isObject()) {
                    const QJsonObject o = v.toObject();
                    const QString id = o.value(QStringLiteral("id")).toString();
                    if (!id.isEmpty())
                        withdrawn.insert(id, o.value(QStringLiteral("reason")).toString());
                }
            }
            vth::tweaks::setRemotelyDisabled(withdrawn);
        });
    }
}
