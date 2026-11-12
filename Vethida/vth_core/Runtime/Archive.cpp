// The LZMA SDK's global Int32 and Int64 typedefs are shadowed by locals in Qt.
#pragma warning(disable: 4459)

#include "Archive.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringLiteral>

#include <vector>

#include "../../lib/lzma/7z.h"
#include "../../lib/lzma/7zAlloc.h"
#include "../../lib/lzma/7zCrc.h"
#include "../../lib/lzma/7zFile.h"

namespace vth::archive
{
    namespace
    {
        constexpr unsigned char kSig[6] = { '7', 'z', 0xBC, 0xAF, 0x27, 0x1C };

        // Offset 0 for a bare .7z. A self-extracting archive has a stub in front.
        // Returns -1 if not found.
        qint64 findSignatureOffset(QFile& file, qint64 scanLimit) {
            constexpr qint64 kChunk = 1 << 20;
            const QByteArray needle(reinterpret_cast<const char*>(kSig), 6);
            QByteArray carry;   // for a signature split across chunks
            qint64 basePos = 0;

            if (!file.seek(0)) return -1;
            while (basePos < scanLimit) {
                const QByteArray buf = file.read(kChunk);
                if (buf.isEmpty()) break;
                const QByteArray hay = carry + buf;
                const int idx = hay.indexOf(needle);
                if (idx >= 0) return basePos - carry.size() + idx;
                carry = buf.right(5);
                basePos += buf.size();
            }
            return -1;
        }

        ExtractResult extractPlain7z(const QString& sevenZPath, const QString& outDir) {
            ExtractResult r;

            ISzAlloc allocImp     = { SzAlloc, SzFree };
            ISzAlloc allocTempImp = { SzAllocTemp, SzFreeTemp };

            CrcGenerateTable();

            CFileInStream archiveStream;
            FileInStream_CreateVTable(&archiveStream);
            File_Construct(&archiveStream.file);
            if (InFile_OpenW(&archiveStream.file,
                             reinterpret_cast<const WCHAR*>(sevenZPath.utf16())) != 0) {
                r.error = QStringLiteral("Could not open staged archive.");
                return r;
            }

            std::vector<Byte> lookBuf(1u << 18);
            CLookToRead2 lookStream;
            LookToRead2_CreateVTable(&lookStream, 0);
            lookStream.realStream = &archiveStream.vt;
            lookStream.buf        = lookBuf.data();
            lookStream.bufSize    = lookBuf.size();
            LookToRead2_INIT(&lookStream);

            CSzArEx db;
            SzArEx_Init(&db);

            SRes res = SzArEx_Open(&db, &lookStream.vt, &allocImp, &allocTempImp);
            if (res != SZ_OK) {
                r.error = QStringLiteral("Not a valid 7z archive (SzArEx_Open=%1).").arg(res);
                SzArEx_Free(&db, &allocImp);
                File_Close(&archiveStream.file);
                return r;
            }

            QDir().mkpath(outDir);

            UInt32 blockIndex    = 0xFFFFFFFF;  // solid-block cache across files
            Byte*  outBuffer     = nullptr;
            size_t outBufferSize = 0;
            std::vector<UInt16> nameBuf;
            bool ok = true;

            for (UInt32 i = 0; i < db.NumFiles; ++i) {
                const size_t nameLen = SzArEx_GetFileNameUtf16(&db, i, nullptr);
                nameBuf.resize(nameLen == 0 ? 1 : nameLen);
                SzArEx_GetFileNameUtf16(&db, i, nameBuf.data());
                QString name = QString::fromUtf16(
                    reinterpret_cast<const char16_t*>(nameBuf.data()),
                    nameLen > 0 ? int(nameLen) - 1 : 0);
                name.replace(QLatin1Char('\\'), QLatin1Char('/'));

                // Entries must stay inside outDir.
                const QString rel = QDir::cleanPath(name);
                if (rel.isEmpty() || rel == QLatin1String("..")
                    || rel.startsWith(QLatin1String("../"))
                    || rel.contains(QLatin1Char(':'))
                    || QDir::isAbsolutePath(rel)) {
                    ok = false;
                    r.error = QStringLiteral("Unsafe path in archive: %1").arg(name);
                    break;
                }
                const QString dest = outDir + QLatin1Char('/') + rel;

                if (SzArEx_IsDir(&db, i)) {
                    QDir().mkpath(dest);
                    continue;
                }

                QDir().mkpath(QFileInfo(dest).absolutePath());

                size_t offset = 0;
                size_t sizeProcessed = 0;
                res = SzArEx_Extract(&db, &lookStream.vt, i, &blockIndex,
                                     &outBuffer, &outBufferSize, &offset, &sizeProcessed,
                                     &allocImp, &allocTempImp);
                if (res != SZ_OK) {
                    ok = false;
                    r.error = QStringLiteral("Decode failed for %1 (SzArEx_Extract=%2).")
                                  .arg(rel).arg(res);
                    break;
                }

                QFile out(dest);
                if (!out.open(QIODevice::WriteOnly)) {
                    ok = false;
                    r.error = QStringLiteral("Could not write %1.").arg(dest);
                    break;
                }
                if (sizeProcessed > 0
                    && out.write(reinterpret_cast<const char*>(outBuffer + offset),
                                 qint64(sizeProcessed)) != qint64(sizeProcessed)) {
                    ok = false;
                    r.error = QStringLiteral("Could not write %1.").arg(dest);
                    break;
                }
                if (!out.flush()) {
                    ok = false;
                    r.error = QStringLiteral("Could not write %1.").arg(dest);
                    break;
                }
            }

            if (outBuffer) allocImp.Free(&allocImp, outBuffer);
            SzArEx_Free(&db, &allocImp);
            File_Close(&archiveStream.file);

            r.ok = ok;
            return r;
        }
    }

    ExtractResult extract7z(const QString& archivePath, const QString& outputDir) {
        ExtractResult r;

        QFile in(archivePath);
        if (!in.open(QIODevice::ReadOnly)) {
            r.error = QStringLiteral("Could not open %1.").arg(archivePath);
            return r;
        }

        const qint64 scanLimit = qMin<qint64>(in.size(), qint64(64) << 20);
        const qint64 sig = findSignatureOffset(in, scanLimit);
        if (sig < 0) {
            r.error = QStringLiteral("No 7z signature found in %1.").arg(archivePath);
            return r;
        }

        if (sig == 0) {
            in.close();
            return extractPlain7z(archivePath, outputDir);
        }

        // Self-extracting: copy the payload out so the SDK sees a bare .7z.
        QDir().mkpath(outputDir);
        const QString staged = outputDir + QStringLiteral("/.vth_payload.7z");
        QFile out(staged);
        if (!out.open(QIODevice::WriteOnly)) {
            r.error = QStringLiteral("Could not stage 7z payload.");
            return r;
        }
        in.seek(sig);
        constexpr qint64 kCopyChunk = qint64(8) << 20;
        for (;;) {
            const QByteArray buf = in.read(kCopyChunk);
            if (buf.isEmpty()) break;
            if (out.write(buf) != buf.size()) {
                out.close();
                QFile::remove(staged);
                r.error = QStringLiteral("Failed staging 7z payload.");
                return r;
            }
        }
        out.close();
        in.close();

        ExtractResult res = extractPlain7z(staged, outputDir);
        QFile::remove(staged);
        return res;
    }
}
