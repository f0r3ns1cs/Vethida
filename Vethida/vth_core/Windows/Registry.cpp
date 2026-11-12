#include "Registry.h"

#include "Privileges.h"

#include <QObject>
#include <QStringLiteral>

#include <windows.h>

#include <cwchar>
#include <string>
#include <vector>

namespace vth::win::reg
{
    namespace
    {
        HKEY rootFor(Hive h) noexcept {
            return (h == Hive::LocalMachine) ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
        }

        WriteResult mapError(LONG r) noexcept {
            if (r == ERROR_SUCCESS)       return WriteResult::Ok;
            if (r == ERROR_ACCESS_DENIED) return WriteResult::AccessDenied;
            return WriteResult::OtherError;
        }

        bool touchesLocalMachine(const QVector<RegOp>& ops) noexcept {
            for (const auto& op : ops) {
                if (op.hive == Hive::LocalMachine) return true;
            }
            return false;
        }
    }

    std::optional<std::uint32_t> readDword(Hive hive, const QString& subkey, const QString& valueName) {
        HKEY key = nullptr;
        const std::wstring sub = subkey.toStdWString();
        if (RegOpenKeyExW(rootFor(hive), sub.c_str(), 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) {
            return std::nullopt;
        }
        DWORD type = 0, data = 0, size = sizeof(data);
        const std::wstring name = valueName.toStdWString();
        const LONG r = RegQueryValueExW(key, name.c_str(), nullptr, &type,
                                        reinterpret_cast<LPBYTE>(&data), &size);
        RegCloseKey(key);
        if (r != ERROR_SUCCESS || type != REG_DWORD) return std::nullopt;
        return static_cast<std::uint32_t>(data);
    }

    std::optional<QString> readAsText(Hive hive, const QString& subkey,
                                      const QString& valueName) {
        HKEY key = nullptr;
        const std::wstring sub = subkey.toStdWString();
        if (RegOpenKeyExW(rootFor(hive), sub.c_str(), 0, KEY_QUERY_VALUE, &key)
            != ERROR_SUCCESS) {
            return std::nullopt;
        }

        const std::wstring name = valueName.toStdWString();
        DWORD type = 0, bytes = 0;
        if (RegQueryValueExW(key, name.c_str(), nullptr, &type, nullptr, &bytes)
            != ERROR_SUCCESS) {
            RegCloseKey(key);
            return std::nullopt;
        }

        std::optional<QString> out;
        switch (type) {
            case REG_DWORD:
                out = QString::number(readDwordAt(key, name.c_str()));
                break;
            case REG_QWORD: {
                quint64 v = 0;
                DWORD   size = sizeof(v);
                if (RegQueryValueExW(key, name.c_str(), nullptr, &type,
                                     reinterpret_cast<LPBYTE>(&v), &size)
                    == ERROR_SUCCESS) {
                    out = QString::number(v);
                }
                break;
            }
            case REG_SZ:
            case REG_EXPAND_SZ:
                out = readStringAt(key, name.c_str());
                break;
            default:
                out = bytes == 1 ? QObject::tr("1 byte")
                                 : QObject::tr("%1 bytes").arg(bytes);
                break;
        }

        RegCloseKey(key);
        return out;
    }

    WriteResult writeDword(Hive hive, const QString& subkey, const QString& valueName, std::uint32_t value) {
        HKEY key = nullptr;
        const std::wstring sub = subkey.toStdWString();
        LONG r = RegCreateKeyExW(rootFor(hive), sub.c_str(), 0, nullptr,
                                 REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, nullptr, &key, nullptr);
        if (r != ERROR_SUCCESS) return mapError(r);

        const std::wstring name = valueName.toStdWString();
        const DWORD payload = static_cast<DWORD>(value);
        r = RegSetValueExW(key, name.c_str(), 0, REG_DWORD,
                           reinterpret_cast<const BYTE*>(&payload), sizeof(payload));
        RegCloseKey(key);
        return mapError(r);
    }

    WriteResult deleteValue(Hive hive, const QString& subkey, const QString& valueName) {
        HKEY key = nullptr;
        const std::wstring sub = subkey.toStdWString();
        LONG r = RegOpenKeyExW(rootFor(hive), sub.c_str(), 0, KEY_SET_VALUE, &key);
        if (r == ERROR_FILE_NOT_FOUND) return WriteResult::Ok;
        if (r != ERROR_SUCCESS)        return mapError(r);

        const std::wstring name = valueName.toStdWString();
        r = RegDeleteValueW(key, name.c_str());
        RegCloseKey(key);
        if (r == ERROR_FILE_NOT_FOUND) return WriteResult::Ok;
        return mapError(r);
    }

    QString readStringAt(void* key, const wchar_t* valueName, bool expand) {
        HKEY k = static_cast<HKEY>(key);
        DWORD type = 0, bytes = 0;
        if (RegQueryValueExW(k, valueName, nullptr, &type, nullptr, &bytes) != ERROR_SUCCESS) {
            return {};
        }
        if (type != REG_SZ && type != REG_EXPAND_SZ) return {};
        if (bytes == 0) return {};

        std::vector<wchar_t> buf(bytes / sizeof(wchar_t) + 1, L'\0');
        DWORD read = static_cast<DWORD>(buf.size() * sizeof(wchar_t));
        if (RegQueryValueExW(k, valueName, nullptr, &type,
                             reinterpret_cast<LPBYTE>(buf.data()), &read) != ERROR_SUCCESS) {
            return {};
        }
        QString s = QString::fromWCharArray(
            buf.data(), static_cast<int>(wcsnlen(buf.data(), buf.size())));

        if (expand && type == REG_EXPAND_SZ && !s.isEmpty()) {
            const std::wstring in = s.toStdWString();
            const DWORD needed = ExpandEnvironmentStringsW(in.c_str(), nullptr, 0);
            if (needed > 0) {
                std::wstring out(needed, L'\0');
                const DWORD wrote = ExpandEnvironmentStringsW(in.c_str(), out.data(), needed);
                if (wrote > 0 && wrote <= needed) {
                    out.resize(wrote - 1);
                    s = QString::fromStdWString(out);
                }
            }
        }
        return s;
    }

    std::uint32_t readDwordAt(void* key, const wchar_t* valueName, std::uint32_t fallback) {
        HKEY k = static_cast<HKEY>(key);
        DWORD type = 0, value = 0, size = sizeof(value);
        if (RegQueryValueExW(k, valueName, nullptr, &type,
                             reinterpret_cast<LPBYTE>(&value), &size) != ERROR_SUCCESS
            || type != REG_DWORD) {
            return fallback;
        }
        return static_cast<std::uint32_t>(value);
    }

    WriteResult worst(WriteResult a, WriteResult b) noexcept {
        if (a == WriteResult::AccessDenied || b == WriteResult::AccessDenied) return WriteResult::AccessDenied;
        if (a == WriteResult::OtherError   || b == WriteResult::OtherError)   return WriteResult::OtherError;
        return WriteResult::Ok;
    }

    State readState(const QVector<RegOp>& ops) {
        if (ops.isEmpty()) return State::Unknown;

        int applied = 0;
        int atDefault = 0;
        for (const auto& op : ops) {
            const std::optional<std::uint32_t> value = readDword(op.hive, op.subkey, op.valueName);
            if (value) {
                if (*value == op.targetValue) ++applied;
                else if (op.defaultValue && *value == *op.defaultValue) ++atDefault;
            } else if (!op.defaultValue) {
                ++atDefault;   // delete-on-restore op with no value present is the default
            }
        }
        if (applied   == ops.size()) return State::Applied;
        if (atDefault == ops.size()) return State::Default;
        return State::Mixed;
    }

    WriteResult apply(const QVector<RegOp>& ops) {
        if (touchesLocalMachine(ops) && !isElevated()) return WriteResult::AccessDenied;
        WriteResult agg = WriteResult::Ok;
        for (const auto& op : ops) {
            agg = worst(agg, writeDword(op.hive, op.subkey, op.valueName, op.targetValue));
        }
        return agg;
    }

    WriteResult restore(const QVector<RegOp>& ops) {
        if (touchesLocalMachine(ops) && !isElevated()) return WriteResult::AccessDenied;
        WriteResult agg = WriteResult::Ok;
        for (const auto& op : ops) {
            if (op.defaultValue) {
                agg = worst(agg, writeDword(op.hive, op.subkey, op.valueName, *op.defaultValue));
            } else {
                agg = worst(agg, deleteValue(op.hive, op.subkey, op.valueName));
            }
        }
        return agg;
    }

    QString severityLabel(Severity s) {
        switch (s) {
            case Severity::Low:     return QStringLiteral("Low risk");
            case Severity::Medium:  return QStringLiteral("Medium risk");
            case Severity::Caution: return QStringLiteral("Use caution");
        }
        return {};
    }

    QString stateLabel(State s) {
        switch (s) {
            case State::Applied: return QStringLiteral("Applied");
            case State::Default: return QStringLiteral("Windows default");
            case State::Mixed:   return QStringLiteral("Mixed");
            case State::Unknown: break;
        }
        return QStringLiteral("Unknown");
    }

    QString writeResultLabel(WriteResult r) {
        switch (r) {
            case WriteResult::Ok:           return QStringLiteral("Change applied.");
            case WriteResult::AccessDenied: return QStringLiteral(
                "Windows protects this setting. A Group Policy, security "
                "software or Windows itself holds it.");
            case WriteResult::OtherError:   return QStringLiteral("Windows could not save this setting.");
        }
        return {};
    }

}
