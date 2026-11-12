#pragma once

#include <QString>
#include <QtTypes>

#include <vector>

namespace vth::win
{
    struct DriveSpace {
        quint64 totalBytes = 0;
        quint64 freeBytes  = 0;
        bool    valid      = false;
    };

    struct DriveInfo {
        QString root;                       // "C:\\"
        QString label;                      // volume label, may be empty
        QString fileSystem;                 // "NTFS"
        quint64 totalBytes = 0;
        quint64 freeBytes  = 0;
        bool    isSystem   = false;         // is this where windows install resides
    };

    struct StorageBreakdown {
        bool    valid                = false;
        quint64 totalBytes           = 0;   // GetDiskFreeSpaceExW: TotalNumberOfBytes
        quint64 freeBytes            = 0;   // GetDiskFreeSpaceExW: TotalNumberOfFreeBytes

        // Measured categories (recursive directory sums or file sizes).
        quint64 windowsBytes         = 0;
        quint64 programFilesBytes    = 0;
        quint64 programFilesX86Bytes = 0;
        quint64 usersBytes           = 0;
        quint64 pageFileBytes        = 0;
        quint64 hibernationBytes     = 0;

        // (totalBytes - freeBytes) - sum(measured categories).
        quint64 otherBytes           = 0;
    };

    // Every fixed (DRIVE_FIXED) volume with a drive letter, with space and label.
    std::vector<DriveInfo> fixedDrives();

    // Fast: one GetDiskFreeSpaceExW for the given volume root (e.g. "D:\\").
    DriveSpace driveSpace(const QString& root);

    // Slow (walks the drive). The Windows/Users/pagefile categories are 0 on data drives.
    StorageBreakdown driveBreakdown(const QString& root);

    // System volume root, e.g. "C:\\". Empty on failure.
    QString systemDriveRoot();

}

namespace vth::win
{
    DriveSpace       systemDriveSpace();
    StorageBreakdown systemDriveBreakdown();
}
