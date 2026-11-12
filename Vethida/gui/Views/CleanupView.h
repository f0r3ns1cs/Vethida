#pragma once

#include "../../vth_core/Runtime/Fetch.h"
#include "../../vth_core/Windows/Cleanup.h"

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QWidget>

namespace vth::widgets
{
    class CommandButton;
    class SettingsList;
    class SettingsRow;
}

class QLabel;

namespace vth::ui
{
    // Disk cleanup: every cleaner Vethida knows about, what each one would free
    // on this drive, and a switch on each.
    class CleanupView final : public QWidget
    {
        Q_OBJECT

    public:
        // driveRoot is the volume the Storage card has selected, which decides the
        // cleaners offered and the volume they act on.
        explicit CleanupView(const QString& driveRoot, QWidget* parent = nullptr);
        ~CleanupView() override;

    Q_SIGNALS:
        // A run finished. The Storage card re-measures the drive on this.
        void cleaned(quint64 reclaimedBytes);

    private:
        struct Row
        {
            QString                 id;
            vth::widgets::SettingsRow* row  = nullptr;
            QLabel*                 size = nullptr;
            bool                    measurable = true;
        };

        void buildUi();
        void rescan();
        void applyScan(const QHash<QString, vth::win::cleanup::ScanResult>& sizes);
        // The footer's figure and the button's enabled state, from whatever is
        // switched on right now.
        void refreshSelection();
        void onCleanClicked();
        QStringList selectedIds() const;

        QString                      driveRoot_;
        vth::widgets::SettingsList*  list_    = nullptr;
        vth::widgets::CommandButton* cleanBtn_ = nullptr;
        QLabel*                      total_   = nullptr;
        QVector<Row>                 rows_;
        // Sizes from the last scan, so refreshSelection can total the selection
        // without re-reading the labels it wrote.
        QHash<QString, quint64>      sizes_;
        bool                         scanned_ = false;

        vth::FetchToken scanToken_;
        vth::FetchToken runToken_;
    };
}
