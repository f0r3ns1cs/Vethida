#include "pch.h"
#include "SystemTools.h"

#include "../Settings/Preferences.h"

#include "../../vth_core/SiteUrls.h"

#include "TweakController.h"

#include "../Dialogs/Confirm.h"
#include "../Widgets/ToastManager.h"

#include "../../vth_core/Models/ProductTier.h"
#include "../../vth_core/Runtime/Dispatcher.h"
#include "../../vth_core/Runtime/ServiceRegistry.h"
#include "../../vth_core/Runtime/Tier.h"
#include "../../vth_core/Services/RegistryEditor.h"
#include "../../vth_core/Tweaks/TweakEngine.h"
#include "../../vth_core/Windows/NvApi.h"
#include "../../vth_core/Windows/Power.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QUrl>
#include <QVector>

namespace vth::ui::tools
{
    namespace
    {
        // failures are warnings
        void toast(bool ok, const QString& title, const QString& body)
        {
            auto* t = vth::ServiceRegistry::instance().get<vth::widgets::ToastManager>();
            if (!t)
                return;
            if (ok)
                t->success(title, body);
            else
                t->warning(title, body);
        }
    }

    bool ensureTier(QWidget* host, vth::models::ProductTier need)
    {
        if (vth::meetsTier(need))
            return true;
        vth::dialogs::showUpgrade(host, vth::models::tierLabel(need));
        return false;
    }

    const QList<PowerPlanSpec>& powerPlans()
    {
        // Function-local static, so the tr() calls run after a translator is
        // installed rather than at static-init time.
        static const QList<PowerPlanSpec> plans = {
            { QStringLiteral(":/powerplans/pro-laptop.pow"),
              QStringLiteral("vth pro laptop"),
              QStringLiteral("6f3d2a51-0b9e-4c7a-9d2e-5a1c7e3b9f01"),
              QObject::tr("Performance on battery"),
              QObject::tr("Holds clocks up under load without parking cores, "
                          "while leaving the battery and thermal limits Windows "
                          "needs on a portable machine intact."),
              vth::models::ProductTier::Pro },
            { QStringLiteral(":/powerplans/pro-desktop.pow"),
              QStringLiteral("vth pro PC"),
              QStringLiteral("6f3d2a51-0b9e-4c7a-9d2e-5a1c7e3b9f02"),
              QObject::tr("Performance on AC power"),
              QObject::tr("Keeps every core awake at full clock and stops the "
                          "idle transitions that cost latency on a machine "
                          "that is always plugged in."),
              vth::models::ProductTier::Pro },
            { QStringLiteral(":/powerplans/ultimate-desktop.pow"),
              QStringLiteral("vth ultimate PC"),
              QStringLiteral("6f3d2a51-0b9e-4c7a-9d2e-5a1c7e3b9f03"),
              QObject::tr("Low latency"),
              QObject::tr("Idle promotion, core parking and interrupt "
                          "moderation disabled outright. Trades power draw and "
                          "heat for the lowest response time."),
              vth::models::ProductTier::Ultimate },
        };
        return plans;
    }

    ActivePowerPlan readActivePowerPlan()
    {
        const vth::win::power::ActiveScheme now = vth::win::power::readActiveScheme();
        ActivePowerPlan active;
        active.guid = now.guid;
        active.name = now.name;
        active.ok   = !now.guid.isEmpty() && !now.name.isEmpty();
        return active;
    }

    ActivePowerPlan readWindowsBalancedPlan()
    {
        // Looked up by Microsoft's fixed GUID, so the name comes from Windows in
        // whatever language it runs. Absent when a cleanup tool deleted it.
        ActivePowerPlan plan;
        plan.guid = vth::win::power::WellKnownPlans::balanced();
        plan.name = vth::win::power::schemeName(plan.guid);
        plan.ok   = !plan.name.isEmpty();
        return plan;
    }

    void setActivePowerScheme(const QString& guid, const QString& displayName,
                              std::function<void(bool)> done)
    {
        vth::Dispatcher::instance().dispatch([guid, displayName, done = std::move(done)]() mutable {
            const vth::win::power::PlanOutcome r = vth::win::power::activate(guid, displayName);
            vth::Dispatcher::instance().dispatchUi([r, done = std::move(done)]() {
                toast(r.ok, QObject::tr("Power plan"), r.message);
                if (done)
                    done(r.ok);
            });
        });
    }

    void importAndActivatePlan(const PowerPlanSpec& plan,
                               std::function<void(bool, QString)> done)
    {
        // Read here, on the calling thread: Qt resources are process-wide and
        // the bytes are small. The install itself runs on a worker.
        QByteArray bytes;
        {
            QFile in(plan.resourcePath);
            if (in.open(QIODevice::ReadOnly))
                bytes = in.readAll();
        }
        const QString guid       = plan.schemeGuid;
        const QString schemeName = plan.schemeName;
        const QString name       = plan.name;
        vth::Dispatcher::instance().dispatch([bytes, guid, schemeName, name,
                                              done = std::move(done)]() mutable {
            const vth::win::power::PlanOutcome r =
                vth::win::power::installAndActivate(bytes, guid, schemeName, name);
            vth::Dispatcher::instance().dispatchUi([r, done = std::move(done)]() mutable {
                if (done)
                    done(r.ok, r.message);
            });
        });
    }

    void applyPowerPlan(QWidget* host, const PowerPlanSpec& plan,
                        std::function<void(bool)> done)
    {
        if (!ensureTier(host, plan.tier)) {
            if (done)
                done(false);
            return;
        }

        QPointer<QWidget> guard(host);
        importAndActivatePlan(plan, [guard, done = std::move(done)]
                                    (bool ok, QString message) mutable {
            if (!guard)
                return;
            toast(ok, QObject::tr("Power plan"), message);
            if (done)
                done(ok);
        });
    }

    void applyOptimalNvidiaProfile(QWidget* host, std::function<void()> done)
    {
        // Pro, the same as the GPU section that holds the button.
        if (!ensureTier(host, vth::models::ProductTier::Pro))
            return;

        // loading nvapi64.dll is slow, check on a worker
        QPointer<QWidget> guard(host);
        vth::Dispatcher::instance().dispatch([guard, done = std::move(done)]() mutable {
            const bool present = vth::win::nvapi::available();
            vth::Dispatcher::instance().dispatchUi([guard, present, done = std::move(done)]() mutable {
                if (!guard)
                    return;
                if (!present) {
                    vth::dialogs::inform(guard, QObject::tr("No NVIDIA GPU found"),
                        QObject::tr("Vethida could not find an NVIDIA driver on this PC. "
                                    "This profile applies to NVIDIA graphics cards only."));
                    return;
                }

                // mention G-SYNC here, it's the change people notice right away
                if (!vth::prefs::askLess()
                    && !vth::dialogs::confirm(guard,
                        QObject::tr("Apply NVIDIA profile"),
                        QObject::tr("Vethida writes %1 latency, power, sync and image quality "
                                    "settings to your global NVIDIA driver profile. This turns "
                                    "G-SYNC off for every game. Undo puts your earlier settings "
                                    "back.").arg(vth::win::nvapi::optimalProfile().size()),
                        QObject::tr("Apply"), vth::dialogs::PromptTone::Warning))
                    return;

                writeNvidiaProfile([guard, done = std::move(done)](bool ok, QString message) {
                    if (!guard)
                        return;
                    toast(ok, QObject::tr("NVIDIA profile"), message);
                    if (done)
                        done();
                });
            });
        });
    }

    void revertNvidiaProfile(QWidget* host, std::function<void()> done)
    {
        if (!vth::prefs::askLess()
            && !vth::dialogs::confirm(host,
                QObject::tr("Undo NVIDIA profile"),
                QObject::tr("Every setting the profile changed goes back to what it was "
                            "before Vethida applied it, G-SYNC included."),
                QObject::tr("Undo"), vth::dialogs::PromptTone::Question))
            return;

        QPointer<QWidget> guard(host);
        vth::Dispatcher::instance().dispatch([guard, done = std::move(done)]() mutable {
            const vth::win::nvapi::Result r = vth::win::nvapi::revertOptimalProfile();
            vth::Dispatcher::instance().dispatchUi([guard, r, done = std::move(done)]() {
                if (!guard)
                    return;
                const bool clean = r.ok && r.error.isEmpty();
                toast(clean, QObject::tr("NVIDIA profile"),
                      clean ? QObject::tr("Your earlier NVIDIA settings are back.")
                            : r.error);
                if (done)
                    done();
            });
        });
    }

    void writeNvidiaProfile(std::function<void(bool, QString)> done)
    {
        vth::Dispatcher::instance().dispatch([done = std::move(done)]() mutable {
            const int total   = static_cast<int>(vth::win::nvapi::optimalProfile().size());
            const int applied = vth::win::nvapi::applyOptimalProfile();

            // A partial write isn't a success, because some settings didn't change.
            const bool ok = applied > 0 && applied == total;
            const QString message =
                applied == 0
                  ? QObject::tr("Vethida could not apply the profile. Update your "
                                "NVIDIA driver and try again.")
                  : applied < total
                      ? QObject::tr("Some profile settings could not be applied. "
                                    "Update your NVIDIA driver and try again.")
                      : QObject::tr("The NVIDIA profile is applied.");

            vth::Dispatcher::instance().dispatchUi([ok, message,
                                                    done = std::move(done)]() mutable {
                if (done)
                    done(ok, message);
            });
        });
    }
}
