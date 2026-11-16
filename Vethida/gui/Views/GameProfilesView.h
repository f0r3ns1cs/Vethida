#pragma once

#include <QVector>
#include <QWidget>

#include "../../vth_core/Runtime/Fetch.h"
#include "../../vth_core/Windows/GameProfiles.h"

class QLabel;
class QVBoxLayout;

namespace vth::ui
{
    // Per-game settings profiles. Lists the games whose config files Vethida can
    // find on this PC and applies a low-overhead settings profile, or restores the
    // backup taken beforehand.
    //
    // Unlike Genesis, which lasts one session, this writes a settings file that
    // stays written. Every write is preceded by a copy to <name>.vth-bak, so
    // Restore is a single copy back rather than a guess at the prior values.
    class GameProfilesView final : public QWidget
    {
        Q_OBJECT

    public:
        explicit GameProfilesView(QWidget* parent = nullptr);

    private:
        void buildUi();
        // Kicks off the filesystem scan on a worker; populateRows renders the
        // result. Never scan on the UI thread - it walks install directories.
        void populate();
        void populateRows(const QVector<vth::win::gameprofiles::Profile>& profiles);

        QLabel*      headline_ = nullptr;
        QVBoxLayout* rows_     = nullptr;
        vth::FetchToken loadToken_;
    };
}
