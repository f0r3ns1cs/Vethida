#pragma once

#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVector>

#include <optional>

// Native helpers for dynamic tweaks: hardware that's only known at runtime, and
// state a registry write can't express.
namespace vth::tweaks::devices
{
    // A saved map of strings per tweak id.
    QMap<QString, QString> loadSnapshot(const QString& tweakId);
    void                   saveSnapshot(const QString& tweakId, const QMap<QString, QString>& kv);
    void                   clearSnapshot(const QString& tweakId);

    // Paths in HIVE\Subkey form. HKLM writes need elevation.
    std::optional<quint32> regReadDword (const QString& path, const QString& name);
    std::optional<QString> regReadString(const QString& path, const QString& name);
    bool regWriteDword (const QString& path, const QString& name, quint32 value);
    bool regWriteString(const QString& path, const QString& name, const QString& value);
    bool regDeleteValue(const QString& path, const QString& name);
    QStringList regSubKeys(const QString& path);

    struct NicAdapter {
        QString classKeyPath;   // HKLM\SYSTEM\...\Class\{4d36e972-...}\NNNN
        QString instanceGuid;   // NetCfgInstanceId
        QString description;
    };
    QVector<NicAdapter> enumeratePhysicalNics();

    // Class keys of the GPUs from a vendor, e.g. "VEN_1002" for AMD.
    QStringList enumerateGpuClassKeys(const QString& venToken);

    struct PciInstance {
        QString instanceKeyPath;   // HKLM\SYSTEM\CurrentControlSet\Enum\PCI\<dev>\<inst>
        QString service;           // lowercased, e.g. "storahci"
        QString classGuid;         // lowercased
    };
    QVector<PciInstance> enumeratePciInstances();

    // On the active power scheme.
    std::optional<quint32> powercfgQueryAcIndex(const QString& subAlias, const QString& settingAlias);
    bool powercfgSetAcIndex(const QString& subAlias, const QString& settingAlias, quint32 value);

    bool runHidden(const QString& program, const QStringList& args, QString* out = nullptr,
                int timeoutMs = 60000);
    bool runPowershell(const QString& script, QString* out = nullptr, int timeoutMs = 120000);

    std::optional<bool> memoryCompressionEnabled();
    bool setMemoryCompression(bool enabled);

    std::optional<bool> pagefileIsAutomatic();
    bool setFixedPagefile();        // sized to RAM, between 4096 and 16384 MB
    bool setAutomaticPagefile();

    std::optional<quint64> physicalRamKb();
}

namespace vth::tweaks::devices
{
    std::optional<QString> bcdReadElement(const QString& element);
    bool bcdSetElement(const QString& element, const QString& value);
    bool bcdDeleteElement(const QString& element);
}
