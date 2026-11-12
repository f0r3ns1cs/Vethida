#include "GpuSampler.h"

#include "../Logger.h"

#include <QtCore/QByteArray>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>

#include <windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#pragma comment(lib, "pdh.lib")

namespace vth::services
{
    namespace
    {
        VTH_DEFINE_LOG_CATEGORY(catGpuSampler, "vth.gpusampler")

        class GpuEngineQuery
        {
        public:
            GpuEngineQuery() { tryOpen(); }
            ~GpuEngineQuery() { if (m_query) PdhCloseQuery(m_query); }

            bool ok() const { return m_query != nullptr && m_counter != nullptr; }

            double sample()
            {
                if (!ok()) return 0.0;
                if (PdhCollectQueryData(m_query) != ERROR_SUCCESS) return 0.0;

                DWORD bufLen = 0, count = 0;
                PdhGetFormattedCounterArrayW(m_counter, PDH_FMT_DOUBLE,
                                             &bufLen, &count, nullptr);
                if (bufLen == 0) return 0.0;
                QByteArray buf(static_cast<int>(bufLen), Qt::Uninitialized);
                auto* arr = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buf.data());
                if (PdhGetFormattedCounterArrayW(m_counter, PDH_FMT_DOUBLE,
                                                 &bufLen, &count, arr) != ERROR_SUCCESS)
                    return 0.0;
                double total = 0.0;
                for (DWORD i = 0; i < count; ++i) {
                    const auto& v = arr[i];
                    if (v.FmtValue.CStatus == ERROR_SUCCESS) {
                        total += v.FmtValue.doubleValue;
                    }
                }
                // Engines can sum past 100 on hybrid systems.
                return total > 100.0 ? 100.0 : total;
            }

        private:
            void tryOpen()
            {
                if (PdhOpenQueryW(nullptr, 0, &m_query) != ERROR_SUCCESS) {
                    m_query = nullptr;
                    return;
                }
                if (PdhAddEnglishCounterW(
                        m_query, L"\\GPU Engine(*)\\Utilization Percentage", 0,
                        &m_counter) != ERROR_SUCCESS) {
                    PdhCloseQuery(m_query);
                    m_query = nullptr;
                    m_counter = nullptr;
                    return;
                }
                // Prime the counter: the first PDH sample reads 0.
                PdhCollectQueryData(m_query);
            }

            PDH_HQUERY   m_query = nullptr;
            PDH_HCOUNTER m_counter = nullptr;
        };

        GpuEngineQuery& engineQuery()
        {
            static GpuEngineQuery q;
            return q;
        }

        QMutex& sampleMutex()
        {
            static QMutex m;
            return m;
        }
    }

    bool GpuSampler::isAvailable()
    {
        QMutexLocker lock(&sampleMutex());
        return engineQuery().ok();
    }

    double GpuSampler::sample()
    {
        QMutexLocker lock(&sampleMutex());
        try {
            return engineQuery().sample();
        } catch (const std::exception& ex) {
            qCWarning(catGpuSampler) << "GpuSampler.sample failed:" << ex.what();
            return 0.0;
        }
    }
}
