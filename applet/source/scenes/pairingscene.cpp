#include "application.hpp"
#include "btcore.hpp"
#include "hidgamepad.hpp"

#include "gfx/graphics.hpp"
#include "scenes/pairingscene.hpp"

PairingScene::PairingScene() 
: m_selectionIdx(-1) {
    m_imgIcons = mc::gfx::LoadTexture("romfs:/images/controller-icons.png");
}

PairingScene::~PairingScene() {
    mc::gfx::DestroyTexture(m_imgIcons);
}

void PairingScene::draw(void) {

    if (mc::btcore::IsDiscovering()) {
        mc::gfx::DrawGlyph(670, 96, 
            mc::font::ExtSmall, 
            mc::app::theme->foregroundColor, 
            GlyphType_SpinnerTopLeft + mc::app::counter
        );
        mc::gfx::DrawText(700, 96, 
            mc::font::Small, 
            mc::app::theme->foregroundColor, 
            "Searching for controllers..."
        );
    }

    uint16_t y_offset = 130;
    unsigned int i;

    int x1 = 470;
    int x2 = 1189;
    int y1;
    int y2;

    // lay down separator lines
    for (i = 0; i < mc::btcore::discoveredDevices.size(); ++i) {
        y1 = y_offset + i*(70+1);
        mc::gfx::DrawHLine(x1, x2, y1, mc::app::theme->foregroundColor2);
    }
    mc::gfx::DrawHLine(470, 1189, y_offset + i*(70+1), mc::app::theme->foregroundColor2);

    i = 0;
    for (auto it = mc::btcore::discoveredDevices.begin(); it != mc::btcore::discoveredDevices.end(); ++it) {
        y1 = y_offset + i*(70+1);
        y2 = y1 + 70;

        if (i == m_selectionIdx) {
            mc::gfx::DrawRoundedRect(x1-5, y1-5, x2+5, y2+5, 2, mc::app::theme->glowColor);
            mc::gfx::DrawRect(x1, y1, x2, y2, mc::app::theme->selectionColor);
        }
        
        // draw controller image
        SDL_Rect clip = {mc::controller::ControllerType_Unknown*48, mc::app::theme->colorSetId*48, 48, 48};
        mc::gfx::DrawTexture(x1+30, y1 + 14, m_imgIcons, &clip);
        // draw controller details
        mc::gfx::DrawText(x1+100, y1 + 10, mc::font::Small, mc::app::theme->foregroundColor, (*it)->name);
        mc::gfx::DrawText(x1+100, y1 + 40, mc::font::Small, mc::app::theme->foregroundColor2, "%02X:%02X:%02X:%02X:%02X:%02X", 
            ((uint8_t *)&(*it)->address)[0],
			((uint8_t *)&(*it)->address)[1],
			((uint8_t *)&(*it)->address)[2],
			((uint8_t *)&(*it)->address)[3],
			((uint8_t *)&(*it)->address)[4],
			((uint8_t *)&(*it)->address)[5]
        );

        if ( (i == m_selectionIdx) && mc::btcore::IsPairing()) {
            mc::gfx::DrawGlyph(x1+600, y1 + 26, mc::font::ExtSmall, mc::app::theme->highlightColor, GlyphType_SpinnerTopLeft + mc::app::counter);
            mc::gfx::DrawText(x1+630, y1 + 26, mc::font::Small, mc::app::theme->highlightColor, "Pairing");
        }

        i++;
    }
    
    // draw console bd address
    /*
    mc::gfx::DrawText(430, 620, mc::font::Small, mc::app::theme->foregroundColor2, "Host address: %02X:%02X:%02X:%02X:%02X:%02X", 
            ((uint8_t *)&mc::btcore::hostAddress)[0],
			((uint8_t *)&mc::btcore::hostAddress)[1],
			((uint8_t *)&mc::btcore::hostAddress)[2],
			((uint8_t *)&mc::btcore::hostAddress)[3],
			((uint8_t *)&mc::btcore::hostAddress)[4],
			((uint8_t *)&mc::btcore::hostAddress)[5]
        );
    */
}

void PairingScene::handleInput(const mc::app::UserInput *input) {
    
    if (!mc::btcore::IsPairing()) {
        /* Move selection up */
        if (input->kDown & KEY_UP) {
            m_selectionIdx = std::max(m_selectionIdx-1, 0);
        }

        /* Move selection down */
        if (input->kDown & KEY_DOWN) {
                                                                                
            if (++m_selectionIdx >= mc::btcore::discoveredDevices.size()) {
                m_selectionIdx = m_selectionIdx - 1;
            }
            //m_selectionIdx = std::min(m_selectionIdx+1, mc::btcore::discoveredDevices.size()-1);
        }

        /* Pair with current selection */
        if (input->kDown & KEY_A) {
            if (m_selectionIdx >= 0) {
                mc::btcore::PairDevice(&mc::btcore::discoveredDevices[m_selectionIdx]->address);
            }
        }
    }
}
