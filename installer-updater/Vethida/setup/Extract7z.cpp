#include "Extract7z.h"

#include <windows.h>

#include <cstring>
#include <vector>

#include "../lib/lzma/7z.h"
#include "../lib/lzma/7zAlloc.h"
#include "../lib/lzma/7zCrc.h"
#include "../lib/lzma/7zFile.h"

namespace vth::setup
{
    namespace
    {
        constexpr size_t kFooterSize = 24;
        constexpr size_t kTailScan   = 256 * 1024;

        // "VTHPKG01" ^ 0x5C so the scan doesn't find the marker in our own code
        void footerMagic(unsigned char out[8])
        {
            static const unsigned char enc[8] = {
                0x0A, 0x08, 0x14, 0x0C, 0x17, 0x1B, 0x6C, 0x6D
            };
            for (int i = 0; i < 8; ++i) out[i] = enc[i] ^ 0x5C;
        }

        struct Payload {
            unsigned long long offset = 0;
            unsigned long long size   = 0;
        };

        bool locatePayload(HANDLE file, Payload& out, std::wstring& error)
        {
            LARGE_INTEGER fileSize{};
            if (!GetFileSizeEx(file, &fileSize) || fileSize.QuadPart <= 0) {
                error = L"Could not read installer size.";
                return false;
            }

            const unsigned long long total = fileSize.QuadPart;
            const size_t tail =
                static_cast<size_t>(total < kTailScan ? total : kTailScan);
            std::vector<unsigned char> buf(tail);

            LARGE_INTEGER pos{};
            pos.QuadPart = static_cast<LONGLONG>(total - tail);
            DWORD read = 0;
            if (!SetFilePointerEx(file, pos, nullptr, FILE_BEGIN)
                || !ReadFile(file, buf.data(), static_cast<DWORD>(tail), &read,
                             nullptr)
                || read != tail) {
                error = L"Could not read installer trailer.";
                return false;
            }

            unsigned char magic[8];
            footerMagic(magic);

            // the signature gets appended after the footer, so scan from the end
            for (size_t i = tail; i >= kFooterSize; --i) {
                const size_t start = i - kFooterSize;
                if (std::memcmp(buf.data() + start, magic, 8) != 0) continue;

                unsigned long long offset = 0;
                unsigned long long size = 0;
                std::memcpy(&offset, buf.data() + start + 8, 8);
                std::memcpy(&size, buf.data() + start + 16, 8);

                if (size == 0 || offset >= total || offset + size > total)
                    continue;
                out.offset = offset;
                out.size   = size;
                return true;
            }

            error = L"This installer has no embedded payload. "
                    L"Rebuild it with scripts/release/make-installer.mjs.";
            return false;
        }

        bool stagePayload(HANDLE self, const Payload& payload,
                          const std::wstring& stagedPath, std::wstring& error)
        {
            HANDLE out = CreateFileW(stagedPath.c_str(), GENERIC_WRITE, 0,
                                     nullptr, CREATE_ALWAYS,
                                     FILE_ATTRIBUTE_TEMPORARY, nullptr);
            if (out == INVALID_HANDLE_VALUE) {
                error = L"Could not stage the payload archive.";
                return false;
            }

            LARGE_INTEGER pos{};
            pos.QuadPart = static_cast<LONGLONG>(payload.offset);
            if (!SetFilePointerEx(self, pos, nullptr, FILE_BEGIN)) {
                CloseHandle(out);
                error = L"Could not seek to the payload.";
                return false;
            }

            std::vector<unsigned char> buf(4 * 1024 * 1024);
            unsigned long long remaining = payload.size;
            while (remaining > 0) {
                const DWORD want = static_cast<DWORD>(
                    remaining < buf.size() ? remaining : buf.size());
                DWORD got = 0, written = 0;
                if (!ReadFile(self, buf.data(), want, &got, nullptr) || got == 0
                    || !WriteFile(out, buf.data(), got, &written, nullptr)
                    || written != got) {
                    CloseHandle(out);
                    error = L"Failed copying the payload archive.";
                    return false;
                }
                remaining -= got;
            }
            CloseHandle(out);
            return true;
        }

        bool safeRelativePath(const std::wstring& rel)
        {
            if (rel.empty()) return false;
            if (rel.find(L':') != std::wstring::npos) return false;
            if (rel[0] == L'\\' || rel[0] == L'/') return false;
            size_t i = 0;
            while (i < rel.size()) {
                size_t j = rel.find_first_of(L"\\/", i);
                if (j == std::wstring::npos) j = rel.size();
                if (j - i == 2 && rel[i] == L'.' && rel[i + 1] == L'.')
                    return false;
                i = j + 1;
            }
            return true;
        }

        bool ensureParentDirs(const std::wstring& root, const std::wstring& rel)
        {
            size_t i = 0;
            while ((i = rel.find_first_of(L"\\/", i)) != std::wstring::npos) {
                const std::wstring dir = root + L"\\" + rel.substr(0, i);
                CreateDirectoryW(dir.c_str(), nullptr);
                ++i;
            }
            return true;
        }

        HANDLE openDestination(const std::wstring& dest)
        {
            HANDLE h = CreateFileW(dest.c_str(), GENERIC_WRITE, 0, nullptr,
                                   CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                   nullptr);
            if (h != INVALID_HANDLE_VALUE) return h;

            const DWORD err = GetLastError();
            if (err != ERROR_SHARING_VIOLATION && err != ERROR_ACCESS_DENIED)
                return INVALID_HANDLE_VALUE;

            // in use, move it aside
            wchar_t aside[MAX_PATH + 32];
            swprintf_s(aside, L"%s.old-%08lx", dest.c_str(), GetTickCount());
            SetFileAttributesW(dest.c_str(), FILE_ATTRIBUTE_NORMAL);
            if (!MoveFileExW(dest.c_str(), aside, MOVEFILE_REPLACE_EXISTING))
                return INVALID_HANDLE_VALUE;
            MoveFileExW(aside, nullptr, MOVEFILE_DELAY_UNTIL_REBOOT);

            return CreateFileW(dest.c_str(), GENERIC_WRITE, 0, nullptr,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        }

        ExtractResult extractArchive(const std::wstring& archivePath,
                                     const std::wstring& outputDir,
                                     const ExtractProgressFn& onProgress)
        {
            ExtractResult r;

            ISzAlloc allocImp     = { SzAlloc, SzFree };
            ISzAlloc allocTempImp = { SzAllocTemp, SzFreeTemp };
            CrcGenerateTable();

            CFileInStream archiveStream;
            FileInStream_CreateVTable(&archiveStream);
            File_Construct(&archiveStream.file);
            if (InFile_OpenW(&archiveStream.file, archivePath.c_str()) != 0) {
                r.error = L"Could not open the staged payload.";
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
            if (SzArEx_Open(&db, &lookStream.vt, &allocImp, &allocTempImp)
                != SZ_OK) {
                r.error = L"The payload is not a valid 7z archive.";
                SzArEx_Free(&db, &allocImp);
                File_Close(&archiveStream.file);
                return r;
            }

            UInt32 blockIndex    = 0xFFFFFFFF;
            Byte*  outBuffer     = nullptr;
            size_t outBufferSize = 0;
            std::vector<UInt16> nameBuf;
            bool ok = true;
            const int fileTotal = static_cast<int>(db.NumFiles);

            for (UInt32 i = 0; i < db.NumFiles; ++i) {
                const size_t nameLen =
                    SzArEx_GetFileNameUtf16(&db, i, nullptr);
                nameBuf.resize(nameLen == 0 ? 1 : nameLen);
                SzArEx_GetFileNameUtf16(&db, i, nameBuf.data());
                std::wstring rel(
                    reinterpret_cast<const wchar_t*>(nameBuf.data()),
                    nameLen > 0 ? nameLen - 1 : 0);
                for (auto& c : rel)
                    if (c == L'/') c = L'\\';

                if (onProgress)
                    onProgress(static_cast<int>(i), fileTotal, rel);

                if (!safeRelativePath(rel)) {
                    ok = false;
                    r.error = L"Unsafe path in payload: " + rel;
                    break;
                }
                const std::wstring dest = outputDir + L"\\" + rel;

                if (SzArEx_IsDir(&db, i)) {
                    CreateDirectoryW(dest.c_str(), nullptr);
                    continue;
                }
                ensureParentDirs(outputDir, rel);

                size_t offset = 0;
                size_t sizeProcessed = 0;
                if (SzArEx_Extract(&db, &lookStream.vt, i, &blockIndex,
                                   &outBuffer, &outBufferSize, &offset,
                                   &sizeProcessed, &allocImp,
                                   &allocTempImp) != SZ_OK) {
                    ok = false;
                    r.error = L"Failed decoding " + rel;
                    break;
                }

                HANDLE out = openDestination(dest);
                if (out == INVALID_HANDLE_VALUE) {
                    ok = false;
                    r.error = L"Could not write " + dest;
                    break;
                }
                DWORD written = 0;
                if (sizeProcessed > 0
                    && (!WriteFile(out, outBuffer + offset,
                                   static_cast<DWORD>(sizeProcessed), &written,
                                   nullptr)
                        || written != sizeProcessed)) {
                    CloseHandle(out);
                    ok = false;
                    r.error = L"Short write on " + dest;
                    break;
                }
                CloseHandle(out);

                ++r.filesExtracted;
                r.bytesExtracted += sizeProcessed;
            }

            if (outBuffer) allocImp.Free(&allocImp, outBuffer);
            SzArEx_Free(&db, &allocImp);
            File_Close(&archiveStream.file);
            if (ok && onProgress) onProgress(fileTotal, fileTotal, std::wstring());
            r.ok = ok;
            return r;
        }
    }

    ExtractResult extractEmbeddedPayload(const std::wstring& selfPath,
                                         const std::wstring& outputDir,
                                         const ExtractProgressFn& onProgress)
    {
        ExtractResult r;

        HANDLE self = CreateFileW(selfPath.c_str(), GENERIC_READ,
                                  FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr,
                                  OPEN_EXISTING, 0, nullptr);
        if (self == INVALID_HANDLE_VALUE) {
            r.error = L"Could not open the installer for reading.";
            return r;
        }

        Payload payload;
        if (!locatePayload(self, payload, r.error)) {
            CloseHandle(self);
            return r;
        }

        wchar_t tempDir[MAX_PATH];
        GetTempPathW(MAX_PATH, tempDir);
        wchar_t staged[MAX_PATH];
        swprintf_s(staged, L"%svth-setup-%08lx.7z", tempDir,
                   GetCurrentProcessId());

        const bool stagedOk = stagePayload(self, payload, staged, r.error);
        CloseHandle(self);
        if (!stagedOk) return r;

        r = extractArchive(staged, outputDir, onProgress);
        DeleteFileW(staged);
        return r;
    }
}
