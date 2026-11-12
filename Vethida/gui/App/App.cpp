#include "pch.h"
#include "App.h"

#include "../../vth_core/Windows/SingleInstance.h"

namespace vth
{
    namespace
    {
#ifdef VETHIDA_DEV_BUILD
        // A development build runs beside an installed copy.
        constexpr auto kSingletonId = "Vethida.Singleton.Dev";
#else
        constexpr auto kSingletonId = "Vethida.Singleton";
#endif
    }

    App::App(int& argc, char* argv[])
        : QApplication(argc, argv)
        , guard_(std::make_unique<vth::win::SingleInstanceGuard>(
              QString::fromLatin1(kSingletonId)))
    {
    }

    // Here because SingleInstanceGuard is incomplete in the header.
    App::~App() = default;

    App* App::instance()
    {
        return static_cast<App*>(QCoreApplication::instance());
    }

    bool App::isPrimaryInstance() const noexcept
    {
        return guard_->isPrimary();
    }

    bool App::forwardToPrimary(const QString& payload)
    {
        return guard_->sendToPrimary(payload);
    }

    void App::setForwardedPayloadHandler(std::function<void(const QString&)> handler)
    {
        guard_->setPayloadHandler(std::move(handler));
    }
}
