#pragma once

namespace vth::win
{
    bool isElevated() noexcept;

    // relaunch elevated. true = started, quit this one. false = declined or failed.
    bool relaunchAsAdmin() noexcept;
}
