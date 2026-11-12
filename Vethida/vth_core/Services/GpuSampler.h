#pragma once

namespace vth::services
{
    // Uses the GPU Engine performance counters, which Task Manager also reads.
    class GpuSampler
    {
    public:
        static bool   isAvailable();
        static double sample();
    };
}
