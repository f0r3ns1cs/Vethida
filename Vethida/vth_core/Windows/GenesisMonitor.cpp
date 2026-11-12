#include "GenesisMonitor.h"

#include "Genesis.h"
#include "../Logger.h"
#include "../Runtime/Fetch.h"
#include "../Runtime/Threading.h"

#include <QTimer>

namespace vth::win::genesis
{
    namespace
    {
        VTH_DEFINE_LOG_CATEGORY(catMonitor, "vth.genesis.monitor")

        constexpr int kSweepIntervalMs = 5000;
    }

    GenesisMonitor::GenesisMonitor(QObject* parent)
        : QObject(parent)
        , live_(std::make_shared<std::atomic<bool>>(false))
    {
        timer_ = new QTimer(this);
        timer_->setInterval(kSweepIntervalMs);
        connect(timer_, &QTimer::timeout, this, &GenesisMonitor::sweep);
    }

    GenesisMonitor::~GenesisMonitor() {
        live_->store(false);
    }

    void GenesisMonitor::start(const QStringList& targets, const QStringList& exceptions) {
        VTH_ASSERT_UI_THREAD();
        stop();

        targets_ = allowedTargets(targets, exceptions);
        if (targets_.isEmpty()) {
            qCInfo(catMonitor) << "nothing to watch";
            return;
        }

        live_ = std::make_shared<std::atomic<bool>>(true);
        timer_->start();
        qCInfo(catMonitor) << "watching" << targets_.size() << "images";
    }

    void GenesisMonitor::stop() {
        VTH_ASSERT_UI_THREAD();
        live_->store(false);
        if (timer_ && timer_->isActive())
            timer_->stop();
        targets_.clear();
        inFlight_ = false;
    }

    void GenesisMonitor::sweep() {
        VTH_ASSERT_UI_THREAD();
        if (inFlight_ || targets_.isEmpty())
            return;
        inFlight_ = true;

        vth::fetch<QStringList>(
            this,
            [targets = targets_, live = live_]() -> QStringList {
                if (!live->load()) return {};
                // start() already applied the exceptions.
                const auto closed = closeProcesses(targets, {});
                if (!live->load()) return {};
                return closed.success;
            },
            [this](QStringList caught) {
                inFlight_ = false;
                if (caught.isEmpty())
                    return;
                qCInfo(catMonitor).noquote()
                    << "closed again:" << caught.join(QStringLiteral(", "));
            });
    }
}
