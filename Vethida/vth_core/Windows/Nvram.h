#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

// The firmware settings editor, on top of AMI's SCEWIN tool.
namespace vth::win::nvram
{
    enum class ValueKind {
        Options,    // a list of [code] labels
        Numeric,    // <integer>
        Text,       // "string"
        Raw,        // anything else, kept as it is
    };

    struct Option {
        QString code;
        QString label;
        bool    selected = false;
    };

    struct Entry {
        QString          setupQuestion;
        QString          helpString;
        QString          token;
        QString          offset;
        QString          width;
        bool             isCommented   = false;
        ValueKind        valueKind     = ValueKind::Options;
        QVector<Option>  options;
        QString          value;          // for Numeric, Text and Raw
        QString          originalValue;
        int              sourceLine     = 0;

        QString currentValue() const;
        QString currentDisplayValue() const;
        bool    isModified() const noexcept    { return currentValue() != originalValue; }
    void    revertToOriginal();
    };

    struct FileHeader {
        QString fileName;
        QString createdDate;
        QString toolVersion;
        QString crc32;
    };

    struct Document {
        FileHeader      header;
        QVector<Entry>  entries;
        QStringList     rawLines;
        QString         sourcePath;
    };

    // Returns an empty document if the file can't be read.
    Document parseFile(const QString& filePath);
    Document parseLines(const QStringList& lines, const QString& sourcePath = {});

    // Patches modified entries into rawLines. Everything else stays as it was.
    QString  serialize(const Document& doc);

    struct ScewinPaths {
        QString exePath;
        QString fldrvPath;
        QString gendrvPath;
        bool    available = false;
    };

    // The nvram/toolDirectory setting first, then tools/ami-scewin in the install
    // directory. SCEWIN isn't included in this repository.
    ScewinPaths locateScewin();

    // AMISCE can only read AMI firmware. Checked first, so an unsupported board
    // doesn't load a kernel driver for nothing.
    bool isAmiFirmware();

    struct RunResult {
        bool    success    = false;
        int     exitCode   = -1;
        QString message;
    };

    RunResult exportSnapshot(const ScewinPaths& paths, const QString& outputPath);
    RunResult importSnapshot(const ScewinPaths& paths, const QString& inputPath);

    // The first export taken on this PC, from before any changes.
    QString  baselinePath();
    bool     hasBaseline();

    // Never overwrites an existing baseline.
    bool     saveBaselineIfAbsent(const QString& exportedPath);
}
