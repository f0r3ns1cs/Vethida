#pragma once

#include <QString>
#include <QVector>
#include <QWidget>

namespace vth::widgets { class CommandButton; class ListRow; }

namespace vth::ui
{
    // Get apps: a curated list of free software, installed silently.
    class AppInstallerView final : public QWidget
    {
        Q_OBJECT

    public:
        explicit AppInstallerView(QWidget* parent = nullptr);

    private:
        struct InstallRow
        {
            QString id;
            vth::widgets::CommandButton* button = nullptr;
            vth::widgets::ListRow*       row    = nullptr;
        };

        void buildUi();
        void scanInstalledState();
        void startInstall(const InstallRow& row);
        void markInstalled(const InstallRow& row, const QString& text);

        QVector<InstallRow> rows_;
    };
}
