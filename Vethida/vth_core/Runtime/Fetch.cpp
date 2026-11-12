#include "Fetch.h"

namespace vth
{
    VTH_DEFINE_LOG_CATEGORY(catFetch, "vth.fetch")

    FetchToken::FetchToken()
        : canceled_(std::make_shared<std::atomic<bool>>(false)) {}

    void FetchToken::cancel() noexcept
    {
        canceled_->store(true, std::memory_order_release);
    }

    bool FetchToken::isCanceled() const noexcept
    {
        return canceled_->load(std::memory_order_acquire);
    }
}
