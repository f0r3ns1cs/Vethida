#pragma once

#include <QtCore/QString>

#include <optional>

namespace vth::services
{
    // The 6 fingerprint sources. Names have to match FINGERPRINT_SOURCES on the server.
    // A source is empty if it couldn't be read or was an OEM placeholder.
    struct HardwareFingerprintVectorPathC {
        std::optional<QString> motherboardUuid;   // firmware
        std::optional<QString> biosSerial;        // firmware
        std::optional<QString> cpuId;             // firmware, weak: see the note below
        std::optional<QString> diskSerial;        // firmware, the drive's own
        std::optional<QString> volumeSerial;      // OS, written by format
        std::optional<QString> primaryMac;        // hardware
    };

    // cpuId isn't unique per chip (same for every CPU of that model), the server
    // doesn't count it as a strong source

    // collects from WMI, the storage stack and the network adapters
    class HardwareFingerprintCollector {
    public:
        virtual ~HardwareFingerprintCollector() = default;

        // Runs all 6 queries and returns a fresh vector.
        virtual HardwareFingerprintVectorPathC collect();
    };

    // SHA-256 hex of one source, with the field name mixed in. Empty in, empty out.
    // Raw values never leave the machine. The pepper is added server-side.
    QString hashFingerprintSource(const QString& fieldName, const QString& value);

    // Applies hashFingerprintSource to every populated field.
    HardwareFingerprintVectorPathC hashFingerprint(
        const HardwareFingerprintVectorPathC& raw);
}
