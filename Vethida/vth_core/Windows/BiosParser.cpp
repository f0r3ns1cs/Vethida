#include "BiosParser.h"

#include <QStringList>
#include <QStringLiteral>
#include <QtGlobal>

#include <vector>

#include <windows.h>
#include "../Logger.h"
#include <QHash>

namespace vth::win::biosparser
{
    static QString chassisTypeLabel(quint8 code);
    static QString memoryTypeLabel(quint8 code);
    static QString memoryFormFactorLabel(quint8 code);
    static QString processorFamilyLabel(quint16 code);

    namespace
    {
        #pragma pack(push, 1)
        struct RawSMBIOSData {
            BYTE  used20CallingMethod;
            BYTE  smbiosMajorVersion;
            BYTE  smbiosMinorVersion;
            BYTE  dmiRevision;
            DWORD length;
            BYTE  smbiosTableData[1];
        };

        struct StructHeader {
            BYTE  type;
            BYTE  length;
            WORD  handle;
        };
        #pragma pack(pop)

        bool isPlaceholder(const QString& s) {
            static const QStringList kPlaceholders = {
                QStringLiteral("to be filled by o.e.m."),
                QStringLiteral("to be filled by o.e.m"),
                QStringLiteral("default string"),
                QStringLiteral("system manufacturer"),
                QStringLiteral("system product name"),
                QStringLiteral("system version"),
                QStringLiteral("system serial number"),
                QStringLiteral("base board version"),
                QStringLiteral("chassis manufacture"),
                QStringLiteral("chassis version"),
                QStringLiteral("chassis serial number"),
                QStringLiteral("asset-1234567890"),
                QStringLiteral("not specified"),
                QStringLiteral("not applicable"),
                QStringLiteral("unknown"),
                QStringLiteral("none"),
                QStringLiteral("n/a"),
                QStringLiteral("oem"),
                QStringLiteral("filled by oem"),
            };
            const QString t = s.trimmed().toLower();
            if (t.isEmpty())
                return true;
            if (kPlaceholders.contains(t))
                return true;
            // Filler like "00000000" or "....".
            for (const QChar c : t) {
                if (c.isLetter() || (c.isDigit() && c != QLatin1Char('0')))
                    return false;
            }
            return true;
        }

        // Strings follow the formatted area, by 1-based index, ending in a double null.
        QString smbiosString(const BYTE* structStart, size_t formattedLen, BYTE index) {
            if (index == 0) return {};
            const BYTE* p = structStart + formattedLen;
            BYTE current = 1;
            while (*p != 0) {
                if (current == index) {
                    const QString s =
                        QString::fromLocal8Bit(reinterpret_cast<const char*>(p)).trimmed();
                    return isPlaceholder(s) ? QString() : s;
                }
                while (*p != 0) ++p;
                ++p;
                ++current;
            }
            return {};
        }

        const BYTE* nextStructure(const BYTE* structStart, size_t formattedLen) {
            const BYTE* p = structStart + formattedLen;
            while (true) {
                if (*p == 0 && *(p + 1) == 0) {
                    return p + 2;
                }
                while (*p != 0) ++p;
                ++p;
            }
        }

        quint16 readU16(const BYTE* p) { return static_cast<quint16>(p[0] | (p[1] << 8)); }
        quint32 readU32(const BYTE* p) {
            return static_cast<quint32>(p[0]) |
                   (static_cast<quint32>(p[1]) << 8) |
                   (static_cast<quint32>(p[2]) << 16) |
                   (static_cast<quint32>(p[3]) << 24);
        }
        quint64 readU64(const BYTE* p) {
            return static_cast<quint64>(readU32(p))
                 | (static_cast<quint64>(readU32(p + 4)) << 32);
        }

        // The first three fields are little-endian.
        QString formatUuid(const BYTE* p) {
            bool allZero = true, allFf = true;
            for (int i = 0; i < 16; ++i) {
                if (p[i] != 0x00) allZero = false;
                if (p[i] != 0xFF) allFf   = false;
            }
            if (allZero || allFf) return {};
            return QString::asprintf(
                "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                p[3], p[2], p[1], p[0],
                p[5], p[4],
                p[7], p[6],
                p[8], p[9],
                p[10], p[11], p[12], p[13], p[14], p[15]);
        }

        // Bit 15 means KB rather than MB. 0 is no module, 0xFFFF is unknown, and 0x7FFF
        // means the extended size, in MB, with its top bit reserved.
        quint64 decodeMemorySize(quint16 raw, quint32 extended) {
            if (raw == 0 || raw == 0xFFFF) return 0;
            if (raw == 0x7FFF) return static_cast<quint64>(extended & 0x7FFFFFFF) * 1024ull * 1024ull;
            const bool kb = (raw & 0x8000) != 0;
            const quint64 value = (raw & 0x7FFF);
            return kb ? value * 1024ull : value * 1024ull * 1024ull;
        }

        void parseType0(const BYTE* s, size_t len, BiosInfo& out) {
            if (len < 0x12) return;
            out.vendor      = smbiosString(s, len, s[0x04]);
            out.version     = smbiosString(s, len, s[0x05]);
            out.releaseDate = smbiosString(s, len, s[0x08]);
            // 0xFF.0xFF means not supported, and 0.0 means not filled in.
            const auto release = [](BYTE major, BYTE minor) -> QString {
                if ((major == 0xFF && minor == 0xFF) || (major == 0 && minor == 0))
                    return {};
                return QStringLiteral("%1.%2").arg(major).arg(minor);
            };
            if (len >= 0x16)
                out.majorRelease = release(s[0x14], s[0x15]);
            if (len >= 0x18)
                out.embeddedFirmware = release(s[0x16], s[0x17]);
        }

        void parseType1(const BYTE* s, size_t len, SystemInfo& out) {
            if (len < 0x08) return;
            out.manufacturer = smbiosString(s, len, s[0x04]);
            out.productName  = smbiosString(s, len, s[0x05]);
            out.version      = smbiosString(s, len, s[0x06]);
            out.serialNumber = smbiosString(s, len, s[0x07]);
            if (len >= 0x19) out.uuid = formatUuid(s + 0x08);
            if (len >= 0x1B) {
                out.sku   = smbiosString(s, len, s[0x19]);
                out.family = smbiosString(s, len, s[0x1A]);
            }
        }

        void parseType2(const BYTE* s, size_t len, BaseboardInfo& out) {
            if (len < 0x09) return;
            out.manufacturer = smbiosString(s, len, s[0x04]);
            out.product      = smbiosString(s, len, s[0x05]);
            out.version      = smbiosString(s, len, s[0x06]);
            out.serialNumber = smbiosString(s, len, s[0x07]);
            out.assetTag     = smbiosString(s, len, s[0x08]);
            if (len >= 0x0E) {
                out.locationInChassis = smbiosString(s, len, s[0x0A]);
            }
        }

        void parseType3(const BYTE* s, size_t len, ChassisInfo& out) {
            if (len < 0x09) return;
            out.manufacturer = smbiosString(s, len, s[0x04]);
            const BYTE typeByte = s[0x05] & 0x7F;     // the top bit is "lock present"
            out.type        = chassisTypeLabel(typeByte);
            out.version      = smbiosString(s, len, s[0x06]);
            out.serialNumber = smbiosString(s, len, s[0x07]);
            out.assetTag     = smbiosString(s, len, s[0x08]);
        }

        void parseType4(const BYTE* s, size_t len, CpuInfo& out) {
            if (len < 0x1A) return;
            out.socketDesignation = smbiosString(s, len, s[0x04]);
            out.manufacturer      = smbiosString(s, len, s[0x07]);
            out.version           = smbiosString(s, len, s[0x10]);

            // Bit 7 means the current voltage, in 0.1 V.
            const BYTE voltage = s[0x11];
            if (voltage & 0x80) out.voltageDeciVolts = voltage & 0x7F;

            out.externalClockMhz = readU16(s + 0x12);
            out.maxSpeedMhz      = readU16(s + 0x14);
            out.currentSpeedMhz  = readU16(s + 0x16);

            const BYTE status = s[0x18];
            out.populated = (status & 0x40) != 0;

            quint16 family = s[0x06];
            if (family == 0xFE && len >= 0x2A) {
                family = readU16(s + 0x28);
            }
            out.family = processorFamilyLabel(family);

            const quint64 sig = readU64(s + 0x08);
            out.signature = QStringLiteral("0x%1").arg(sig, 16, 16, QChar::fromLatin1('0'));

            if (len >= 0x26) {
                out.coreCount   = s[0x23];
                out.threadCount = s[0x25];
            }
            // 0xFF means the count is in the 16-bit field.
            if (len >= 0x30) {
                if (out.coreCount == 0xFF)   out.coreCount   = readU16(s + 0x2A);
                if (out.threadCount == 0xFF) out.threadCount = readU16(s + 0x2E);
            }
        }

        void parseType17(const BYTE* s, size_t len, MemoryDeviceInfo& out) {
            if (len < 0x15) return;

            out.totalWidthBits = readU16(s + 0x08);
            out.dataWidthBits  = readU16(s + 0x0A);

            const quint16 sizeRaw = readU16(s + 0x0C);
            quint32 extendedSize  = 0;
            if (len >= 0x20 && sizeRaw == 0x7FFF) {
                extendedSize = readU32(s + 0x1C);
            }
            out.sizeBytes = decodeMemorySize(sizeRaw, extendedSize);

            out.formFactor   = memoryFormFactorLabel(s[0x0E]);
            out.locator      = smbiosString(s, len, s[0x10]);
            out.bankLocator  = smbiosString(s, len, s[0x11]);
            out.memoryType   = memoryTypeLabel(s[0x12]);

            if (len >= 0x17) out.maxSpeedMtps = readU16(s + 0x15);
            if (len >= 0x1B) {
                out.manufacturer = smbiosString(s, len, s[0x17]);
                out.serialNumber = smbiosString(s, len, s[0x18]);
                out.assetTag     = smbiosString(s, len, s[0x19]);
                out.partNumber   = smbiosString(s, len, s[0x1A]);
            }
            if (len >= 0x1C) out.rank        = s[0x1B] & 0x0F;
            if (len >= 0x22) out.speedMtps = readU16(s + 0x20);

            // 0xFFFF means the speed is in the 32-bit extended field.
            if (out.maxSpeedMtps == 0xFFFF)
                out.maxSpeedMtps = len >= 0x58 ? (readU32(s + 0x54) & 0x7FFFFFFF) : 0;
            if (out.speedMtps == 0xFFFF)
                out.speedMtps = len >= 0x5C ? (readU32(s + 0x58) & 0x7FFFFFFF) : 0;

            if (out.speedMtps == 0)
                out.speedMtps = out.maxSpeedMtps;
        }
    }

    static QString chassisTypeLabel(quint8 code) {
        switch (code) {
            case 0x01: return QStringLiteral("Other");
            case 0x02: return QStringLiteral("Unknown");
            case 0x03: return QStringLiteral("Desktop");
            case 0x04: return QStringLiteral("Low Profile Desktop");
            case 0x05: return QStringLiteral("Pizza Box");
            case 0x06: return QStringLiteral("Mini Tower");
            case 0x07: return QStringLiteral("Tower");
            case 0x08: return QStringLiteral("Portable");
            case 0x09: return QStringLiteral("Laptop");
            case 0x0A: return QStringLiteral("Notebook");
            case 0x0B: return QStringLiteral("Hand Held");
            case 0x0C: return QStringLiteral("Docking Station");
            case 0x0D: return QStringLiteral("All-in-One");
            case 0x0E: return QStringLiteral("Sub Notebook");
            case 0x0F: return QStringLiteral("Space-Saving");
            case 0x10: return QStringLiteral("Lunch Box");
            case 0x11: return QStringLiteral("Main Server Chassis");
            case 0x12: return QStringLiteral("Expansion Chassis");
            case 0x13: return QStringLiteral("SubChassis");
            case 0x14: return QStringLiteral("Bus Expansion Chassis");
            case 0x15: return QStringLiteral("Peripheral Chassis");
            case 0x16: return QStringLiteral("RAID Chassis");
            case 0x17: return QStringLiteral("Rack Mount Chassis");
            case 0x18: return QStringLiteral("Sealed-Case PC");
            case 0x19: return QStringLiteral("Multi-System Chassis");
            case 0x1A: return QStringLiteral("Compact PCI");
            case 0x1B: return QStringLiteral("Advanced TCA");
            case 0x1C: return QStringLiteral("Blade");
            case 0x1D: return QStringLiteral("Blade Enclosure");
            case 0x1E: return QStringLiteral("Tablet");
            case 0x1F: return QStringLiteral("Convertible");
            case 0x20: return QStringLiteral("Detachable");
            case 0x21: return QStringLiteral("IoT Gateway");
            case 0x22: return QStringLiteral("Embedded PC");
            case 0x23: return QStringLiteral("Mini PC");
            case 0x24: return QStringLiteral("Stick PC");
        }
        return QStringLiteral("Unknown");
    }

    static QString memoryTypeLabel(quint8 code) {
        switch (code) {
            case 0x01: return QStringLiteral("Other");
            case 0x02: return QStringLiteral("Unknown");
            case 0x03: return QStringLiteral("DRAM");
            case 0x04: return QStringLiteral("EDRAM");
            case 0x05: return QStringLiteral("VRAM");
            case 0x06: return QStringLiteral("SRAM");
            case 0x07: return QStringLiteral("RAM");
            case 0x08: return QStringLiteral("ROM");
            case 0x09: return QStringLiteral("FLASH");
            case 0x0A: return QStringLiteral("EEPROM");
            case 0x0B: return QStringLiteral("FEPROM");
            case 0x0C: return QStringLiteral("EPROM");
            case 0x0D: return QStringLiteral("CDRAM");
            case 0x0E: return QStringLiteral("3DRAM");
            case 0x0F: return QStringLiteral("SDRAM");
            case 0x10: return QStringLiteral("SGRAM");
            case 0x11: return QStringLiteral("RDRAM");
            case 0x12: return QStringLiteral("DDR");
            case 0x13: return QStringLiteral("DDR2");
            case 0x14: return QStringLiteral("DDR2 FB-DIMM");
            case 0x18: return QStringLiteral("DDR3");
            case 0x19: return QStringLiteral("FBD2");
            case 0x1A: return QStringLiteral("DDR4");
            case 0x1B: return QStringLiteral("LPDDR");
            case 0x1C: return QStringLiteral("LPDDR2");
            case 0x1D: return QStringLiteral("LPDDR3");
            case 0x1E: return QStringLiteral("LPDDR4");
            case 0x1F: return QStringLiteral("Logical non-volatile device");
            case 0x20: return QStringLiteral("HBM");
            case 0x21: return QStringLiteral("HBM2");
            case 0x22: return QStringLiteral("DDR5");
            case 0x23: return QStringLiteral("LPDDR5");
            case 0x24: return QStringLiteral("HBM3");
        }
        return QStringLiteral("Unknown");
    }

    static QString memoryFormFactorLabel(quint8 code) {
        switch (code) {
            case 0x01: return QStringLiteral("Other");
            case 0x02: return QStringLiteral("Unknown");
            case 0x03: return QStringLiteral("SIMM");
            case 0x04: return QStringLiteral("SIP");
            case 0x05: return QStringLiteral("Chip");
            case 0x06: return QStringLiteral("DIP");
            case 0x07: return QStringLiteral("ZIP");
            case 0x08: return QStringLiteral("Proprietary Card");
            case 0x09: return QStringLiteral("DIMM");
            case 0x0A: return QStringLiteral("TSOP");
            case 0x0B: return QStringLiteral("Row of chips (soldered)");
            case 0x0C: return QStringLiteral("RIMM");
            case 0x0D: return QStringLiteral("SODIMM");
            case 0x0E: return QStringLiteral("SRIMM");
            case 0x0F: return QStringLiteral("FB-DIMM");
            case 0x10: return QStringLiteral("Die (soldered)");
            case 0x11: return QStringLiteral("CAMM");
        }
        return QStringLiteral("Unknown");
    }

    // Consumer hardware only. Anything else shows as hex.
    static QString processorFamilyLabel(quint16 code) {
        switch (code) {
            case 0x01: return QStringLiteral("Other");
            case 0x02: return QStringLiteral("Unknown");
            case 0x0B: return QStringLiteral("Pentium III");
            case 0x0C: return QStringLiteral("M1");
            case 0x0D: return QStringLiteral("M2");
            case 0xB1: return QStringLiteral("Pentium M");
            case 0xB2: return QStringLiteral("Celeron D");
            case 0xB3: return QStringLiteral("Pentium D");
            case 0xC2: return QStringLiteral("Core Duo");
            case 0xC3: return QStringLiteral("Core Solo");
            case 0xC6: return QStringLiteral("Core 2 Duo");
            case 0xC7: return QStringLiteral("Core 2 Solo");
            case 0xC8: return QStringLiteral("Core 2 Extreme");
            case 0xC9: return QStringLiteral("Core 2 Quad");
            case 0xCA: return QStringLiteral("Core 2 Extreme Mobile");
            case 0xCB: return QStringLiteral("Core 2 Duo Mobile");
            case 0xCC: return QStringLiteral("Core 2 Solo Mobile");
            case 0xCD: return QStringLiteral("Core i7");
            case 0xCE: return QStringLiteral("Dual-Core Celeron");
            case 0xD1: return QStringLiteral("Core i5");
            case 0xD2: return QStringLiteral("Core i3");
            case 0xD3: return QStringLiteral("Core i9");
            case 0xD4: return QStringLiteral("Xeon D");
            case 0x83: return QStringLiteral("Athlon 64");
            case 0x84: return QStringLiteral("Opteron");
            case 0x85: return QStringLiteral("Sempron");
            case 0x86: return QStringLiteral("Turion 64");
            case 0x87: return QStringLiteral("Dual-Core Opteron");
            case 0x88: return QStringLiteral("Athlon 64 X2");
            case 0x89: return QStringLiteral("Turion 64 X2");
            case 0x8A: return QStringLiteral("Quad-Core Opteron");
            case 0x8B: return QStringLiteral("3rd-gen Opteron");
            case 0x8C: return QStringLiteral("AMD Phenom FX");
            case 0x8D: return QStringLiteral("AMD Phenom X4");
            case 0x8E: return QStringLiteral("AMD Phenom X2");
            case 0x8F: return QStringLiteral("AMD Athlon X2");
            case 0xE4: return QStringLiteral("AMD Opteron 3000");
            case 0xE5: return QStringLiteral("AMD Sempron II");
            case 0xE6: return QStringLiteral("Embedded AMD Opteron Quad-Core");
            case 0xE7: return QStringLiteral("AMD Phenom II");
            case 0xE8: return QStringLiteral("AMD Athlon II");
            case 0xE9: return QStringLiteral("AMD Opteron Six-Core");
            case 0xEA: return QStringLiteral("AMD Sempron M");
            case 0x6F: return QStringLiteral("Ryzen");
            case 0x70: return QStringLiteral("Ryzen Pro");
            case 0xE0: return QStringLiteral("AMD A-Series");
            case 0x100: return QStringLiteral("ARMv7");
            case 0x101: return QStringLiteral("ARMv8");
        }
        return QStringLiteral("0x%1").arg(code, 2, 16, QChar::fromLatin1('0'));
    }

    Snapshot read() {
        Snapshot snap;

        const UINT size = GetSystemFirmwareTable('RSMB', 0, nullptr, 0);
        if (size == 0) {
            snap.error = QStringLiteral(
                "Could not query SMBIOS firmware table. The system firmware "
                "may not expose DMI to the OS.");
            return snap;
        }

        // Zeros past the end stop the string walks if firmware leaves out the double
        // null.
        std::vector<BYTE> buffer(static_cast<size_t>(size) + 4, 0);
        const UINT actual = GetSystemFirmwareTable('RSMB', 0, buffer.data(), size);
        if (actual == 0 || actual > size) {
            snap.error = QStringLiteral("GetSystemFirmwareTable returned no data.");
            return snap;
        }

        if (actual < sizeof(RawSMBIOSData)) {
            snap.error = QStringLiteral("SMBIOS buffer truncated.");
            return snap;
        }
        const auto* hdr = reinterpret_cast<const RawSMBIOSData*>(buffer.data());

        // The length comes from firmware, so it's clamped to what was returned.
        const BYTE* tableStart = hdr->smbiosTableData;
        const size_t headerBytes = static_cast<size_t>(tableStart - buffer.data());
        const size_t available   = (actual > headerBytes) ? (actual - headerBytes) : 0;
        const BYTE* tableEnd   = tableStart + qMin<size_t>(hdr->length, available);
        const BYTE* p = tableStart;

        while (p + sizeof(StructHeader) <= tableEnd) {
            const auto* sh = reinterpret_cast<const StructHeader*>(p);
            const size_t formattedLen = sh->length;
            if (formattedLen < sizeof(StructHeader)) break;
            if (p + formattedLen > tableEnd) break;

            switch (sh->type) {
                case 0:  parseType0(p, formattedLen, snap.bios);       break;
                case 1:  parseType1(p, formattedLen, snap.system);     break;
                case 2:  parseType2(p, formattedLen, snap.baseboard);  break;
                case 3:  parseType3(p, formattedLen, snap.chassis);    break;
                case 4: {
                    CpuInfo cpu;
                    parseType4(p, formattedLen, cpu);
                    if (!cpu.socketDesignation.isEmpty() || cpu.populated)
                        snap.cpus.append(cpu);
                    break;
                }
                case 17: {
                    MemoryDeviceInfo m;
                    parseType17(p, formattedLen, m);
                    snap.memorySlots.append(m);
                    break;
                }
                case 127:
                    snap.ok = true;
                    return snap;
                default:
                    break;
            }
            p = nextStructure(p, formattedLen);
        }

        snap.ok = true;
        return snap;
    }
}

namespace vth::win::biosparser
{
namespace
{
        VTH_DEFINE_LOG_CATEGORY(catBios, "vth.bios")
}
}
