#include "RegistryEditor.h"

#include "../Windows/Privileges.h"

#include <QByteArray>
#include <QStringList>

#include <vector>

#include <windows.h>

#pragma comment(lib, "advapi32.lib")

namespace vth::services
{
    namespace
    {
        thread_local long t_lastWriteError = 0;

        bool fail(long code) {
            t_lastWriteError = code;
            return false;
        }

        std::wstring wstr(const QString& s) { return s.toStdWString(); }

        HKEY hiveFor(const QString& path, QString& subkeyOut) {
            const int slash = path.indexOf(QLatin1Char('\\'));
            const QString hive = slash < 0 ? path : path.left(slash);
            subkeyOut = slash < 0 ? QString() : path.mid(slash + 1);
            if (hive.compare(QStringLiteral("HKLM"), Qt::CaseInsensitive) == 0) return HKEY_LOCAL_MACHINE;
            if (hive.compare(QStringLiteral("HKCU"), Qt::CaseInsensitive) == 0) return HKEY_CURRENT_USER;
            if (hive.compare(QStringLiteral("HKCR"), Qt::CaseInsensitive) == 0) return HKEY_CLASSES_ROOT;
            if (hive.compare(QStringLiteral("HKU"), Qt::CaseInsensitive) == 0)  return HKEY_USERS;
            return nullptr;
        }

        bool isHklm(const QString& path) {
            return path.startsWith(QStringLiteral("HKLM"), Qt::CaseInsensitive);
        }

        DWORD win32Kind(RegistryValueKind k) {
            switch (k) {
                case RegistryValueKind::String:       return REG_SZ;
                case RegistryValueKind::ExpandString: return REG_EXPAND_SZ;
                case RegistryValueKind::Binary:       return REG_BINARY;
                case RegistryValueKind::DWord:        return REG_DWORD;
                case RegistryValueKind::MultiString:  return REG_MULTI_SZ;
                case RegistryValueKind::QWord:        return REG_QWORD;
                case RegistryValueKind::None:         return REG_NONE;
            }
            return REG_NONE;
        }
    }

    QVariant RegistryEditor::getValue(const QString& keyPath, const QString& valueName) const {
        QString sub;
        HKEY hive = hiveFor(keyPath, sub);
        if (!hive) return {};
        HKEY key = nullptr;
        if (RegOpenKeyExW(hive, wstr(sub).c_str(), 0, KEY_READ | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS)
            return {};

        DWORD type = 0, size = 0;
        const LSTATUS probe = RegQueryValueExW(key, wstr(valueName).c_str(), nullptr, &type, nullptr, &size);
        if (probe != ERROR_SUCCESS) {
            RegCloseKey(key);
            return {};
        }

        QVariant result;
        std::vector<BYTE> buf(size ? size : 1, 0);
        DWORD got = size;
        if (RegQueryValueExW(key, wstr(valueName).c_str(), nullptr, &type, buf.data(), &got) == ERROR_SUCCESS) {
            switch (type) {
                case REG_DWORD:
                    if (got >= sizeof(DWORD))
                        result = QVariant::fromValue<uint>(*reinterpret_cast<const DWORD*>(buf.data()));
                    break;
                case REG_QWORD:
                    if (got >= sizeof(quint64))
                        result = QVariant::fromValue<qulonglong>(*reinterpret_cast<const quint64*>(buf.data()));
                    break;
                case REG_SZ:
                case REG_EXPAND_SZ:
                    result = QString::fromWCharArray(reinterpret_cast<const wchar_t*>(buf.data()),
                                                     static_cast<int>(got / sizeof(wchar_t)))
                                 .split(QChar(u'\0')).value(0);
                    break;
                case REG_MULTI_SZ: {
                    const QString joined = QString::fromWCharArray(
                        reinterpret_cast<const wchar_t*>(buf.data()), static_cast<int>(got / sizeof(wchar_t)));
                    QStringList parts = joined.split(QChar(u'\0'), Qt::SkipEmptyParts);
                    result = parts;
                    break;
                }
                default:
                    result = QByteArray(reinterpret_cast<const char*>(buf.data()), static_cast<int>(got));
                    break;
            }
        }
        RegCloseKey(key);
        return result;
    }

    bool RegistryEditor::setValue(const QString& keyPath, const QString& valueName,
                                  const QVariant& value, RegistryValueKind kind) {
        if (isHklm(keyPath) && !vth::win::isElevated()) return fail(ERROR_ELEVATION_REQUIRED);
        QString sub;
        HKEY hive = hiveFor(keyPath, sub);
        if (!hive) return fail(ERROR_INVALID_PARAMETER);
        HKEY key = nullptr;
        DWORD disp = 0;
        if (const LSTATUS open = RegCreateKeyExW(hive, wstr(sub).c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE,
                                                 KEY_WRITE | KEY_READ | KEY_WOW64_64KEY, nullptr, &key, &disp);
            open != ERROR_SUCCESS)
            return fail(open);

        LSTATUS rc = ERROR_INVALID_PARAMETER;
        const DWORD type = win32Kind(kind);
        if (type == REG_DWORD) {
            const DWORD v = value.toUInt();
            rc = RegSetValueExW(key, wstr(valueName).c_str(), 0, REG_DWORD,
                                reinterpret_cast<const BYTE*>(&v), sizeof(v));
        } else if (type == REG_QWORD) {
            const quint64 v = value.toULongLong();
            rc = RegSetValueExW(key, wstr(valueName).c_str(), 0, REG_QWORD,
                                reinterpret_cast<const BYTE*>(&v), sizeof(v));
        } else if (type == REG_BINARY) {
            const QByteArray b = value.toByteArray();
            rc = RegSetValueExW(key, wstr(valueName).c_str(), 0, REG_BINARY,
                                reinterpret_cast<const BYTE*>(b.constData()), static_cast<DWORD>(b.size()));
        } else if (type == REG_MULTI_SZ) {
            // Each string NUL-terminated, then one more NUL. An empty list is a
            // lone terminator, which is a valid empty multi-string.
            const QStringList parts = value.typeId() == QMetaType::QStringList
                ? value.toStringList() : QStringList{ value.toString() };
            std::wstring w;
            for (const QString& p : parts) {
                w += wstr(p);
                w.push_back(L'\0');
            }
            w.push_back(L'\0');
            rc = RegSetValueExW(key, wstr(valueName).c_str(), 0, REG_MULTI_SZ,
                                reinterpret_cast<const BYTE*>(w.data()),
                                static_cast<DWORD>(w.size() * sizeof(wchar_t)));
        } else {  // REG_SZ / REG_EXPAND_SZ
            const std::wstring w = wstr(value.toString());
            rc = RegSetValueExW(key, wstr(valueName).c_str(), 0, type == REG_EXPAND_SZ ? REG_EXPAND_SZ : REG_SZ,
                                reinterpret_cast<const BYTE*>(w.c_str()),
                                static_cast<DWORD>((w.size() + 1) * sizeof(wchar_t)));
        }
        RegCloseKey(key);
        return rc == ERROR_SUCCESS ? true : fail(rc);
    }

    bool RegistryEditor::deleteValue(const QString& keyPath, const QString& valueName) {
        if (isHklm(keyPath) && !vth::win::isElevated()) return fail(ERROR_ELEVATION_REQUIRED);
        QString sub;
        HKEY hive = hiveFor(keyPath, sub);
        if (!hive) return false;
        HKEY key = nullptr;
        if (RegOpenKeyExW(hive, wstr(sub).c_str(), 0, KEY_SET_VALUE | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS)
            return true;  // key gone => value gone
        const LSTATUS rc = RegDeleteValueW(key, wstr(valueName).c_str());
        RegCloseKey(key);
        return (rc == ERROR_SUCCESS || rc == ERROR_FILE_NOT_FOUND) ? true : fail(rc);
    }

    bool RegistryEditor::valueExists(const QString& keyPath, const QString& valueName) const {
        QString sub;
        HKEY hive = hiveFor(keyPath, sub);
        if (!hive) return false;
        HKEY key = nullptr;
        if (RegOpenKeyExW(hive, wstr(sub).c_str(), 0, KEY_READ | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS)
            return false;
        const LSTATUS rc = RegQueryValueExW(key, wstr(valueName).c_str(), nullptr, nullptr, nullptr, nullptr);
        RegCloseKey(key);
        return rc == ERROR_SUCCESS;
    }

    std::vector<QString> RegistryEditor::enumerateValueNames(const QString& keyPath) const {
        std::vector<QString> out;
        QString sub;
        HKEY hive = hiveFor(keyPath, sub);
        if (!hive) return out;
        HKEY key = nullptr;
        if (RegOpenKeyExW(hive, wstr(sub).c_str(), 0, KEY_READ | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS)
            return out;
        for (DWORD i = 0;; ++i) {
            wchar_t name[512] = {};
            DWORD len = 512;
            const LSTATUS rc = RegEnumValueW(key, i, name, &len, nullptr, nullptr, nullptr, nullptr);
            if (rc == ERROR_NO_MORE_ITEMS) break;
            if (rc == ERROR_SUCCESS) out.push_back(QString::fromWCharArray(name, static_cast<int>(len)));
            else if (rc != ERROR_MORE_DATA) break;
        }
        RegCloseKey(key);
        return out;
    }

    std::vector<QString> RegistryEditor::enumerateSubKeys(const QString& keyPath) const {
        std::vector<QString> out;
        QString sub;
        HKEY hive = hiveFor(keyPath, sub);
        if (!hive) return out;
        HKEY key = nullptr;
        if (RegOpenKeyExW(hive, wstr(sub).c_str(), 0,
                          KEY_READ | KEY_ENUMERATE_SUB_KEYS | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS)
            return out;
        for (DWORD i = 0;; ++i) {
            wchar_t name[256] = {};
            DWORD len = 256;
            const LSTATUS rc = RegEnumKeyExW(key, i, name, &len, nullptr, nullptr, nullptr, nullptr);
            if (rc == ERROR_NO_MORE_ITEMS) break;
            if (rc == ERROR_SUCCESS) out.push_back(QString::fromWCharArray(name, static_cast<int>(len)));
        }
        RegCloseKey(key);
        return out;
    }

    bool RegistryEditor::keyExists(const QString& keyPath) const {
        QString sub;
        HKEY hive = hiveFor(keyPath, sub);
        if (!hive) return false;
        HKEY key = nullptr;
        if (RegOpenKeyExW(hive, wstr(sub).c_str(), 0, KEY_READ | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS)
            return false;
        RegCloseKey(key);
        return true;
    }

    bool RegistryEditor::createKey(const QString& keyPath) {
        if (isHklm(keyPath) && !vth::win::isElevated()) return fail(ERROR_ELEVATION_REQUIRED);
        QString sub;
        HKEY hive = hiveFor(keyPath, sub);
        if (!hive) return fail(ERROR_INVALID_PARAMETER);
        HKEY key = nullptr;
        DWORD disp = 0;
        if (const LSTATUS open = RegCreateKeyExW(hive, wstr(sub).c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE,
                                                 KEY_WRITE | KEY_READ | KEY_WOW64_64KEY, nullptr, &key, &disp);
            open != ERROR_SUCCESS)
            return fail(open);
        RegCloseKey(key);
        return true;
    }

    long lastRegistryWriteError() {
        return t_lastWriteError;
    }

    bool RegistryEditor::deleteKeyTree(const QString& keyPath) {
        if (isHklm(keyPath) && !vth::win::isElevated()) return fail(ERROR_ELEVATION_REQUIRED);
        QString sub;
        HKEY hive = hiveFor(keyPath, sub);
        if (!hive) return false;
        // A path with no subkey would delete everything under the hive.
        if (QString(sub).remove(QLatin1Char('\\')).trimmed().isEmpty())
            return fail(ERROR_INVALID_PARAMETER);
        const LSTATUS rc = RegDeleteTreeW(hive, wstr(sub).c_str());
        return (rc == ERROR_SUCCESS || rc == ERROR_FILE_NOT_FOUND) ? true : fail(rc);
    }
}
