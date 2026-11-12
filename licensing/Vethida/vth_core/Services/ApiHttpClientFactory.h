#pragma once

#include <QtCore/QObject>

#include <chrono>

class QNetworkAccessManager;

namespace vth::services
{
    class DoHResolver;

    // QNAM for ApiClient: DoH for api.vethida.com, chain validation + SPKI pinning,
    // https only.
    class ApiHttpClientFactory
    {
    public:
        // create once at startup
        static QNetworkAccessManager* createHardenedClient(DoHResolver* doh,
                                                           std::chrono::milliseconds timeout,
                                                           QObject* parent = nullptr);
    };
}
