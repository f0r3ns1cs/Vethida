#pragma once

#include "../../vth_core/Windows/Apps.h"

class QWidget;

namespace vth::dialogs
{
    // Properties for one installed application, with Uninstall and Open install folder.
    // Returns true if the user launched the uninstaller, so the caller can refresh.
    bool showAppDetails(QWidget* host, const vth::win::apps::AppInfo& app);
}
