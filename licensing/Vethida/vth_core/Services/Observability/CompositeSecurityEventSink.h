#pragma once

#include "SecurityEvents.h"

#include <memory>
#include <vector>

namespace vth::services::observability
{
    // forwards to every sink, one failing doesn't stop the others
    class CompositeSecurityEventSink : public ISecurityEventSink
    {
    public:
        void addSink(std::shared_ptr<ISecurityEventSink> sink);
        void submit(const SecurityEvent& event) override;

    private:
        std::vector<std::shared_ptr<ISecurityEventSink>> m_sinks;
    };
}
