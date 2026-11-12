#pragma once

#include <QFuture>
#include <QtConcurrent/QtConcurrent>

#include <functional>
#include <type_traits>

namespace vth
{
    // Logs an exception that escaped a worker task. Keeps Logger.h out of this header.
    void reportWorkerException(const char* what);

    class Dispatcher {
    public:
        bool isShuttingDown() const noexcept;
        static Dispatcher& instance();

        // Configure pool size at boot. Subsequent calls are ignored.
        void configure(int workerCount = 0);

        // Queue work on the worker pool. Returns immediately.
        template <typename Fn>
        auto dispatch(Fn&& fn) -> QFuture<std::invoke_result_t<std::decay_t<Fn>>> {
            using R = std::invoke_result_t<std::decay_t<Fn>>;
            static_assert(std::is_void_v<R> || std::is_default_constructible_v<R>,
                          "Dispatcher::dispatch needs a default-constructible result "
                          "so a failed task has something to return");
            return QtConcurrent::run(pool_, [fn = std::forward<Fn>(fn)]() mutable -> R {
                try {
                    return fn();
                } catch (const std::exception& e) {
                    reportWorkerException(e.what());
                } catch (...) {
                    reportWorkerException(nullptr);
                }
                if constexpr (!std::is_void_v<R>)
                    return R{};
            });
        }

        // Queue a callable to run on the UI thread, asynchronously. Safe from
        // any thread. No-ops once the dispatcher has been shut down.
        void dispatchUi(std::function<void()> fn);

        // Block until the worker pool drains. Called once at shutdown by
        // App::onShutdown(). Do not call from the worker pool itself.
        void shutdown();

        int workerCount() const noexcept;

        QThreadPool* pool() const noexcept { return pool_; }

    private:
        Dispatcher();
        ~Dispatcher();
        Dispatcher(const Dispatcher&) = delete;
        Dispatcher& operator=(const Dispatcher&) = delete;

        QThreadPool* pool_ = nullptr;
        bool         configured_   = false;
        bool         shuttingDown_ = false;
    };

}
