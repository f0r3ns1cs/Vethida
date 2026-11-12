#pragma once

#include "../../vth_core/Runtime/Fetch.h"

#include <QHash>
#include <QWidget>

class QButtonGroup;
class QLabel;
class QRadioButton;

namespace vth::ui
{
    // "Power plans": the bundled powercfg schemes, plus Windows' own Balanced,
    // as one card holding one row per plan.
    class PowerPlansView final : public QWidget
    {
        Q_OBJECT

    public:
        explicit PowerPlansView(QWidget* parent = nullptr);
        ~PowerPlansView() override;

    private:
        void buildUi();
        // Reads the active scheme on a worker and settles the row selection and
        // the status line from the answer.
        void refreshActivePlan();
        // Radios off while a read or an import is in flight.
        void setRowsEnabled(bool enabled);

        QButtonGroup* group_  = nullptr;
        QLabel*       status_ = nullptr;
        QHash<int, QRadioButton*> rows_;   // index into powerPlans() -> row

        // id = powerPlans().size(), label filled in from powercfg
        QRadioButton* balancedRow_    = nullptr;
        QLabel*       balancedDetail_ = nullptr;
        QWidget*      balancedHost_   = nullptr;
        QString       balancedGuid_;

        FetchToken activeToken_;
    };
}
