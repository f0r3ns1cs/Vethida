#pragma once

#include <functional>
#include <memory>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace vth
{
    class ServiceRegistry {
    public:
        static ServiceRegistry& instance();

        template <typename T>
        void add(std::unique_ptr<T> service) {
            const auto key = std::type_index(typeid(T));
            T* raw = service.release();
            slots_[key] = Slot{
                raw,
                [](void* p) { delete static_cast<T*>(p); }
            };
            order_.push_back(key);
        }

        template <typename T>
        T* get() const {
            const auto it = slots_.find(std::type_index(typeid(T)));
            return it == slots_.end() ? nullptr : static_cast<T*>(it->second.ptr);
        }

        template <typename Interface, typename Implementation>
        void addAlias(Implementation* service) {
            static_assert(std::is_base_of_v<Interface, Implementation>);
            const auto key = std::type_index(typeid(Interface));
            slots_[key] = Slot{
                static_cast<Interface*>(service),
                [](void*) {}
            };
            order_.push_back(key);
        }

        template <typename T>
        bool has() const {
            return slots_.find(std::type_index(typeid(T))) != slots_.end();
        }

        // Destroy services in reverse registration order. Safe to call twice.
        void clear();

    private:
        ServiceRegistry()  = default;
        ~ServiceRegistry() { clear(); }
        ServiceRegistry(const ServiceRegistry&)            = delete;
        ServiceRegistry& operator=(const ServiceRegistry&) = delete;

        struct Slot {
            void* ptr;
            void (*deleter)(void*);
        };

        std::unordered_map<std::type_index, Slot> slots_;
        std::vector<std::type_index>              order_;
    };

}
