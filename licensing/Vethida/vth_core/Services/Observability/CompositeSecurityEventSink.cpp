#include "CompositeSecurityEventSink.h"

#include "../../Logger.h"

namespace vth::services::observability
{
    namespace
    {
        VTH_DEFINE_LOG_CATEGORY(catSecComposite, "vth.security.events")
    }

    void CompositeSecurityEventSink::addSink(std::shared_ptr<ISecurityEventSink> sink)
    {
        if (sink)
            m_sinks.push_back(std::move(sink));
    }

    void CompositeSecurityEventSink::submit(const SecurityEvent& event)
    {
        for (auto& sink : m_sinks) {
            try {
                if (sink)
                    sink->submit(event);
            } catch (const std::exception& ex) {
                qCWarning(catSecComposite) << "sink threw:" << ex.what();
            }
        }
    }
}
