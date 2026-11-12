#include "SecurityEvents.h"

#include "../../Logger.h"

#include <QtCore/QDateTime>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>

namespace vth::services::observability
{
    namespace
    {
        VTH_DEFINE_LOG_CATEGORY(catSecEvents, "vth.security.events")

        QMutex g_lock;
        std::shared_ptr<ISecurityEventSink> g_sink;
    }

    void SecurityEvents::setSink(std::shared_ptr<ISecurityEventSink> sink)
    {
        QMutexLocker lock(&g_lock);
        g_sink = std::move(sink);
    }

    void SecurityEvents::emit_(SecurityEventType type, const QString& message,
                               std::function<void(QHash<QString, QString>&)> detailsFiller)
    {
        try {
            SecurityEvent ev;
            ev.type = type;
            ev.message = message;
            ev.timestamp = QDateTime::currentDateTimeUtc();
            if (detailsFiller)
                detailsFiller(ev.details);

            std::shared_ptr<ISecurityEventSink> sink;
            {
                QMutexLocker lock(&g_lock);
                sink = g_sink;
            }
            if (sink)
                sink->submit(ev);
        } catch (const std::exception& ex) {
            // Observability must never disrupt the caller.
            qCWarning(catSecEvents) << "security-event emit failed:" << ex.what();
        }
    }
}
