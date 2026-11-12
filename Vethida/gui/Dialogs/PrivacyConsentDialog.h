#pragma once

class QWidget;

namespace vth::dialogs
{
    // The first-run question about crash reports. Nothing is sent until the
    // person answers, and either answer is one click.
    class PrivacyConsentDialog
    {
    public:
        // Records the answer through vth::consent. No-op once the user has decided.
        static void runFirstRun(QWidget* host);
    };
}
