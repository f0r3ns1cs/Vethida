#pragma once

#include "../../vth_core/Models/ProductTier.h"

#include <QList>
#include <QString>

#include <functional>

class QWidget;

namespace vth::ui
{
    class TweakController;
}

namespace vth::ui::tools
{
    // True when the current entitlement meets need. Otherwise shows the upgrade prompt
    // and returns false.
    bool ensureTier(QWidget* host, vth::models::ProductTier need);

    // The bundled power plans.
    struct PowerPlanSpec
    {
        // Aliased in vethida.qrc.
        QString resourcePath;
        // The scheme name stored inside the .pow, which is what powercfg reports as
        // active. It differs from name, the label this app shows.
        QString schemeName;
        // The fixed GUID the plan is installed under, so installing it again
        // replaces it instead of adding a second entry to Power Options.
        QString schemeGuid;
        QString name;
        QString detail;
        vth::models::ProductTier tier;
    };

    // in UI order: lowest tier first, laptop plan first within a tier
    const QList<PowerPlanSpec>& powerPlans();

    // Whatever powercfg currently reports as the active scheme, Vethida's or
    // Windows' own. Blocking: it runs powercfg, so call it through vth::fetch.
    struct ActivePowerPlan
    {
        bool    ok = false;   // powercfg answered and the line parsed
        QString guid;
        QString name;         // as Windows names it
    };
    ActivePowerPlan readActivePowerPlan();

    // Windows' own Balanced scheme, read by its well-known GUID so the localized name
    // comes from Windows. The way back from a Vethida plan.
    ActivePowerPlan readWindowsBalancedPlan();

    // no tier check, switching back must always work
    void setActivePowerScheme(const QString& guid, const QString& displayName,
                              std::function<void(bool)> done = {});

    // The work half of applyPowerPlan: stage the bundled .pow, import it, make
    // it active. No host, no tier gate, no prompt, no toast.
    void importAndActivatePlan(const PowerPlanSpec& plan,
                               std::function<void(bool, QString)> done);

    // Imports the plan with powercfg /import and makes it active. Gated on the
    // plan's own tier. Runs the powercfg pair off the UI thread.
    void applyPowerPlan(QWidget* host, const PowerPlanSpec& plan,
                        std::function<void(bool)> done = {});

    // Writes Vethida's tuned NVIDIA profile to the global driver profile via NVAPI DRS.
    // Gated on Pro, and does nothing on non-NVIDIA systems. Runs off the UI thread.
    void applyOptimalNvidiaProfile(QWidget* host, std::function<void()> done = {});

    // Restores the NVIDIA profile from before the first apply (asks first, no tier check).
    // done() runs on the UI thread afterwards.
    void revertNvidiaProfile(QWidget* host, std::function<void()> done = {});

    // The write half of the above, with no host, no tier gate and no prompt.
    void writeNvidiaProfile(std::function<void(bool, QString)> done);
}
