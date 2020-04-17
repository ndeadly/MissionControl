#include "application.hpp"
#include "bluetooth/core.hpp"

#include "gfx/graphics.hpp"
#include "scenes/settingsscene.hpp"

enum SettingsType {
    SettingsType_Toggle
};

struct Setting {
    SettingsType type;
    const char *text;
    const char *tooltip;
};

static const Setting g_settings[] = {
    {
        SettingsType_Toggle,
        "Enable bluetooth patches (restart required)",
        "Patches the bluetooth module pairing process to allow Wii and WiiU controllers to be paired"
    },
};

void SettingsScene::draw(void) {
    uint16_t y_offset = 130;
    int i;

    int num_items = 2;
    int x1 = 470;
    int x2 = 1189;
    int y1;
    int y2;
    
    // lay down separator lines
    for (i = 0; i < num_items; ++i) {
        y1 = y_offset + i*(70+1);
        mc::gfx::DrawHLine(x1, x2, y1, mc::app::theme->foregroundColor2);
    }
    mc::gfx::DrawHLine(470, 1189, y_offset + i*(70+1), mc::app::theme->foregroundColor2);

    // First option
}

void SettingsScene::handleInput(const mc::app::UserInput *input) {

}
