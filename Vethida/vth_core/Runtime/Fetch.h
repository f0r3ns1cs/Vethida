#pragma once

#include "Dispatcher.h"
#include "../Logger.h"

#include <QObject>
#include <QPointer>

#include <atomic>
#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

namespace vth
{
    VTH_DECLARE_LOG_CATEGORY(catFetch)

    // Cancellation handle from fetch<T>. Copies share one flag.
    class FetchToken {
    public:
        FetchToken();

        void cancel() noexcept;
        bool isCanceled() const noexcept;

    private:
        std::shared_ptr<std::atomic<bool>> canceled_;
    };

    // work() on a worker, deliver() on the UI thread (skipped if canceled or the
    // context is gone). If work() throws, deliver gets T{}.
    template <typename T>
    FetchToken fetch(QObject*               context,
                     std::function<T()>     work,
                     std::function<void(T)> deliver)
    {
        FetchToken token;
        QPointer<QObject> ctxGuard(context);

        Dispatcher::instance().dispatch(
            [token, work    = std::move(work),
                    deliver = std::move(deliver),
                    ctxGuard]() mutable {

                if (token.isCanceled()) return;

                std::optional<T> result;
                try {
                    result.emplace(work());
                } catch (const std::exception& e) {
                    qCWarning(catFetch) << "fetch work threw:" << e.what();
                } catch (...) {
                    qCWarning(catFetch) << "fetch work threw (unknown exception)";
                }
                // Deliver anyway so the view can leave its loading state.
                if (!result) {
                    // Not for a bare enum: its first value is often Ok.
                    if constexpr (std::is_default_constructible_v<T> && !std::is_enum_v<T>)
                        result.emplace();
                    else
                        return;
                }

                if (token.isCanceled()) return;
                if (!ctxGuard)          return;

                Dispatcher::instance().dispatchUi(
                    [token, deliver = std::move(deliver),
                            ctxGuard,
                            result = std::move(result)]() mutable {
                        if (token.isCanceled()) return;
                        if (!ctxGuard)          return;
                        deliver(std::move(*result));
                    });
            });

        return token;
    }
}
