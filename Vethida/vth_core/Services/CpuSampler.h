#pragma once

namespace vth::services
{
    // Reads "% Processor Utility", falling back to "% Processor Time". Rate
    // counters need repeated reads, so call this periodically.
    class CpuSampler
    {
    public:
        static double sample();
    };
}
