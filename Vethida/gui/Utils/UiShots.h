#pragma once

class QWidget;

namespace vth::dev
{
    // Dev builds only. Runs the steps in VETHIDA_UI_SHOT and saves screenshots.
    void startUiShots(QWidget* mainWindow);
}
