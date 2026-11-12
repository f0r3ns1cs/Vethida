#pragma once

#include "SecurityEvents.h"

namespace vth::services::observability
{
    // writes security events to the app log (always installed)
    class LogSecurityEventSink : public ISecurityEventSink
    {
    public:
        void submit(const SecurityEvent& event) override;
    };
}
