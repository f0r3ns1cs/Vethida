#include "MemorySampler.h"

#include <windows.h>

namespace vth::services
{
    std::tuple<double, quint64, quint64> MemorySampler::sample()
    {
        MEMORYSTATUSEX m{};
        m.dwLength = sizeof(MEMORYSTATUSEX);
        if (!GlobalMemoryStatusEx(&m)) return {0.0, 0, 0};
        return {static_cast<double>(m.dwMemoryLoad), m.ullTotalPhys, m.ullTotalPhys - m.ullAvailPhys};
    }
}
