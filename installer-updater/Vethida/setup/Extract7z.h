#pragma once

#include <functional>
#include <string>

namespace vth::setup
{
    struct ExtractResult {
        bool          ok = false;
        std::wstring  error;
        int           filesExtracted = 0;
        unsigned long long bytesExtracted = 0;
    };

    using ExtractProgressFn =
        std::function<void(int filesDone, int filesTotal,
                           const std::wstring& currentName)>;

    // Finds the .7z appended to the exe (footer from make-installer.mjs), copies it
    // to temp and extracts it. Locked files get renamed and deleted on reboot.
    ExtractResult extractEmbeddedPayload(const std::wstring& selfPath,
                                         const std::wstring& outputDir,
                                         const ExtractProgressFn& onProgress = {});
}
