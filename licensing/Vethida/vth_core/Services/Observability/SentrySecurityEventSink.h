#pragma once

#include "SecurityEvents.h"

namespace vth::services::observability
{
    // Sends events to Sentry through SentryBridge (no-op without sentry.dll or
    // consent). Important ones are captured, the rest are breadcrumbs.
    class SentrySecurityEventSink : public ISecurityEventSink
    {
    public:
        SentrySecurityEventSink();
        ~SentrySecurityEventSink() override;

        void submit(const SecurityEvent& event) override;
    };
}
