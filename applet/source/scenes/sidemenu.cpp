#include "gfx/graphics.hpp"
#include "scenes/sidemenu.hpp"

const char *g_menuOptions[] = {
    "Pair Controllers",
    "Test Controller",
    //"Configure Controls",
    "View Database",
    "Settings",
};

void SideMenu::draw(void) {
    uint8_t menu_len = sizeof(g_menuOptions) / sizeof(char *);

    // draw side menu background
    mc::gfx::DrawRect(0, 88, 409, 646, mc::app::theme->backgroundColor2);

    uint16_t y_offset = 130;
    unsigned int i;
    for (i = 0; i < menu_len; ++i) {
        int x1 = 80;
        int x2 = 380;
        int y1 = y_offset + i*(70+1);
        int y2 = y1 + 70;

        if (i == m_selectionIdx) {
            if (this->hasFocus()) {
                // selection "glow"
                mc::gfx::DrawRoundedRect(x1-5, y1-5, x2+5, y2+5, 2, mc::app::theme->glowColor);
                // selection background
                mc::gfx::DrawRect(x1, y1, x2, y2, mc::app::theme->selectionColor);
            }

            // side bar
            mc::gfx::DrawRect(x1+9, y1+10, x1+9+4, y2-10, mc::app::theme->highlightColor);
            mc::gfx::DrawText(x1+22, y_offset + i*(70+1) + 25, mc::font::Medium, mc::app::theme->highlightColor, g_menuOptions[i]);
        } else {
            mc::gfx::DrawText(x1+22, y_offset + i*(70+1) + 25, mc::font::Medium, mc::app::theme->foregroundColor, g_menuOptions[i]);
        }
    }
}

void SideMenu::handleInput(const mc::app::UserInput *input) {
    uint8_t menu_len = sizeof(g_menuOptions) / sizeof(char *);

    if (input->kDown & KEY_UP) {
        m_selectionIdx = std::max(m_selectionIdx-1, 0);
    }

    if (input->kDown & KEY_DOWN) {
        m_selectionIdx = std::min(m_selectionIdx+1, menu_len-1);
    }
}
