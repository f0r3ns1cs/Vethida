#pragma once

#include <QWidget>

class QVBoxLayout;

namespace vth::dialogs
{
    // Vethida's open-source disclosure.
    class LicensesPanel : public QWidget
    {
        Q_OBJECT

    public:
        explicit LicensesPanel(QWidget* parent = nullptr);

    private:
        void buildUi();
        // The wording meets LGPLv3 section 4 and GPLv3 section 6: it names the work and
        // version, carries the written source offer and states the relink right.
        void addDisclosure(QVBoxLayout* col, QWidget* host);
        // Writes the embedded license texts to a folder the user picks.
        void saveTexts();
    };

    // Opens the disclosure as a content modal over the page the user is on.
    void showLicensesModal(QWidget* host);
}
