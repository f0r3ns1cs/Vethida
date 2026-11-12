#pragma once

#include <QtCore/QObject>

class QNetworkAccessManager;
class QTimer;

namespace vth
{
    // Remote kill switch: a JSON list on vethida.com, checked at start and daily.
    // Saved, so it still applies offline.
    class WithdrawnTweaks final : public QObject
    {
    public:
        explicit WithdrawnTweaks(QObject* parent = nullptr);
        void start();

    private:
        void fetch();

        QNetworkAccessManager* m_net = nullptr;
        QTimer*                m_timer = nullptr;
    };
}
