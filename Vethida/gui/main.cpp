#include "App/App.h"
#include "Theme/Theme.h"
#include "Windows/MainWindow.h"
#include "Dialogs/Confirm.h"
#include "Dialogs/CrashRecoveryDialog.h"
#include "Dialogs/PrivacyConsentDialog.h"
#include "Utils/UiShots.h"
#include "Theme/DesignTokens.h"
#include "Widgets/Spinner.h"

#include "../vth_core/Logger.h"
#include "../vth_core/OsVersionHelper.h"
#include "../vth_core/Runtime/CrashRecoveryService.h"
#include "../vth_core/Runtime/TweakStateTracker.h"
#include "../vth_core/Runtime/Dispatcher.h"
#include "../vth_core/Runtime/Fetch.h"
#include "../vth_core/Runtime/Paths.h"
#include "../vth_core/Services/LicenseService.h"
#include "../vth_core/Runtime/Sentry.h"
#include "../vth_core/Runtime/ServiceRegistry.h"
#include "../vth_core/Runtime/Settings.h"
#include "../vth_core/Runtime/Tier.h"
#include "../vth_core/Windows/CrashHandler.h"
#include "../vth_core/Windows/Genesis.h"
#include "../vth_core/Windows/GenesisMonitor.h"
#include "../vth_core/Windows/NvidiaReinstall.h"

#include "../version.h"

#include <QApplication>
#include <QCoreApplication>
#include <QEventLoop>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QTranslator>

#include <exception>
#include <memory>

#include <windows.h>

namespace
{
    VTH_DEFINE_LOG_CATEGORY(catBoot, "vth.boot")

    // A second launch found a copy already running. Setup and the updater read this.
    constexpr int kExitDuplicateInstance = 2;
}

int main(int argc, char* argv[])
{
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    vth::App app(argc, argv);
#ifdef VETHIDA_DEV_BUILD
    // Keeps its settings and license apart from a real install's.
    QCoreApplication::setApplicationName(QString::fromLatin1(VETHIDA_PRODUCT_NAME " (dev)"));
#else
    QCoreApplication::setApplicationName(QString::fromLatin1(VETHIDA_PRODUCT_NAME));
#endif
    QCoreApplication::setApplicationVersion(QString::fromLatin1(VETHIDA_VERSION_STRING));
    QCoreApplication::setOrganizationName(QString::fromLatin1(VETHIDA_COMPANY_NAME));
    QCoreApplication::setOrganizationDomain(QString::fromLatin1(VETHIDA_COMPANY_DOMAIN));

    if (!app.isPrimaryInstance()) {
        if (app.forwardToPrimary(QStringLiteral("show")))
            return 0;
        qCInfo(catBoot) << "another instance is already running; exiting";
        return kExitDuplicateInstance;
    }

    // The installer's AppMutex names this, so Setup knows Vethida is running.
    CreateMutexW(nullptr, FALSE, L"VethidaApplicationMutex");

    // translations\vethida_<lang>.qm and qtbase_<lang>.qm beside the exe, if present.
    {
        const QString dir = QCoreApplication::applicationDirPath() + QStringLiteral("/translations");
        for (const QString& name : { QStringLiteral("qtbase"), QStringLiteral("vethida") }) {
            auto* translator = new QTranslator(&app);
            if (translator->load(QLocale(), name, QStringLiteral("_"), dir))
                QCoreApplication::installTranslator(translator);
            else
                delete translator;
        }
    }

    // First, before anything can fail.
    vth::win::installCrashHandler();

    vth::log::initialize();
    qCInfo(catBoot).noquote() << "Vethida" << VETHIDA_VERSION_STRING
                              << "starting; data dir =" << vth::paths::userData();
    qCInfo(catBoot).noquote() << "OS:" << vth::win::prettyVersion();

    if (vth::win::hadPreviousCrash()) {
        qCWarning(catBoot).noquote()
            << "previous session ended in a crash; dump at"
            << vth::win::previousCrashDumpPath();
        vth::win::acknowledgePreviousCrash();
    }
    vth::win::pruneOldDumps(5);

    vth::Dispatcher::instance().configure();
    qCInfo(catBoot) << "Dispatcher configured with"
                    << vth::Dispatcher::instance().workerCount() << "worker thread(s)";

    // Before the window exists, so every widget is built under the stylesheet.
    vth::theme::apply(app);

    auto& registry = vth::ServiceRegistry::instance();
    registry.add(std::make_unique<vth::Settings>());
    registry.add(std::make_unique<vth::sentry::Sentry>());
    if (auto* sentry = registry.get<vth::sentry::Sentry>()) {
        sentry->initialize();
    }

    registry.add(std::make_unique<vth::CrashRecoveryService>());
    if (auto* recovery = registry.get<vth::CrashRecoveryService>()) {
        recovery->initialize();
    }

    registry.add(std::make_unique<vth::TweakStateTracker>());

    // after the tracker, before the UI. Interrupted sessions are undone, never resumed.
    vth::win::genesis::RecoveryReport genesisRecovery;
    {
        const auto last = vth::win::genesis::loadPersistedState();
        if (vth::win::genesis::hasUnfinishedSession(last)) {
            QWidget notice(nullptr, Qt::SplashScreen | Qt::FramelessWindowHint
                                        | Qt::WindowStaysOnTopHint);
            notice.setObjectName(QStringLiteral("toastCard"));
            notice.setAttribute(Qt::WA_StyledBackground, true);
            auto* lay = new QHBoxLayout(&notice);
            lay->setContentsMargins(vth::theme::tokens::DialogInsetH, vth::theme::tokens::Space16,
                                    vth::theme::tokens::DialogInsetH, vth::theme::tokens::Space16);
            lay->setSpacing(vth::theme::tokens::CardMargin);
            lay->addWidget(new vth::widgets::Spinner(&notice, vth::theme::tokens::GlyphXl));
            lay->addWidget(new QLabel(QObject::tr("Finishing the last Genesis session..."), &notice));
            notice.adjustSize();
            notice.show();

            QEventLoop wait;
            vth::fetch<vth::win::genesis::RecoveryReport>(
                &notice,
                []() { return vth::win::genesis::recoverInterruptedSession(); },
                [&genesisRecovery, &wait](vth::win::genesis::RecoveryReport r) {
                    genesisRecovery = std::move(r);
                    wait.quit();
                });
            wait.exec();
        }
    }

    // A service, because it has to keep sweeping while the user is on another page.
    registry.add(std::make_unique<vth::win::genesis::GenesisMonitor>());

    auto licenseService = std::make_unique<vth::services::LicenseService>();
    vth::services::LicenseService* licenseServicePtr = licenseService.get();
    registry.add(std::move(licenseService));
    licenseServicePtr->start();

    MainWindow window(*licenseServicePtr);
    window.show();
    // Before the startup questions, so a step file can reach their exec() loops.
    vth::dev::startUiShots(&window);

    // Parented to the window so the scrim covers something, and one at a time so no
    // two exec() loops stack.
    vth::dialogs::PrivacyConsentDialog::runFirstRun(&window);

    if (auto* recovery = registry.get<vth::CrashRecoveryService>();
        recovery && recovery->detectUnclean()) {
        vth::dialogs::showCrashRecovery(&window, recovery->priorBatchOps(),
                                        recovery->priorBatchStarted());
        recovery->acknowledge();
    }

    if (!genesisRecovery.unrestored.isEmpty()) {
        vth::dialogs::warn(
            &window,
            QObject::tr("Some apps are still renamed"),
            QObject::tr("A Genesis session was interrupted. These files still carry "
                        "the name it gave them, because something is holding them "
                        "open. Restart the PC and open Vethida to finish.\n\n%1")
                .arg(genesisRecovery.unrestored.join(QLatin1Char('\n'))));
    }

    if (vth::win::nvidia::reinstall::recoverAtStartup()) {
        vth::dialogs::warn(
            &window,
            QObject::tr("The NVIDIA driver install did not finish"),
            QObject::tr("The last driver reinstall stopped before the new driver was in place. "
                        "Open GPU, then the driver card, and choose \"Finish the install\". "
                        "Vethida kept the prepared driver, so nothing has to be downloaded again."));
    }

    app.setForwardedPayloadHandler([&window](const QString&) {
        window.bringToFront();
    });

    // Qt doesn't catch exceptions thrown from slots. Logged here, then rethrown to
    // the terminate handler, which writes the dump.
    int rc = 0;
    try {
        rc = app.exec();
    } catch (const std::exception& e) {
        qCCritical(catBoot).noquote()
            << "unhandled exception escaped the event loop:" << e.what();
        vth::log::flush();
        throw;
    } catch (...) {
        qCCritical(catBoot) << "unhandled non-standard exception escaped the event loop";
        vth::log::flush();
        throw;
    }

    // Drains the workers while services exist, because in-flight completions could
    // otherwise touch dead Qt state.
    vth::Dispatcher::instance().shutdown();
    registry.clear();
    qCInfo(catBoot) << "Vethida shutting down";
    vth::log::shutdown();
    // Last, because it clears the crash breadcrumb the next launch reads.
    vth::win::uninstallCrashHandler();
    return rc;
}
