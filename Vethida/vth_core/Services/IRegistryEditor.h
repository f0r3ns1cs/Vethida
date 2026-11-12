#pragma once

#include <QtCore/QString>
#include <QtCore/QVariant>
#include <vector>

namespace vth::services
{
    // Values match the Win32 REG_* type codes.
    enum class RegistryValueKind {
        None = 0,
        String = 1,        // REG_SZ
        ExpandString = 2,  // REG_EXPAND_SZ
        Binary = 3,        // REG_BINARY
        DWord = 4,         // REG_DWORD
        MultiString = 7,   // REG_MULTI_SZ
        QWord = 11,        // REG_QWORD
    };

    // Paths use the HIVE\Subkey form, e.g. HKLM\SOFTWARE\Vethida.
    class IRegistryEditor {
    public:
        QVariant getValueOrDefault(const QString& keyPath,
                                   const QString& valueName,
                                   const QVariant& defaultValue) const
        {
            const QVariant raw = getValue(keyPath, valueName);
            return raw.isValid() ? raw : defaultValue;
        }
        virtual ~IRegistryEditor() = default;

        virtual QVariant getValue(const QString& keyPath, const QString& valueName) const = 0;

        virtual bool setValue(const QString& keyPath,
                              const QString& valueName,
                              const QVariant& value,
                              RegistryValueKind kind) = 0;

        // True if the value is gone afterwards, including when it never existed.
        virtual bool deleteValue(const QString& keyPath, const QString& valueName) = 0;

        virtual bool valueExists(const QString& keyPath, const QString& valueName) const = 0;

        // Both empty if the key is missing or unreadable.
        virtual std::vector<QString> enumerateValueNames(const QString& keyPath) const = 0;
        virtual std::vector<QString> enumerateSubKeys(const QString& keyPath) const = 0;

        virtual bool keyExists(const QString& keyPath) const = 0;

        virtual bool createKey(const QString& keyPath) = 0;

        // True if gone afterwards. Refuses a path with no subkey, such as a bare hive.
        virtual bool deleteKeyTree(const QString& keyPath) = 0;
    };

    // Win32 status of this thread's last failed write. Not reset by a success.
    long lastRegistryWriteError();
}
