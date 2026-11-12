#pragma once

#include <QString>
#include <QtTypes>

// How much of what Vethida can do on this PC has been done.
namespace vth::win::health
{
    struct Report {
        int  score = 0;                 // 0-100

        // For display only.
        QString gpuName;
        QString cpuName;
        quint64 ramBytes = 0;
        QString osName;                 // "Windows 11 Pro (10.0.26100)"
    };

    QString scoreBand(int score);

    // Slow: call it from a worker.
    Report evaluate();
}
