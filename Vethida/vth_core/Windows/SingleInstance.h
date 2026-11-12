#pragma once

#include <QString>

#include <functional>
#include <memory>

class QLocalServer;

namespace vth::win
{
    class SingleInstanceGuard
    {
    public:
        // Claims the primary slot, so isPrimary() is valid right after.
        explicit SingleInstanceGuard(const QString& appId);
        ~SingleInstanceGuard();

        SingleInstanceGuard(const SingleInstanceGuard&) = delete;
        SingleInstanceGuard& operator=(const SingleInstanceGuard&) = delete;

        bool isPrimary() const { return m_primary; }

        // Hands `payload` to the running copy and returns whether it landed.
        bool sendToPrimary(const QString& payload);

        // Called on the UI thread for each payload. Set it before the event loop
        // starts: a payload with no handler is dropped.
        void setPayloadHandler(std::function<void(const QString&)> handler);

    private:
        bool m_primary = false;
        QString m_serverName;
        std::unique_ptr<QLocalServer> m_server;
        std::function<void(const QString&)> m_handler;
    };

}
