#pragma once

// Updates, through WinSparkle, which verifies each update's EdDSA signature against
// the key below and runs the new installer.
namespace vth::update
{
    inline constexpr const char* kAppcastUrl = "https://vethida.com/desktop/appcast.xml";

    // Base64 Ed25519 public key, printed by `node vethida.com/scripts/appcast.mjs keys`.
    // A release build won't compile without it, so an unsigned updater can't ship.
    inline constexpr const char* kEdDsaPublicKey = "";

    // Call once, after the main window exists.
    void start(bool automatic);

    bool available();

    void checkNow();

    void setAutomatic(bool on);

    // Call before the application exits.
    void stop();
}
