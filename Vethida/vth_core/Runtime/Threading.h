#pragma once

#include <QCoreApplication>
#include <QThread>

namespace vth
{
    inline bool isUiThread() noexcept {
        auto* app = QCoreApplication::instance();
        return app && QThread::currentThread() == app->thread();
    }
}

#if !defined(NDEBUG)
#  define VTH_ASSERT_UI_THREAD()                                                \
      Q_ASSERT_X(::vth::isUiThread(), Q_FUNC_INFO,                              \
                 "must be called on the UI thread")
#  define VTH_ASSERT_WORKER_THREAD()                                            \
      Q_ASSERT_X(!::vth::isUiThread(), Q_FUNC_INFO,                             \
                 "must be called off the UI thread")
#else
#  define VTH_ASSERT_UI_THREAD()     ((void)0)
#  define VTH_ASSERT_WORKER_THREAD() ((void)0)
#endif
