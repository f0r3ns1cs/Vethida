#pragma once

#include <QString>

namespace vth::support
{
    struct BundleResult {
        bool    ok        = false;
        QString path;
        QString error;                // user-facing
        int     fileCount = 0;
        qint64  sizeBytes = 0;
    };

    // Writes a zip of diagnostics for a support reply, overwriting outputPath.
    // Reads a lot from disk, so call it from a worker.
    BundleResult writeBundle(const QString& outputPath);

    QString suggestedFileName();

    // Masks license keys, the home folder, and any INI or JSON value whose name
    // looks like a secret.
    QString redact(const QString& text);
}
