#pragma once

#include "Fixes.h"

// Clears leftover policy values, which make Windows say "Some of these settings
// are managed by your organization" on a PC nobody manages.
namespace vth::win::managed
{
    // Takes a few seconds, so call it from a worker. Changes nothing if the PC
    // really is managed.
    vth::win::fixes::FixResult restoreWindowsDefaults();
}
