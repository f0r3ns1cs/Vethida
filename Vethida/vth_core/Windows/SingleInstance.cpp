#include "SingleInstance.h"

#include <QCryptographicHash>
#include <QDataStream>
#include <QLocalServer>
#include <QLocalSocket>

#include <windows.h>

namespace vth::win
{
    namespace
    {
        // The username is hashed because any process can enumerate pipe names.
        QString pipeNameFor(const QString& appId)
        {
            wchar_t buffer[256] = {};
            DWORD size = static_cast<DWORD>(std::size(buffer));
            const QString account = ::GetUserNameW(buffer, &size)
                ? QString::fromWCharArray(buffer, static_cast<int>(size ? size - 1 : 0))
                : QStringLiteral("unknown-user");

            const QByteArray digest = QCryptographicHash::hash(
                account.toUtf8(), QCryptographicHash::Sha256);
            return appId + QLatin1Char('-') + QString::fromLatin1(digest.toHex().left(16));
        }

        constexpr int kHandshakeTimeoutMs = 1500;
    }

    SingleInstanceGuard::SingleInstanceGuard(const QString& appId)
        : m_serverName(pipeNameFor(appId))
    {
        const auto someoneIsListening = [this]() {
            QLocalSocket probe;
            probe.connectToServer(m_serverName);
            if (!probe.waitForConnected(kHandshakeTimeoutMs))
                return false;
            probe.disconnectFromServer();
            return true;
        };

        // Check by connecting, because a killed process can leave the name behind.
        if (someoneIsListening()) {
            m_primary = false;
            return;
        }

        m_server = std::make_unique<QLocalServer>();
        // Other accounts must not reach this pipe: whatever is sent down it is
        // acted on by a process that may be elevated.
        m_server->setSocketOptions(QLocalServer::UserAccessOption);

        // Listen before removeServer, so a probe that merely timed out can't steal
        // the name from a live primary.
        if (!m_server->listen(m_serverName)) {
            // Lost a startup race to another copy.
            if (someoneIsListening()) {
                m_server.reset();
                m_primary = false;
                return;
            }

            // No one answered, so the name is stale.
            QLocalServer::removeServer(m_serverName);
            if (!m_server->listen(m_serverName)) {
                // Couldn't connect or listen, so run without the guard.
                m_server.reset();
                m_primary = true;
                return;
            }
        }

        QObject::connect(m_server.get(), &QLocalServer::newConnection,
                         m_server.get(), [this]() {
            while (QLocalSocket* client = m_server->nextPendingConnection()) {
                QObject::connect(client, &QLocalSocket::disconnected,
                                 client, &QLocalSocket::deleteLater);
                QObject::connect(client, &QLocalSocket::readyRead, client, [this, client]() {
                    // The transaction waits until the whole payload has arrived.
                    QDataStream in(client);
                    in.setVersion(QDataStream::Qt_6_0);
                    in.startTransaction();
                    QString payload;
                    in >> payload;
                    if (!in.commitTransaction())
                        return;
                    if (m_handler)
                        m_handler(payload);
                });
            }
        });
        m_primary = true;
    }

    SingleInstanceGuard::~SingleInstanceGuard()
    {
        if (m_server)
            m_server->close();
    }

    void SingleInstanceGuard::setPayloadHandler(std::function<void(const QString&)> handler)
    {
        m_handler = std::move(handler);
    }

    bool SingleInstanceGuard::sendToPrimary(const QString& payload)
    {
        if (m_primary)
            return false;

        // Before the send: the primary may raise its window as soon as it reads.
        ::AllowSetForegroundWindow(ASFW_ANY);

        QLocalSocket socket;
        socket.connectToServer(m_serverName);
        if (!socket.waitForConnected(kHandshakeTimeoutMs))
            return false;

        QByteArray framed;
        {
            QDataStream out(&framed, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_6_0);
            out << payload;
        }
        socket.write(framed);
        if (!socket.waitForBytesWritten(kHandshakeTimeoutMs))
            return false;

        // This process is about to exit, and unflushed bytes would be lost.
        socket.flush();
        socket.disconnectFromServer();
        if (socket.state() != QLocalSocket::UnconnectedState)
            socket.waitForDisconnected(kHandshakeTimeoutMs);
        return true;
    }
}
