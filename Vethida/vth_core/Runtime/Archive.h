#pragma once

#include <QString>
#include <QStringList>

namespace vth::archive
{
    struct ExtractResult {
        bool    ok = false;
        QString error;
        int         filesExtracted = 0;
        qint64      bytesExtracted = 0;
        QStringList paths;
    };

    // Extracts a .7z or a self-extracting .exe, using the LZMA SDK in lib/lzma.
    ExtractResult extract7z(const QString& archivePath, const QString& outputDir);
}
