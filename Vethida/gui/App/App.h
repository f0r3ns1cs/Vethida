#pragma once

#include <QApplication>
#include <QString>

#include <functional>
#include <memory>

namespace vth::win
{
    class SingleInstanceGuard;
}

namespace vth
{
    // Single-instance QApplication subclass.
    class App : public QApplication
    {
        Q_OBJECT

    public:
        App(int& argc, char* argv[]);
        ~App() override;

        static App* instance();

        bool isPrimaryInstance() const noexcept;

        // Sends a payload to the running copy, from a second launch.
        bool forwardToPrimary(const QString& payload);

        // Called on the UI thread when a later launch forwards a payload. Install it
        // before entering the event loop.
        void setForwardedPayloadHandler(std::function<void(const QString&)> handler);

    private:
        std::unique_ptr<vth::win::SingleInstanceGuard> guard_;
    };
}
