#include "ServiceRegistry.h"

namespace vth
{
    ServiceRegistry& ServiceRegistry::instance() {
        static ServiceRegistry r;
        return r;
    }

    void ServiceRegistry::clear() {
        // Reverse order, so consumers shut down before their dependencies.
        for (auto it = order_.rbegin(); it != order_.rend(); ++it) {
            const auto slotIt = slots_.find(*it);
            if (slotIt != slots_.end()) {
                slotIt->second.deleter(slotIt->second.ptr);
            }
        }
        slots_.clear();
        order_.clear();
    }
}
