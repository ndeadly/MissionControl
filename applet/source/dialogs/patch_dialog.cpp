#include "gfx/graphics.hpp"
#include "dialogs/patch_dialog.hpp"

#include "reboot.h"
#include "patches.h"

void PatchDialog::accept(void) {
    this->hide();
    generate_bluetooth_patches();
    reboot();
}

void PatchDialog::cancel(void) {
    this->hide();
}

void PatchDialog::draw(void) {
    // translucent overlay
    mc::gfx::DrawRect(0, 0, SCREEN_W, SCREEN_H, mc::app::theme->dialogOverlayColor);
    // dialog background
    mc::gfx::DrawRoundedRect(255, 215, 1024, 505, 4, mc::app::theme->backgroundColor2);
    // dialog text
    mc::gfx::DrawText(310, 300, mc::font::Medium, mc::app::theme->foregroundColor, m_message);

    // buttons
    // button background
    mc::gfx::DrawRoundedRect(255, 436, 1024, 505, 3, mc::app::theme->backgroundColor3);
    // horizontal separator
    mc::gfx::DrawRect(255, 434, 1024, 435, mc::app::theme->dialogSeparatorColor);
    // vertical spacer
    mc::gfx::DrawRect(639, 436, 640, 505, mc::app::theme->dialogSeparatorColor);

    // selection
    switch(m_selection) {
        case DialogSelectionType_Ok:
            mc::gfx::DrawRoundedRect(635, 431, 1029, 510, 8, mc::app::theme->glowColor);
            mc::gfx::DrawRoundedRect(640, 436, 1024, 505, 3, mc::app::theme->backgroundColor3);
            break;
        case DialogSelectionType_Cancel:
            mc::gfx::DrawRoundedRect(250, 431, 644, 510, 8, mc::app::theme->glowColor);
            mc::gfx::DrawRoundedRect(255, 436, 639, 505, 3, mc::app::theme->backgroundColor3);
            break;
        default:
            break;
    }
  
    // button text
    mc::gfx::DrawText(400, 460, mc::font::Medium, mc::app::theme->highlightColor, "Cancel");
    mc::gfx::DrawText(790, 460, mc::font::Medium, mc::app::theme->highlightColor, "Generate");  
}

void PatchDialog::handleInput(const mc::app::UserInput *input) {

    if (input->kDown & KEY_A) {
        switch(m_selection) {
            case DialogSelectionType_Ok:
                this->accept();
                break;
            case DialogSelectionType_Cancel:
                this->cancel();
                break;
            default:
                break;
        }
    }

    if ( (input->kDown & KEY_LEFT) || (input->kDown & KEY_B) ) {
        m_selection = DialogSelectionType_Cancel;
    }

    if ( (input->kDown & KEY_RIGHT)) {
        m_selection = DialogSelectionType_Ok;
    }
}
