#pragma once

#include <QString>

class QWidget;

namespace vth::services { class LicenseService; }

namespace vth::dialogs
{
    // In-app activation modal (the startup one is ActivationWindow). Blocks until it
    // closes, returns true if a license got activated.
    bool runActivation(QWidget* host, vth::services::LicenseService& svc,
                       const QString& prefillKey = QString());
}
