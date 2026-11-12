#include "NvramPresets.h"

#include <QHash>

#include <cstring>
#include <intrin.h>

namespace vth::win::nvram::presets
{
    namespace
    {
        // code is the AGESA option code on the AMD list. The Intel list has labels only.
        struct Tweak {
            const char* question;
            const char* label;
            const char* code;
            bool        dangerous;
        };

        constexpr bool kSafe = false;
        constexpr bool kRisk = true;

        const Tweak kIntel[] = {
            // Link and device power management
            { "Native ASPM",                                         "Disabled",            nullptr, kSafe },
            { "Enable ASPM",                                         "Disabled",            nullptr, kSafe },
            { "ASPM",                                                "Disabled",            nullptr, kSafe },
            { "Enable TBT ASPM",                                     "Disabled",            nullptr, kSafe },
            { "DMI Link ASPM Control",                               "Disabled",            nullptr, kSafe },
            { "PEG 0 ASPM",                                          "Disabled",            nullptr, kSafe },
            { "PEG 1 ASPM",                                          "Disabled",            nullptr, kSafe },
            { "PEG0 - ASPM",                                         "Disabled",            nullptr, kSafe },
            { "PEG1 - ASPM",                                         "Disabled",            nullptr, kSafe },
            { "PEG2 - ASPM",                                         "Disabled",            nullptr, kSafe },
            { "PCI Express Root Port 1 ASPM",                        "Disabled",            nullptr, kSafe },
            { "PCI Express Root Port 2 ASPM",                        "Disabled",            nullptr, kSafe },
            { "PCI Express Root Port 3 ASPM",                        "Disabled",            nullptr, kSafe },
            { "PCI Express Root Port 4 ASPM",                        "Disabled",            nullptr, kSafe },
            { "PCI Express Root Port 5 ASPM",                        "Disabled",            nullptr, kSafe },
            { "PCI Express Root Port 6 ASPM",                        "Disabled",            nullptr, kSafe },
            { "PCI Express Root Port 7 ASPM",                        "Disabled",            nullptr, kSafe },
            { "PCI Express Root Port 8 ASPM",                        "Disabled",            nullptr, kSafe },
            { "PCI Express Root Port 9 ASPM",                        "Disabled",            nullptr, kSafe },
            { "PCI Express Root Port 10 ASPM",                       "Disabled",            nullptr, kSafe },
            { "PCI Express Root Port 11 ASPM",                       "Disabled",            nullptr, kSafe },
            { "PCI Express Root Port 12 ASPM",                       "Disabled",            nullptr, kSafe },
            { "PCI Express Root Port 13 ASPM",                       "Disabled",            nullptr, kSafe },
            { "PCI Express Root Port 14 ASPM",                       "Disabled",            nullptr, kSafe },
            { "PCI Express Root Port 15 ASPM",                       "Disabled",            nullptr, kSafe },
            { "PCI Express Root Port 16 ASPM",                       "Disabled",            nullptr, kSafe },
            { "PCI Express Root Port 17 ASPM",                       "Disabled",            nullptr, kSafe },
            { "PCI Express Root Port 18 ASPM",                       "Disabled",            nullptr, kSafe },
            { "PCI Express Root Port 19 ASPM",                       "Disabled",            nullptr, kSafe },
            { "PCI Express Root Port 20 ASPM",                       "Disabled",            nullptr, kSafe },
            { "PCI Express Root Port 21 ASPM",                       "Disabled",            nullptr, kSafe },
            { "PCI Express Root Port 22 ASPM",                       "Disabled",            nullptr, kSafe },
            { "PCI Express Root Port 23 ASPM",                       "Disabled",            nullptr, kSafe },
            { "PCI Express Root Port 24 ASPM",                       "Disabled",            nullptr, kSafe },
            { "Enable LTR",                                          "Disabled",            nullptr, kSafe },
            { "LTR",                                                 "Disabled",            nullptr, kSafe },
            { "LTR Mechanism Enable",                                "Disabled",            nullptr, kSafe },
            { "Clock Power Management",                              "Disabled",            nullptr, kSafe },
            { "Power Down Unused Lanes",                             "Disabled",            nullptr, kSafe },
            { "PCI Express Clock Gating",                            "Disabled",            nullptr, kSafe },
            { "PCI Express Power Gating",                            "Disabled",            nullptr, kSafe },
            { "L1 Substates",                                        "Disabled",            nullptr, kSafe },
            { "Disable Gen2 Pll Shutdown and L1 Controller Power gating", "Enabled",        nullptr, kSafe },
            { "PCI Delay Optimization",                              "Enabled",             nullptr, kSafe },
            { "MSI enabled",                                         "Enabled",             nullptr, kSafe },
            { "JTAG C10 Power Gate",                                 "Disabled",            nullptr, kSafe },
            { "Enable 8254 Clock Gate",                              "Disabled",            nullptr, kSafe },
            { "USB2 PHY Sus Well Power Gating",                      "Disabled",            nullptr, kSafe },
            { "USB 2.0 Controller Mode",                             "HiSpeed",             nullptr, kSafe },
            { "Aggressive LPM Support",                              "Disabled",            nullptr, kSafe },
            { "PUIS Enable",                                         "Disabled",            nullptr, kSafe },
            { "ZPODD",                                               "Disabled",            nullptr, kSafe },
            { "RC6(Render Standby)",                                 "Disabled",            nullptr, kSafe },
            { "Power Down Mode",                                     "Disabled",            nullptr, kSafe },
            { "Legacy IO Low Latency",                               "Enabled",             nullptr, kSafe },
            { "HPET",                                                "Enabled",             nullptr, kSafe },
            { "Pcie Pll SSC",                                        "0.0%",                nullptr, kSafe },
            { "PCIE Spread Spectrum",                                "Disabled",            nullptr, kSafe },
            { "ACPI Debug",                                          "Disabled",            nullptr, kSafe },
            { "Power Loss Notification Feature",                     "Disabled",            nullptr, kSafe },
            { "BIST",                                                "Disabled",            nullptr, kSafe },

            // Wake sources
            { "Wake System from S5",                                 "Disabled",            nullptr, kSafe },
            { "USB S5 Wakeup Support",                               "Disabled",            nullptr, kSafe },
            { "LAN Wake From DeepSx",                                "Disabled",            nullptr, kSafe },
            { "Wake on LAN Enable",                                  "Disabled",            nullptr, kSafe },
            { "Wake on WLAN and BT Enable",                          "Disabled",            nullptr, kSafe },
            { "DeepSx Wake on WLAN and BT Enable",                   "Disabled",            nullptr, kSafe },
            { "USB Standby Power at S4/S5",                          "Disabled",            nullptr, kSafe },
            { "Intel Ready Mode Technology",                         "Disabled",            nullptr, kSafe },
            { "Intel RMT State",                                     "Disabled",            nullptr, kSafe },

            // CPU power states
            { "C6DRAM",                                              "Disabled",            nullptr, kSafe },
            { "SW Guard Extensions (SGX)",                           "Software Controlled", nullptr, kSafe },
            { "AP threads Idle Manner",                              "RUN Loop",            nullptr, kSafe },
            { "Intel(R) Speed Shift Technology Interrupt Control",   "Disabled",            nullptr, kSafe },
            { "Boot performance mode",                               "Turbo Performance",   nullptr, kSafe },
            { "EIST",                                                "Disabled",            nullptr, kSafe },
            { "Race To Halt (RTH)",                                  "Disabled",            nullptr, kSafe },
            { "Intel Speed Shift Technology",                        "Disabled",            nullptr, kSafe },
            { "Intel(R) Adaptive Boost Technology",                  "Disabled",            nullptr, kSafe },
            { "Intel Adaptive Boost Technology",                     "Disabled",            nullptr, kSafe },
            { "Intel C-State",                                       "Disabled",            nullptr, kSafe },
            { "C1E Support",                                         "Disabled",            nullptr, kSafe },
            { "C-State Auto Demotion",                               "Disabled",            nullptr, kSafe },
            { "C-State Un-demotion",                                 "Disabled",            nullptr, kSafe },
            { "Package C-State Demotion",                            "Disabled",            nullptr, kSafe },
            { "Package C-State Un-demotion",                         "Disabled",            nullptr, kSafe },
            { "CState Pre-Wake",                                     "Disabled",            nullptr, kSafe },
            { "Package C State Limit",                               "C0",                  nullptr, kSafe },
            { "CPU C States Support",                                "Disabled",            nullptr, kSafe },
            { "Package C States support",                            "Disabled",            nullptr, kSafe },
            { "Interrupt Redirection Mode Selection",                "Round robin",         nullptr, kSafe },
            { "Energy Efficient P-state",                            "Disabled",            nullptr, kSafe },
            { "Energy Efficient Turbo",                              "Disabled",            nullptr, kSafe },
            { "P-state Capping",                                     "Disabled",            nullptr, kSafe },

            // Offset 0 is the factory default, throttling at the processor's own limit.
            { "Tcc Activation Offset",                               "0",                   nullptr, kRisk },

            // Dangerous: voltage, ratios, memory power
            { "BCLK Aware Adaptive Voltage",                         "Disabled",            nullptr, kRisk },
            { "TVB Ratio Clipping",                                  "Disabled",            nullptr, kRisk },
            { "Ring Down Bin",                                       "Disabled",            nullptr, kRisk },
            { "EPG DIMM Idd3N",                                      "0",                   nullptr, kRisk },
            { "EPG DIMM Idd3P",                                      "0",                   nullptr, kRisk },
            { "DDR PowerDown and idle counter",                      "PCODE",               nullptr, kRisk },
            { "For LPDDR Only: DDR PowerDown and idle counter",      "PCODE",               nullptr, kRisk },
            { "PowerDown Energy Ch0Dimm0",                           "0",                   nullptr, kRisk },
            { "PowerDown Energy Ch0Dimm1",                           "0",                   nullptr, kRisk },
            { "PowerDown Energy Ch1Dimm0",                           "0",                   nullptr, kRisk },
            { "PowerDown Energy Ch1Dimm1",                           "0",                   nullptr, kRisk },

            // Dangerous: sleep and laptop embedded controller
            { "Low Power S0 Idle Capability",                        "Disabled",            nullptr, kRisk },
            { "Modern StandBy",                                      "Disabled",            nullptr, kRisk },
            { "ACPI Standby State",                                  "Suspend Disabled",    nullptr, kRisk },
            { "Enable Hibernation",                                  "0",                   nullptr, kRisk },
            { "EC Notification",                                     "Disabled",            nullptr, kRisk },
            { "EC CS Debug Light",                                   "Disabled",            nullptr, kRisk },
            { "EC Low Power Mode",                                   "Disabled",            nullptr, kRisk },
            { "Sensor Standby",                                      "Disabled",            nullptr, kRisk },
            { "CS PL1 Limit",                                        "Disabled",            nullptr, kRisk },

            // Dangerous: security and platform features Windows relies on
            { "Intel Virtualization Tech",                           "Disabled",            nullptr, kRisk },
            { "Intel VT-D Tech",                                     "Disabled",            nullptr, kRisk },
            { "Execute Disable Bit",                                 "Disabled",            nullptr, kRisk },
            { "Re-Size BAR Support",                                 "Enabled",             nullptr, kRisk },
        };

        const Tweak kAmd[] = {
            // Power management and clocks
            { "Adaptive S4",                                         "Disabled",   "00", kSafe },
            { "PM L1 SS",                                            "Disabled",   "00", kSafe },
            { "Unused GPP Clocks Off",                               "Disabled",   "00", kSafe },
            { "Clock Power Management(CLKREQ#)",                     "Disabled",   "00", kSafe },
            { "Win7 USB Wake Support",                               "Disabled",   "00", kSafe },
            { "ACP Power Gating",                                    "Disabled",   "00", kSafe },
            { "ACP CLock Gating",                                    "Disabled",   "00", kSafe },
            { "DRAM Latency Enhance",                                "Enabled",    "01", kSafe },
            { "Power Down Enable",                                   "Disabled",   "00", kSafe },
            { "ECO Mode",                                            "Disabled",   "00", kSafe },
            { "LN2 Mode",                                            "Disabled",   "00", kSafe },
            { "LCLK DPM",                                            "Disabled",   "02", kSafe },
            { "LCLK DPM Enhanced PCIe Detection",                    "Disabled",   "02", kSafe },
            { "Global C-state Control",                              "Disabled",   "00", kSafe },
            { "ACPI _CST C1 Declaration",                            "Disabled",   "00", kSafe },
            { "Fast Short REP MOVSB",                                "Enabled",    "01", kSafe },
            { "Enhanced REP MOVSB/STOSB",                            "Enabled",    "01", kSafe },
            { "REP-MOV/STOS Streaming",                              "Enabled",    "01", kSafe },
            { "Determinism Control",                                 "Manual",     "01", kSafe },
            { "Determinism Slider",                                  "Power",      "00", kSafe },
            { "APBDIS",                                              "1",          "01", kSafe },
            { "DF Cstates",                                          "Disabled",   "00", kSafe },
            { "Fixed SOC Pstate",                                    "P0",         "00", kSafe },
            { "SB Clock Spread Spectrum",                            "Disabled",   "00", kSafe },
            { "PPC Adjustment",                                      "PState 0",   "00", kSafe },
            { "ASPM Support",                                        "Disabled",   "00", kSafe },
            { "PCIB Clock Run",                                      "Disabled",   "00", kSafe },
            { "Int. Clk Differential Spread",                        "Disabled",   "00", kSafe },
            { "SATA Slumber State Capability",                       "Disabled",   "00", kSafe },
            { "ACPI SLIT remote relative distance",                  "Near",       "00", kSafe },
            { "PCIe Ten Bit Tag Support",                            "Disable",    "00", kSafe },
            { "NBIO DPM Control",                                    "Manual",     "01", kSafe },
            { "Aggresive SATA Device Sleep Port 0",                  "Disabled",   "00", kSafe },
            { "Aggresive SATA Device Sleep Port 1",                  "Disabled",   "00", kSafe },
            { "Socket1 DevSlp0 Enable",                              "Disabled",   "00", kSafe },
            { "Socket1 DevSlp1 Enable",                              "Disabled",   "00", kSafe },
            { "Chipset Power Saving Features",                       "Disabled",   "00", kSafe },
            { "AB Clock Gating",                                     "Disabled",   "00", kSafe },

            // Dangerous: hardware features some machines need
            { "PX Dynamic Mode",                                     "Disabled",   "00", kRisk },
            { "Discrete GPU's Audio",                                "Disabled",   "00", kRisk },
            { "LAN Power Enable",                                    "Disabled",   "00", kRisk },
            { "ESPI Enable",                                         "Disabled",   "00", kRisk },
            { "S3/Modern Standby Support",                           "Disabled",   "00", kRisk },
            { "SoC/Uncore OC Mode",                                  "Enabled",    "01", kRisk },
            { "CPPC",                                                "Disabled",   "00", kRisk },
            { "CPPC Preferred Cores",                                "Disabled",   "00", kRisk },
            { "PSS Support",                                         "Disabled",   "00", kRisk },
            { "SRIS",                                                "Enable",     "01", kRisk },
            { "ACPI SRAT L3 Cache As NUMA Domain",                   "Enabled",    "01", kRisk },
            { "Periodic Directory Rinse",                            "Disabled",   "00", kRisk },

            // Dangerous: memory mapping
            { "BankGroupSwap",                                       "Disabled",   "00", kRisk },
            { "BankGroupSwapAlt",                                    "Disabled",   "00", kRisk },
            { "Address Hash Bank",                                   "Disabled",   "00", kRisk },
            { "Address Hash CS",                                     "Disabled",   "00", kRisk },
            { "Address Hash Rm",                                     "Disabled",   "00", kRisk },
            { "DRAM map inversion",                                  "Enabled",    "01", kRisk },
            { "SPD Read Optimization",                               "Enabled",    "01", kRisk },
            { "PSP error injection support",                         "False",      "00", kRisk },

            // Dangerous: security, encryption, virtualization
            { "Indirect Branch Prediction Speculation",              "Disabled",   "00", kRisk },
            { "SMEE",                                                "Disabled",   "00", kRisk },
            { "TSME",                                                "Disabled",   "00", kRisk },
            { "CC6 memory region encryption",                        "Disabled",   "00", kRisk },
            { "GMI encryption control",                              "Disabled",   "00", kRisk },
            { "xGMI encryption control",                             "Disabled",   "00", kRisk },
            { "IOMMU",                                               "Disabled",   "00", kRisk },
            { "DMA Protection",                                      "Disabled",   "00", kRisk },
            { "DMAr Support",                                        "Disabled",   "00", kRisk },
            { "ACS Enable",                                          "Disabled",   "00", kRisk },
            { "SR-IOV Support",                                      "Disabled",   "00", kRisk },
            { "SVM Mode",                                            "Disabled",   "00", kRisk },
        };

        // Case, spacing and punctuation differ between boards.
        QString normName(const QString& s)
        {
            QString out;
            out.reserve(s.size());
            for (const QChar c : s)
                if (c.isLetterOrNumber())
                    out.append(c.toLower());
            return out;
        }

        // Labels additionally treat "Disable"/"Disabled" and "Enable"/"Enabled" as one.
        QString normLabel(const QString& s)
        {
            QString out;
            for (const QChar c : s)
                if (c.isLetterOrNumber() || c == QLatin1Char('.') || c == QLatin1Char('%'))
                    out.append(c.toLower());
            if (out == QLatin1String("disable")) return QStringLiteral("disabled");
            if (out == QLatin1String("enable"))  return QStringLiteral("enabled");
            return out;
        }

        bool sameCode(const QString& a, const QString& b)
        {
            const auto parse = [](QString s, bool* ok) {
                s = s.trimmed();
                if (s.startsWith(QLatin1String("0x"), Qt::CaseInsensitive))
                    s = s.mid(2);
                return s.toULongLong(ok, 16);
            };
            bool okA = false, okB = false;
            const auto va = parse(a, &okA);
            const auto vb = parse(b, &okB);
            return okA && okB && va == vb;
        }

        template <size_t N>
        QVector<const Tweak*> select(const Tweak (&table)[N], Level level)
        {
            QVector<const Tweak*> out;
            for (const Tweak& t : table)
                if (level == Level::Everything || !t.dangerous)
                    out.append(&t);
            return out;
        }

        QVector<const Tweak*> listFor(Platform p, Level level)
        {
            switch (p) {
            case Platform::Intel: return select(kIntel, level);
            case Platform::Amd:   return select(kAmd, level);
            default:              return {};
            }
        }

        int findOption(const Entry& e, const Tweak& t)
        {
            const QString want = normLabel(QString::fromLatin1(t.label));
            for (int i = 0; i < e.options.size(); ++i)
                if (normLabel(e.options[i].label) == want)
                    return i;
            if (t.code) {
                const QString code = QString::fromLatin1(t.code);
                for (int i = 0; i < e.options.size(); ++i)
                    if (sameCode(e.options[i].code, code))
                        return i;
            }
            return -1;
        }
    }

    Platform detectPlatform()
    {
        int regs[4] = {};
        __cpuid(regs, 0);
        char vendor[13] = {};
        std::memcpy(vendor + 0, &regs[1], 4);   // EBX
        std::memcpy(vendor + 4, &regs[3], 4);   // EDX
        std::memcpy(vendor + 8, &regs[2], 4);   // ECX
        if (std::strcmp(vendor, "GenuineIntel") == 0) return Platform::Intel;
        if (std::strcmp(vendor, "AuthenticAMD") == 0) return Platform::Amd;
        return Platform::Unknown;
    }

    QString platformName(Platform p)
    {
        switch (p) {
        case Platform::Intel: return QStringLiteral("Intel");
        case Platform::Amd:   return QStringLiteral("AMD");
        default:              return QString();
        }
    }

    int countFor(Platform p, Level level)
    {
        return listFor(p, level).size();
    }

    Plan apply(Document& doc, Platform p, Level level)
    {
        Plan plan;
        const QVector<const Tweak*> list = listFor(p, level);

        QHash<QString, QVector<int>> byName;
        for (int i = 0; i < doc.entries.size(); ++i) {
            // AMISCE won't write back a commented-out entry.
            if (!doc.entries[i].isCommented)
                byName[normName(doc.entries[i].setupQuestion)].append(i);
        }

        for (const Tweak* t : list) {
            const auto hit = byName.constFind(normName(QString::fromLatin1(t->question)));
            if (hit == byName.constEnd()) {
                ++plan.notOnBoard;
                continue;
            }

            for (const int idx : *hit) {
                Entry& e = doc.entries[idx];
                const QString before = e.currentDisplayValue();

                if (e.valueKind == ValueKind::Options) {
                    const int opt = findOption(e, *t);
                    if (opt < 0) {
                        ++plan.noSuchValue;
                        continue;
                    }
                    if (e.options[opt].selected) {
                        ++plan.alreadySet;
                        continue;
                    }
                    for (int i = 0; i < e.options.size(); ++i)
                        e.options[i].selected = (i == opt);
                } else if (e.valueKind == ValueKind::Numeric) {
                    bool ok = false;
                    const int target = QString::fromLatin1(t->label).toInt(&ok);
                    if (!ok) {
                        ++plan.noSuchValue;
                        continue;
                    }
                    if (e.value.toInt() == target) {
                        ++plan.alreadySet;
                        continue;
                    }
                    e.value = QString::number(target);
                } else {
                    ++plan.noSuchValue;
                    continue;
                }

                plan.changes.append({ e.setupQuestion, before,
                                      e.currentDisplayValue(), t->dangerous });
            }
        }
        return plan;
    }
}

