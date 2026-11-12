#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>

namespace vth::core
{
    class Updater;

    // Pulls updates.json from vethida.com, passes the update to Updater and
    // applies disabled_tweaks. Only the update part is signed.
    class UpdateFeed : public QObject
    {
        Q_OBJECT

    public:
        explicit UpdateFeed(Updater* updater, QObject* parent = nullptr);

        Updater* updater()   const { return updater_; }
        QString  lastError() const { return lastError_; }
        bool     checking()  const { return checking_; }

        // throttled to every 6h unless forced, does nothing if the channel is "off"
        void checkNow(bool force = false);

    Q_SIGNALS:
        void checkFinished(bool ok);

    private:
        void applyPayload(const QByteArray& body);

        Updater* updater_ = nullptr;
        QString  lastError_;
        bool     checking_ = false;
    };
}
