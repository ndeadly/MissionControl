#include "application.hpp"
#include "btcore.hpp"

#include "gfx/graphics.hpp"
#include "scenes/settingsscene.hpp"

void SettingsScene::draw(void) {
    uint16_t y_offset = 130;
    unsigned int i;

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
