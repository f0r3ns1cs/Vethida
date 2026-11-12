#pragma once

#include "IRegistryEditor.h"

namespace vth::services
{
    // Win32 IRegistryEditor, always on the 64-bit registry view. HKLM writes need
    // elevation and return false without it.
    class RegistryEditor final : public IRegistryEditor {
    public:
        QVariant getValue(const QString& keyPath, const QString& valueName) const override;

        bool setValue(const QString& keyPath,
                      const QString& valueName,
                      const QVariant& value,
                      RegistryValueKind kind) override;

        bool deleteValue(const QString& keyPath, const QString& valueName) override;
        bool valueExists(const QString& keyPath, const QString& valueName) const override;
        std::vector<QString> enumerateValueNames(const QString& keyPath) const override;
        std::vector<QString> enumerateSubKeys(const QString& keyPath) const override;
        bool keyExists(const QString& keyPath) const override;
        bool createKey(const QString& keyPath) override;
        bool deleteKeyTree(const QString& keyPath) override;
    };
}
