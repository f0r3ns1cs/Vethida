#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>

#include <atomic>

namespace vth::sentry
{
    // Crash reports, off until the person turns them on. The SDK only runs while
    // they're on, so its crash handler never runs without consent.
    class Sentry : public QObject {
        Q_OBJECT

    public:
        Sentry();
        ~Sentry() override;

        void initialize();
        bool isCrashEnabled() const noexcept { return crashEnabled_; }
        void setCrashEnabled(bool on);

    private:
        std::atomic<bool> crashEnabled_{false};
    };
}
