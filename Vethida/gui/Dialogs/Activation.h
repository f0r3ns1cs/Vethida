#pragma once

#include <QString>

class QWidget;

namespace vth::services { class LicenseService; }

namespace vth::dialogs
{
    // The activation modal: a license key field, then a receipt. Blocks on a
    // local event loop and returns whether a license was activated.
    bool runActivation(QWidget* host, vth::services::LicenseService& svc,
                       const QString& prefillKey = QString());
}
