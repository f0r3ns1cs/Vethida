#include "CpuSampler.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>

#ifdef Q_OS_WIN
#  include <windows.h>
#  include <pdh.h>
#  pragma comment(lib, "Pdh.lib")
#endif

namespace vth::services
{
    namespace
    {
        QMutex g_sync;
        PDH_HQUERY g_query = nullptr;
        PDH_HCOUNTER g_utilityCounter = nullptr;
        PDH_HCOUNTER g_fallbackCounter = nullptr;
        bool g_useFallback = false;
    }

    double CpuSampler::sample()
    {
        QMutexLocker lock(&g_sync);
        try {
            if (g_query == nullptr) {
                if (PdhOpenQueryW(nullptr, 0, &g_query) != ERROR_SUCCESS) {
                    return 0.0;
                }
                PdhAddEnglishCounterW(g_query, L"\\Processor Information(_Total)\\% Processor Utility",
                                      0, &g_utilityCounter);
                PdhAddEnglishCounterW(g_query, L"\\Processor(_Total)\\% Processor Time",
                                      0, &g_fallbackCounter);
                // Rate counters need two collects; this is the first.
                PdhCollectQueryData(g_query);
                return 0.0;
            }

            if (PdhCollectQueryData(g_query) != ERROR_SUCCESS) {
                return 0.0;
            }

            PDH_FMT_COUNTERVALUE val{};
            if (!g_useFallback && g_utilityCounter != nullptr) {
                if (PdhGetFormattedCounterValue(g_utilityCounter, PDH_FMT_DOUBLE, nullptr, &val) == ERROR_SUCCESS) {
                    if (qIsFinite(val.doubleValue)) {
                        return qBound(0.0, val.doubleValue, 100.0);
                    }
                    g_useFallback = true;
                } else {
                    g_useFallback = true;
                }
            }
            if (g_useFallback && g_fallbackCounter != nullptr) {
                if (PdhGetFormattedCounterValue(g_fallbackCounter, PDH_FMT_DOUBLE, nullptr, &val) == ERROR_SUCCESS) {
                    if (qIsFinite(val.doubleValue)) {
                        return qBound(0.0, val.doubleValue, 100.0);
                    }
                }
            }
            return 0.0;
        } catch (...) {
            return 0.0;
        }
    }
}
