#include "AutoUpdate.h"

#include "../Logger.h"
#include "../../version.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QLibrary>
#include <QtCore/QMetaObject>
#include <QtGui/QGuiApplication>
#include <QtGui/QWindow>

#include <cstring>
#include <string>

namespace vth::update
{
    namespace
    {
        VTH_DEFINE_LOG_CATEGORY(catUpdate, "vth.update")

#ifndef VETHIDA_DEV_BUILD
        static_assert(std::char_traits<char>::length(kEdDsaPublicKey) > 0,
                      "Set kEdDsaPublicKey in vth_core/Runtime/AutoUpdate.h before "
                      "a release build (node vethida.com/scripts/appcast.mjs keys).");
#endif

        // The slice of winsparkle.h Vethida uses. All __cdecl.
        using VoidFn        = void (__cdecl*)();
        using SetStrFn      = void (__cdecl*)(const char*);
        using SetKeyFn      = int (__cdecl*)(const char*);
        using SetDetailsFn  = void (__cdecl*)(const wchar_t*, const wchar_t*, const wchar_t*);
        using SetIntFn      = void (__cdecl*)(int);
        using CanShutdownFn = int (__cdecl*)();
        using SetCanShutFn  = void (__cdecl*)(CanShutdownFn);
        using SetShutReqFn  = void (__cdecl*)(VoidFn);

        struct Api {
            QLibrary     lib;
            VoidFn       init = nullptr;
            VoidFn       cleanup = nullptr;
            VoidFn       checkWithUi = nullptr;
            SetStrFn     setAppcastUrl = nullptr;
            SetKeyFn     setEdDsaKey = nullptr;
            SetDetailsFn setAppDetails = nullptr;
            SetIntFn     setAutomatic = nullptr;
            SetCanShutFn setCanShutdown = nullptr;
            SetShutReqFn setShutdownRequest = nullptr;
            bool         running = false;
        };

        Api& api()
        {
            static Api a;
            return a;
        }

        int __cdecl canShutdown() { return 1; }

        // WinSparkle thread, right before the installer runs. Close windows normally, then quit.
        void __cdecl requestShutdown()
        {
            QMetaObject::invokeMethod(QCoreApplication::instance(), []() {
                for (QWindow* w : QGuiApplication::topLevelWindows())
                    w->close();
                QCoreApplication::quit();
            }, Qt::QueuedConnection);
        }

        template <typename T>
        bool resolve(T& fn, const char* name)
        {
            fn = reinterpret_cast<T>(api().lib.resolve(name));
            return fn != nullptr;
        }
    }

    void start(bool automatic)
    {
        Api& a = api();
        if (a.running)
            return;

        // An absolute path beside the executable, never the DLL search order.
        a.lib.setFileName(QDir(QCoreApplication::applicationDirPath())
                              .filePath(QStringLiteral("WinSparkle.dll")));
        if (!a.lib.load()) {
            qCInfo(catUpdate) << "WinSparkle.dll not found; updates are off in this build";
            return;
        }
        const bool ok = resolve(a.init, "win_sparkle_init")
                     && resolve(a.cleanup, "win_sparkle_cleanup")
                     && resolve(a.checkWithUi, "win_sparkle_check_update_with_ui")
                     && resolve(a.setAppcastUrl, "win_sparkle_set_appcast_url")
                     && resolve(a.setEdDsaKey, "win_sparkle_set_eddsa_public_key")
                     && resolve(a.setAppDetails, "win_sparkle_set_app_details")
                     && resolve(a.setAutomatic, "win_sparkle_set_automatic_check_for_updates")
                     && resolve(a.setCanShutdown, "win_sparkle_set_can_shutdown_callback")
                     && resolve(a.setShutdownRequest, "win_sparkle_set_shutdown_request_callback");
        if (!ok) {
            qCWarning(catUpdate) << "WinSparkle.dll is missing an entry point; updates are off";
            a.lib.unload();
            return;
        }

        a.setAppcastUrl(kAppcastUrl);
        if (std::strlen(kEdDsaPublicKey) > 0 && a.setEdDsaKey(kEdDsaPublicKey) != 1) {
            qCWarning(catUpdate) << "WinSparkle rejected the EdDSA public key; updates are off";
            a.lib.unload();
            return;
        }
        a.setAppDetails(L"" VETHIDA_COMPANY_NAME, L"" VETHIDA_PRODUCT_NAME,
                        L"" VETHIDA_VERSION_STRING);
        a.setCanShutdown(&canShutdown);
        a.setShutdownRequest(&requestShutdown);
        a.setAutomatic(automatic ? 1 : 0);
        a.init();
        a.running = true;
        qCInfo(catUpdate) << "WinSparkle started; automatic checks" << automatic;
    }

    bool available()
    {
        return api().running;
    }

    void checkNow()
    {
        if (api().running)
            api().checkWithUi();
    }

    void setAutomatic(bool on)
    {
        if (api().running)
            api().setAutomatic(on ? 1 : 0);
    }

    void stop()
    {
        Api& a = api();
        if (!a.running)
            return;
        a.cleanup();
        a.running = false;
    }
}
