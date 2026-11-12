#pragma once

#include <QWidget>

class QShowEvent;

namespace vth::widgets { class PageHeader; }

namespace vth::ui
{
    // The BIOS tab: the UEFI NVRAM settings editor built on AMISCE, and nothing
    // else.
    class BiosPage final : public QWidget
    {
        Q_OBJECT

    public:
        explicit BiosPage(QWidget* parent = nullptr);

    protected:
        void showEvent(QShowEvent* e) override;

    private:
        void buildContent();

        vth::widgets::PageHeader* header_ = nullptr;
        bool                      built_  = false;
    };
}
