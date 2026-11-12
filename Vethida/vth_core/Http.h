#pragma once

#include <QByteArray>
#include <QString>

#include <chrono>
#include <functional>
#include <vector>

// Blocking HTTPS over QNetworkAccessManager. Worker threads only.
namespace vth::http
{
    struct Header {
        QString name;
        QString value;
    };

    struct Response {
        bool        ok = false;         // transport succeeded and status is 2xx
        int         statusCode = 0;     // 0 if the transport failed
        QByteArray  body;
        std::vector<Header> headers;
        QString     errorString;
    };

    constexpr std::chrono::milliseconds kDefaultTimeout{ 30'000 };

    // Content type defaults to application/json.
    Response post(const QString& url,
        const QByteArray& body,
        const std::vector<Header>& headers = {},
        std::chrono::milliseconds timeout = kDefaultTimeout) noexcept;

    Response get(const QString& url,
        const std::vector<Header>& headers = {},
        std::chrono::milliseconds timeout = kDefaultTimeout) noexcept;

    struct Download {
        bool    ok = false;          // transport succeeded and status is 2xx
        int     statusCode = 0;      // 0 if the transport failed
        qint64  bytes = 0;           // handed to the sink
        bool    canceled = false;
        bool    sinkRefused = false;
        QString errorString;
    };

    // Returns false to abort. The block is only valid during the call.
    using ByteSink = std::function<bool(const char* data, qint64 size)>;
    using CancelFn = std::function<bool()>;
    // total is -1 without a Content-Length.
    using ProgressFn = std::function<void(qint64 received, qint64 total)>;

    // Time without data, not total time, so a large file can't time out midway.
    constexpr std::chrono::milliseconds kStallTimeout{ 45'000 };

    // Streams a GET to the sink instead of holding it in memory.
    Download download(const QString& url,
        const ByteSink& sink,
        const CancelFn& canceled = {},
        const ProgressFn& progress = {},
        std::chrono::milliseconds stallTimeout = kStallTimeout) noexcept;
}
