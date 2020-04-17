#pragma once

#include "ui/dialog.hpp"

class PatchDialog : public mc::ui::Dialog {

    public:
        PatchDialog() 
            : Dialog("Bluetooth exefs patches not found for current firmware.\nGenerate now? (Console will reboot)")
            , m_selection(DialogSelectionType_Ok)
        {};

        void accept(void);
        void cancel(void);

        void draw(void);
        void handleInput(const mc::app::UserInput *input);

    private:
        DialogSelectionType m_selection;

};
