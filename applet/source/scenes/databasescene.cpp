#include "application.hpp"
#include "btcore.hpp"
#include "controllermanager.hpp"

#include "gfx/graphics.hpp"
#include "scenes/databasescene.hpp"

DatabaseScene::DatabaseScene() {
    m_imgIcons = mc::gfx::LoadTexture("romfs:/images/controller-icons.png");
}

DatabaseScene::~DatabaseScene() {
    mc::gfx::DestroyTexture(m_imgIcons);
}

void DatabaseScene::draw(void) {
    uint16_t y_offset = 130;
    unsigned int i;

    mc::gfx::DrawText(700, 96, mc::font::Small, mc::app::theme->foregroundColor, "%d paired controllers", mc::btcore::controllerDatabase->size());

    int x1 = 470;
    int x2 = 1189;
    int y1;
    int y2;
    // lay down separator lines
    for (i = 0; i < mc::btcore::controllerDatabase->size(); ++i) {
        y1 = y_offset + i*(70+1);
        mc::gfx::DrawHLine(x1, x2, y1, mc::app::theme->foregroundColor2);
    }
    mc::gfx::DrawHLine(470, 1189, y_offset + i*(70+1), mc::app::theme->foregroundColor2);

    const BluetoothDevice *device;
    for (i = 0; i < mc::btcore::controllerDatabase->size(); ++i) {
        device = mc::btcore::controllerDatabase->deviceAt(i);

        y1 = y_offset + i*(70+1);
        y2 = y1 + 70;
        
        // draw controller image
        SDL_Rect clip = {mc::controller::ControllerManager::identify(device)*48, mc::app::theme->colorSetId*48, 48, 48};
        mc::gfx::DrawTexture(x1+30, y1 + 14, m_imgIcons, &clip);
        // draw controller details
        mc::gfx::DrawText(x1+100, y1 + 10, mc::font::Small, mc::app::theme->foregroundColor, device->name);
        mc::gfx::DrawText(x1+100, y1 + 40, mc::font::Small, mc::app::theme->foregroundColor2, "%02X:%02X:%02X:%02X:%02X:%02X", 			
            ((uint8_t *)&device->address)[0],
			((uint8_t *)&device->address)[1],
			((uint8_t *)&device->address)[2],
			((uint8_t *)&device->address)[3],
			((uint8_t *)&device->address)[4],
			((uint8_t *)&device->address)[5]
        );
        mc::gfx::DrawText(x1+360, y1 + 40, mc::font::Small, mc::app::theme->foregroundColor2, "[%04X:%04X]", device->vid, device->pid);
    }
}

void DatabaseScene::handleInput(const mc::app::UserInput *input) {

}
