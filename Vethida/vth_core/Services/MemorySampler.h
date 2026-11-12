#pragma once

#include <cstdint>
#include <tuple>

#include <QtCore/QtGlobal>
#include <QtCore/QtTypes>

namespace vth::services
{
    class MemorySampler {
    public:
        // Returns (loadPercent, totalBytes, usedBytes).
        static std::tuple<double, quint64, quint64> sample();
    };
}
