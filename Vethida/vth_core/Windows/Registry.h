#pragma once

#include <QString>
#include <QVector>

#include <cstdint>
#include <optional>

namespace vth::win::reg
{

    enum class Hive { LocalMachine, CurrentUser };

    enum class WriteResult {
        Ok,
        AccessDenied,
        OtherError,
    };

    struct RegOp {
        Hive    hive;
        QString subkey;
        QString valueName;
        std::uint32_t targetValue;
        std::optional<std::uint32_t> defaultValue;
    };

    enum class State {
        Unknown,
        Applied,
        Default,
        Mixed,
    };

    enum class Severity { Low, Medium, Caution };

    struct Toggle {
        QString         id;
        QString         displayName;
        Severity        severity;
        QString         summary;
        QString         impact;
        QVector<RegOp>  ops;
    };

    std::optional<std::uint32_t> readDword(Hive hive, const QString& subkey, const QString& valueName);

    std::optional<QString> readAsText(Hive hive, const QString& subkey,
                                      const QString& valueName);

    WriteResult writeDword(Hive hive, const QString& subkey, const QString& valueName, std::uint32_t value);
    WriteResult deleteValue(Hive hive, const QString& subkey, const QString& valueName);

    // Reads on an open HKEY, passed as void* to keep windows.h out of this header.
    QString       readStringAt(void* key, const wchar_t* valueName, bool expand = false);
    std::uint32_t readDwordAt (void* key, const wchar_t* valueName, std::uint32_t fallback = 0);

    WriteResult worst(WriteResult a, WriteResult b) noexcept;

    State readState(const QVector<RegOp>& ops);

    WriteResult apply(const QVector<RegOp>& ops);
    WriteResult restore(const QVector<RegOp>& ops);

    QString severityLabel(Severity s);
    QString stateLabel(State s);
    QString writeResultLabel(WriteResult r);

}
