#include "mainapplet.hpp"

#include "patches.h"

#include "error.hpp"
#include "application.hpp"
#include "bluetooth/core.hpp"

#include "dialogs/patch_dialog.hpp"

#include "scenes/pairingscene.hpp"
#include "scenes/testcontrollerscene.hpp"
#include "scenes/configurecontrollerscene.hpp"
#include "scenes/databasescene.hpp"
#include "scenes/settingsscene.hpp"

MainApplet::MainApplet() {
    mc::app::Initialise();
    mc::gfx::Initialise();
    mc::font::Initialise();
    mc::bluetooth::core::Initialise();

    m_imgAppLogo = mc::gfx::LoadTexture("romfs:/images/applogo.png");

    m_activeDialog = nullptr;
}

MainApplet::~MainApplet() {
    mc::gfx::DestroyTexture(m_imgAppLogo);

    mc::bluetooth::core::Finalise();
    mc::font::Finalise();
    mc::gfx::Finalise();
    mc::app::Finalise();
}

void MainApplet::run(void) {
    Result rc;

    /* Create side menu */
    std::unique_ptr<SideMenu> m_sideMenu = std::make_unique<SideMenu>();
    m_sideMenu->setFocus(true);

    /* Create Gui Scenes */
    std::vector<std::unique_ptr<mc::ui::Scene>> m_scenes;
    m_scenes.emplace_back(std::make_unique<PairingScene>());
    m_scenes.emplace_back(std::make_unique<TestControllerScene>());
    //scenes.emplace_back(std::make_unique<ConfigureControllerScene>());
    m_scenes.emplace_back(std::make_unique<DatabaseScene>());
    m_scenes.emplace_back(std::make_unique<SettingsScene>());

    /* Ask user to generate IPS patches if not found */
    if (!check_bluetooth_patches()) {
        m_activeDialog = std::make_unique<PatchDialog>();
        m_activeDialog->show();
    }

    /* Start bluetooth discovery */
    rc = mc::bluetooth::core::StartDiscovery();
    if (R_FAILED(rc))
        fatalThrow(rc);

    mc::app::UserInput input = {};

    /* Begin event loop */
    while (appletMainLoop()) {

        mc::gfx::Clear();

        // ================================================================================================================
        // Draw the screen 

        // draw window background
        mc::gfx::DrawRect(0, 0,  SCREEN_W, SCREEN_H, mc::app::theme->backgroundColor);

        /* Draw the appropriate scene for the side menu selection */
        m_scenes[m_sideMenu->index()]->draw();
        
        /* Draw the rest of the main scene */
        m_sideMenu->draw();


        /* Top bar */
        // logo
        SDL_Rect clip = {0, mc::app::theme->colorSetId*48, 48, 48};
        mc::gfx::DrawTexture(66, 28, m_imgAppLogo, &clip);
        // window title
        mc::gfx::DrawText(130, 40, mc::font::Large, mc::app::theme->foregroundColor, "MissionControl");
        // warning message
        //Color warnColor;
        //warnColor.abgr = RGBA8(0xff, 0x00, 0x00, 0xff);
        //mc::gfx::DrawText(800, 50, mc::font::Medium, warnColor, "Alpha software. Not for distribution");
        // draw separator lines
        mc::gfx::DrawHLine(30, 1249, 87, mc::app::theme->foregroundColor);


        /* Buttom bar */
        mc::gfx::DrawHLine(30, 1249, 647, mc::app::theme->foregroundColor);
        if (m_scenes[MenuItem_TestController]->hasFocus()) {
            mc::gfx::DrawText(962, 670, mc::font::Medium, mc::app::theme->foregroundColor, "(");
            mc::gfx::DrawGlyph(980, 670, mc::font::ExtMedium, mc::app::theme->foregroundColor, GlyphType_LButtonInverted);
            mc::gfx::DrawText(1012, 670, mc::font::Medium, mc::app::theme->foregroundColor, "+");
            mc::gfx::DrawGlyph(1040, 670, mc::font::ExtMedium, mc::app::theme->foregroundColor, GlyphType_RButtonInverted);
            mc::gfx::DrawText(1072, 670, mc::font::Medium, mc::app::theme->foregroundColor, "+");
            mc::gfx::DrawGlyph(1100, 670, mc::font::ExtMedium, mc::app::theme->foregroundColor, GlyphType_JoyDownButtonInverted);
            mc::gfx::DrawText(1130, 670, mc::font::Medium, mc::app::theme->foregroundColor, ")");
            mc::gfx::DrawText(1150, 670, mc::font::Medium, mc::app::theme->foregroundColor, "Return");
        }
        else {
            mc::gfx::DrawGlyph(890, 670, mc::font::ExtMedium, mc::app::theme->foregroundColor, GlyphType_PlusButtonInverted);
            mc::gfx::DrawText(925, 670, mc::font::Medium, mc::app::theme->foregroundColor, "Exit");
            mc::gfx::DrawGlyph(1020, 670, mc::font::ExtMedium, mc::app::theme->foregroundColor, GlyphType_BButtonInverted);
            mc::gfx::DrawText(1055, 670, mc::font::Medium, mc::app::theme->foregroundColor, "Back");
            mc::gfx::DrawGlyph(1130, 670, mc::font::ExtMedium, mc::app::theme->foregroundColor, GlyphType_AButtonInverted);
            mc::gfx::DrawText(1165, 670, mc::font::Medium, mc::app::theme->foregroundColor, "OK");
        }

        // Read user inputs
        mc::app::ScanInput(&input);

        if ( (m_activeDialog != nullptr) && m_activeDialog->isVisible() ) {
            m_activeDialog->draw();
            m_activeDialog->handleInput(&input);
        }
        else {

            if ( ((input.kDown & KEY_PLUS) && !m_scenes[MenuItem_TestController]->hasFocus()) || 
                ((input.kDown & KEY_B) && m_sideMenu->hasFocus()) ) {
                mc::bluetooth::core::StopDiscovery();
                break;
            }

            if (m_sideMenu->hasFocus()) {

                if ( (input.kDown & KEY_RIGHT) || (input.kDown & KEY_A)) {
                    /*
                    menu_focus = false;
                    panel_idx = menu_idx;
                    pair_controller_idx = 0;

                    if (menu_idx == MenuItem_TestController) {
                        test_focus = true;
                    }
                    */
                    m_scenes[m_sideMenu->index()]->setFocus(true);
                    m_sideMenu->setFocus(false);
                } 
                else {
                    m_sideMenu->handleInput(&input);
                }

            } 
            else {

                if (m_scenes[MenuItem_TestController]->hasFocus()) {
                    /* Check for button combo to break out */
                    if ( (input.kHeld & KEY_L) && 
                        (input.kHeld & KEY_R) &&
                        (input.kHeld & KEY_DDOWN)) {
                        /*
                        test_focus = false;
                        menu_focus = true;
                        panel_idx = -1;
                        */
                        m_scenes[m_sideMenu->index()]->setFocus(false);
                        m_sideMenu->setFocus(true);
                    }
                }
                else {
                    if ( (input.kDown & KEY_LEFT) || (input.kDown & KEY_B)) {
                        /*
                        menu_focus = true;
                        panel_idx = -1;
                        pair_controller_idx = -1;
                        */
                    m_scenes[m_sideMenu->index()]->setFocus(false);
                    m_sideMenu->setFocus(true);
                    }
                }
                m_scenes[m_sideMenu->index()]->handleInput(&input);
            }
        }

        mc::gfx::Present();
        svcSleepThread(1.6e7);
    }

    /* Set exit flag to signal threads to exit */
    mc::app::exitFlag = true;
}