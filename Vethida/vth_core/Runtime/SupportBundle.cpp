// The LZMA SDK's global Int32 and Int64 typedefs are shadowed by locals in Qt.
#pragma warning(disable: 4459)

#include "SupportBundle.h"

#include "Paths.h"
#include "Settings.h"
#include "../Logger.h"
#include "../SystemInfo.h"
#include "../Services/SecureStorage.h"
#include "../../version.h"

#include <QByteArray>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QStringLiteral>
#include <QSysInfo>

#include <utility>
#include <vector>

#include "../../lib/lzma/7zCrc.h"

namespace vth::support
{
    namespace
    {
        // Stored without compression, because a bundle is only a few megabytes.
        constexpr quint16 kMethodStore     = 0;
        constexpr quint16 kVersionNeeded   = 20;   // base format, no zip64
        constexpr quint32 kSigLocalHeader  = 0x04034b50;
        constexpr quint32 kSigCentralDir   = 0x02014b50;
        constexpr quint32 kSigEndOfCentral = 0x06054b50;

        // The format carries 32-bit sizes.
        constexpr qint64 kMaxTotalBytes = 200LL * 1024 * 1024;

        void put16(QByteArray& out, quint16 v) {
            out.append(char(v & 0xFF));
            out.append(char((v >> 8) & 0xFF));
        }

        void put32(QByteArray& out, quint32 v) {
            out.append(char(v & 0xFF));
            out.append(char((v >> 8) & 0xFF));
            out.append(char((v >> 16) & 0xFF));
            out.append(char((v >> 24) & 0xFF));
        }

        quint32 crc32Of(const QByteArray& data) {
            static const bool tableReady = [] { CrcGenerateTable(); return true; }();
            Q_UNUSED(tableReady);
            return quint32(CrcCalc(data.constData(), size_t(data.size())));
        }

        // Zip carries MS-DOS dates, from 1980, with two-second resolution.
        void dosStamp(const QDateTime& when, quint16& outTime, quint16& outDate) {
            const QDate d = when.date();
            const QTime t = when.time();
            const int year = d.year() < 1980 ? 1980 : d.year();
            outDate = quint16(((year - 1980) << 9) | (d.month() << 5) | d.day());
            outTime = quint16((t.hour() << 11) | (t.minute() << 5) | (t.second() / 2));
        }

        struct Entry {
            QByteArray name;      // forward slashes
            QByteArray data;
            quint32    crc = 0;
            quint32    offset = 0;
        };

        QByteArray buildZip(const std::vector<Entry>& entries, const QDateTime& when) {
            quint16 dosTime = 0;
            quint16 dosDate = 0;
            dosStamp(when, dosTime, dosDate);

            QByteArray out;
            std::vector<quint32> offsets;
            offsets.reserve(entries.size());

            for (const Entry& e : entries) {
                offsets.push_back(quint32(out.size()));
                put32(out, kSigLocalHeader);
                put16(out, kVersionNeeded);
                put16(out, 0);                      // flags
                put16(out, kMethodStore);
                put16(out, dosTime);
                put16(out, dosDate);
                put32(out, e.crc);
                put32(out, quint32(e.data.size())); // compressed == uncompressed
                put32(out, quint32(e.data.size()));
                put16(out, quint16(e.name.size()));
                put16(out, 0);                      // no extra field
                out.append(e.name);
                out.append(e.data);
            }

            const quint32 centralStart = quint32(out.size());
            for (size_t i = 0; i < entries.size(); ++i) {
                const Entry& e = entries[i];
                put32(out, kSigCentralDir);
                put16(out, kVersionNeeded);         // version made by
                put16(out, kVersionNeeded);         // version needed
                put16(out, 0);
                put16(out, kMethodStore);
                put16(out, dosTime);
                put16(out, dosDate);
                put32(out, e.crc);
                put32(out, quint32(e.data.size()));
                put32(out, quint32(e.data.size()));
                put16(out, quint16(e.name.size()));
                put16(out, 0);                      // extra
                put16(out, 0);                      // comment
                put16(out, 0);                      // disk number start
                put16(out, 0);                      // internal attributes
                put32(out, 0);                      // external attributes
                put32(out, offsets[i]);
                out.append(e.name);
            }
            const quint32 centralSize = quint32(out.size()) - centralStart;

            put32(out, kSigEndOfCentral);
            put16(out, 0);                          // this disk
            put16(out, 0);                          // disk with central directory
            put16(out, quint16(entries.size()));
            put16(out, quint16(entries.size()));
            put32(out, centralSize);
            put32(out, centralStart);
            put16(out, 0);                          // no archive comment
            return out;
        }

        void addText(std::vector<Entry>& entries, const char* name, const QString& text) {
            Entry e;
            e.name = QByteArray(name);
            e.data = redact(text).toUtf8();
            e.crc  = crc32Of(e.data);
            entries.push_back(std::move(e));
        }

        // A file that can't be read is noted in the bundle, because that's worth
        // knowing too.
        void addFile(std::vector<Entry>& entries, const QString& archiveName,
                     const QString& diskPath) {
            QFile f(diskPath);
            if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                addText(entries, archiveName.toUtf8().constData(),
                        QStringLiteral("Not readable: %1\n").arg(f.errorString()));
                return;
            }
            addText(entries, archiveName.toUtf8().constData(), QString::fromUtf8(f.readAll()));
        }

        QString systemSection() {
            const vth::win::SystemInfo si = vth::win::collect();
            QString s;
            s += QStringLiteral("Vethida %1\n").arg(QLatin1String(VETHIDA_VERSION_STRING));
            s += QStringLiteral("Generated  %1\n\n")
                     .arg(QDateTime::currentDateTime().toString(Qt::ISODate));
            s += QStringLiteral("OS         %1 (%2)\n").arg(si.osEdition, si.osBuild);
            s += QStringLiteral("Kernel     %1\n").arg(QSysInfo::kernelVersion());
            s += QStringLiteral("Arch       %1\n").arg(QSysInfo::currentCpuArchitecture());
            s += QStringLiteral("CPU        %1 (%2, %3 logical)\n")
                     .arg(vth::win::tidyHardwareName(si.cpuBrand), si.cpuVendor)
                     .arg(si.cpuLogicalProcs);
            s += QStringLiteral("RAM        %1 MB\n").arg(si.ramTotalBytes / (1024 * 1024));
            s += QStringLiteral("GPU        %1\n")
                     .arg(vth::win::tidyHardwareName(si.gpuName));
            // Computer and user names are left out.
            return s;
        }

        QString manifestSection(int fileCount) {
            QString s;
            s += QStringLiteral("Vethida support bundle\n");
            s += QStringLiteral("======================\n\n");
            s += QStringLiteral(
                "Generated by Vethida at the customer's request so a support reply has\n"
                "something concrete to read. It contains:\n\n"
                "  system.txt        the machine's hardware and Windows build\n"
                "  settings.ini      Vethida's own preferences\n"
                "  tweak-state.json  every change Vethida has applied and the value it\n"
                "                    recorded to put back on revert\n"
                "  logs/             Vethida's log files\n"
                "  crashes.txt       a list of crash files, not the files themselves\n\n"
                "License keys are masked throughout. Your Windows account name, your\n"
                "computer name and the contents of any crash dump are not included.\n"
                "Nothing here is sent anywhere on its own - this file only leaves your\n"
                "machine if you attach it to a message yourself.\n\n");
            s += QStringLiteral("Files in this bundle: %1\n").arg(fileCount);
            return s;
        }

        QString crashListing() {
            QDir dir(vth::paths::userCrashes());
            const QFileInfoList files =
                dir.entryInfoList(QDir::Files, QDir::Time | QDir::Reversed);
            if (files.isEmpty()) {
                return QStringLiteral("No crash files recorded.\n");
            }
            // Names and sizes only, because a minidump holds process memory.
            QString s = QStringLiteral("Crash files present (contents not included):\n\n");
            for (const QFileInfo& fi : files) {
                s += QStringLiteral("  %1  %2 bytes  %3\n")
                         .arg(fi.fileName())
                         .arg(fi.size())
                         .arg(fi.lastModified().toString(Qt::ISODate));
            }
            return s;
        }
    }

    QString redact(const QString& text) {
        QString out = text;

        // Paths carry the Windows account name.
        const QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        if (!home.isEmpty()) {
            out.replace(home, QStringLiteral("<home>"), Qt::CaseInsensitive);
            out.replace(QDir::toNativeSeparators(home), QStringLiteral("<home>"), Qt::CaseInsensitive);
        }

        // From the saved license, because this runs on a worker.
        if (const auto saved = vth::services::SecureStorage::getJson(QStringLiteral("license"))) {
            const QString licenseKey =
                QJsonDocument::fromJson(*saved).object().value(QStringLiteral("key")).toString();
            if (!licenseKey.isEmpty())
                out.replace(licenseKey, QStringLiteral("<license key>"), Qt::CaseInsensitive);
        }

        // Any other key in the 5x5 format.
        static const QRegularExpression key(
            QStringLiteral("\\b[0-9A-HJ-NP-TV-Z]{5}(?:-[0-9A-HJ-NP-TV-Z]{5}){4}\\b"));
        out.replace(key, QStringLiteral("XXXXX-XXXXX-XXXXX-XXXXX-XXXXX"));

        // INI and JSON members whose name says the value is sensitive.
        static const QRegularExpression secretAssign(
            QStringLiteral("(?i)^(\\s*\"?[A-Za-z0-9_.\\-]*"
                           "(?:key|token|secret|password|licen[cs]e|fingerprint)"
                           "[A-Za-z0-9_.\\-]*\"?\\s*[=:]\\s*)(.+)$"),
            QRegularExpression::MultilineOption);
        out.replace(secretAssign, QStringLiteral("\\1[redacted]"));

        return out;
    }

    QString suggestedFileName() {
        return QStringLiteral("vethida-support-%1.zip")
            .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmm")));
    }

    BundleResult writeBundle(const QString& outputPath) {
        BundleResult r;

        vth::log::flush();

        std::vector<Entry> entries;
        entries.reserve(8);

        addText(entries, "system.txt", systemSection());
        addFile(entries, QStringLiteral("settings.ini"), vth::Settings::userFilePath());
        addFile(entries, QStringLiteral("tweak-state.json"),
                vth::paths::protectedData() + QStringLiteral("/tweak-state.json"));
        addText(entries, "crashes.txt", crashListing());

        QDir logDir(vth::paths::userLogs());
        const QFileInfoList logs = logDir.entryInfoList(
            { QStringLiteral("*.log"), QStringLiteral("*.txt") },
            QDir::Files, QDir::Time);
        if (logs.isEmpty()) {
            addText(entries, "logs/none.txt",
                    QStringLiteral("No log files were present in %1.\n")
                        .arg(logDir.absolutePath()));
        }
        for (const QFileInfo& fi : logs) {
            addFile(entries,
                    QStringLiteral("logs/") + fi.fileName(),
                    fi.absoluteFilePath());
        }

        // Written last for the real count, but first in the archive.
        {
            Entry e;
            e.name = QByteArray("README.txt");
            e.data = manifestSection(int(entries.size()) + 1).toUtf8();
            e.crc  = crc32Of(e.data);
            entries.insert(entries.begin(), std::move(e));
        }

        qint64 total = 0;
        for (const Entry& e : entries) {
            total += e.data.size();
        }
        if (total > kMaxTotalBytes) {
            r.error = QObject::tr(
                "The diagnostic data is larger than a support bundle can carry. "
                "Clear the log folder and try again.");
            return r;
        }

        const QByteArray zip = buildZip(entries, QDateTime::currentDateTime());

        QSaveFile f(outputPath);
        if (!f.open(QIODevice::WriteOnly)) {
            r.error = QObject::tr("Could not create %1: %2")
                          .arg(QDir::toNativeSeparators(outputPath), f.errorString());
            return r;
        }
        if (f.write(zip) != zip.size() || !f.commit()) {
            r.error = QObject::tr("Could not finish writing %1: %2")
                          .arg(QDir::toNativeSeparators(outputPath), f.errorString());
            return r;
        }

        r.ok        = true;
        r.path      = outputPath;
        r.fileCount = int(entries.size());
        r.sizeBytes = zip.size();
        return r;
    }
}
