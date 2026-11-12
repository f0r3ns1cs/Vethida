#include "Dispatcher.h"

#include "../Logger.h"

#include <QCoreApplication>
#include <QMetaObject>
#include <QThread>
#include <QThreadPool>

#include <algorithm>

namespace vth
{
    namespace
    {
        VTH_DEFINE_LOG_CATEGORY(catWorker, "vth.worker")
    }

    void reportWorkerException(const char* what) {
        if (what)
            qCCritical(catWorker).noquote() << "worker task threw:" << what;
        else
            qCCritical(catWorker) << "worker task threw a non-standard exception";
    }

    Dispatcher& Dispatcher::instance() {
        static Dispatcher d;
        return d;
    }

    Dispatcher::Dispatcher() {
        pool_ = new QThreadPool();
        pool_->setObjectName(QStringLiteral("vth-workers"));
    }

    Dispatcher::~Dispatcher() {
        if (!shuttingDown_) {
            shutdown();
        }
        delete pool_;
        pool_ = nullptr;
    }

    void Dispatcher::configure(int workerCount) {
        if (configured_) return;

        int n = workerCount;
        if (n <= 0) {
            n = std::max(2, QThread::idealThreadCount() - 1);
        }
        pool_->setMaxThreadCount(n);

        pool_->setExpiryTimeout(30000);

        configured_ = true;
    }

    void Dispatcher::dispatchUi(std::function<void()> fn) {
        if (shuttingDown_ || !fn) return;
        auto* app = QCoreApplication::instance();
        if (!app) return;

        QMetaObject::invokeMethod(app, std::move(fn), Qt::QueuedConnection);
    }

    void Dispatcher::shutdown() {
        if (shuttingDown_) return;
        shuttingDown_ = true;

        if (pool_) {
            // The timeout is so a stuck task can't block exit.
            pool_->clear();
            pool_->waitForDone(5000);
        }
    }

    int Dispatcher::workerCount() const noexcept {
        return pool_ ? pool_->maxThreadCount() : 0;
    }

}

namespace vth
{
    bool Dispatcher::isShuttingDown() const noexcept {
        return shuttingDown_;
    }
}
