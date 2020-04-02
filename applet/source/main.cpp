#include <algorithm>
#include <memory>
#include <vector>
#include <cstdio>
#include <cstring>
#include <switch.h>

#include "reboot.h"
#include "patches.h"

#include "application.hpp"
#include "btcore.hpp"
#include "bluetoothdatabase.hpp"
#include "hidgamepad.hpp"
#include "controllermanager.hpp"

#include "gfx/graphics.hpp"
#include "scenes/sidemenu.hpp"
#include "scenes/pairingscene.hpp"
#include "scenes/testcontrollerscene.hpp"
#include "scenes/configurecontrollerscene.hpp"
#include "scenes/databasescene.hpp"
#include "scenes/settingsscene.hpp"

// Adjust size as needed.
#define INNER_HEAP_SIZE 0x80000

#ifdef __cplusplus
extern "C" {
#endif

u32 __nx_applet_type = AppletType_LibraryApplet;
u32 __nx_applet_exit_mode = 1;

/*
size_t nx_inner_heap_size = INNER_HEAP_SIZE;
char   nx_inner_heap[INNER_HEAP_SIZE];

void __libnx_initheap(void) {
	void*  addr = nx_inner_heap;
	size_t size = nx_inner_heap_size;

	// Newlib
	extern char* fake_heap_start;
	extern char* fake_heap_end;

	fake_heap_start = (char*)addr;
	fake_heap_end   = (char*)addr + size;
}

void __attribute__((weak)) __appInit(void) {
    Result rc;

    // Initialize default services.
    rc = smInitialize();
    if (R_FAILED(rc))
        fatalThrow(MAKERESULT(Module_Libnx, LibnxError_InitFail_SM));

    rc = setsysInitialize();
    if (R_SUCCEEDED(rc)) {
        if (hosversionGet() == 0) {
            SetSysFirmwareVersion fw;
            rc = setsysGetFirmwareVersion(&fw);
            if (R_SUCCEEDED(rc))
                hosversionSet(MAKEHOSVERSION(fw.major, fw.minor, fw.micro));
        }
    }
    else {
        fatalThrow(rc);
    }


    rc = fsInitialize();
    if (R_FAILED(rc))
        fatalThrow(MAKERESULT(Module_Libnx, LibnxError_InitFail_FS));

    fsdevMountSdmc();


    rc = btdrvInitialize();
    if R_FAILED(rc)
        fatalThrow(rc);

    rc = hidInitialize(); 
    if R_FAILED(rc)
        fatalThrow(rc);

    rc = splInitialize();
    if R_FAILED(rc)
        fatalThrow(rc);

    rc = romfsInit();
    if R_FAILED(rc)
        fatalThrow(rc);

    rc = plInitialize();
    if R_FAILED(rc)
        fatalThrow(rc);
}

void __attribute__((weak)) userAppExit(void);

void __attribute__((weak)) __appExit(void) {
    setsysExit();
    plExit();
    romfsExit();
    splExit();
    hidExit();
    btdrvExit();

    fsdevUnmountAll();
    fsExit();
    smExit();
}
*/


void userAppInit(void) {
    Result rc;

    rc = btdrvInitialize();
    if R_FAILED(rc)
        fatalThrow(rc);

    rc = splInitialize();
    if R_FAILED(rc)
        fatalThrow(rc);

    rc = romfsInit();
    if R_FAILED(rc)
        fatalThrow(rc);

    rc = plInitialize();
    if R_FAILED(rc)
        fatalThrow(rc);

    rc = setsysInitialize();
    if R_FAILED(rc)
        fatalThrow(rc);
}

void userAppExit(void) {   
    setsysExit();
    plExit();
    romfsExit();
    splExit();
    btdrvExit();
}


#ifdef __cplusplus
}
#endif


void draw_dialog(const char *message) {
    // translucent overlay
    mc::gfx::DrawRect(0, 0, SCREEN_W, SCREEN_H, mc::app::theme->dialogOverlayColor);
    // dialog background
    mc::gfx::DrawRoundedRect(255, 215, 1024, 505, 4, mc::app::theme->backgroundColor2);
    // dialog text
    mc::gfx::DrawText(310, 300, mc::font::Medium, mc::app::theme->foregroundColor, message);

    // buttons
    // horizontal separator
    mc::gfx::DrawRect(255, 434, 1024, 435, mc::app::theme->dialogSeparatorColor);
    // button highlight
    mc::gfx::DrawRoundedRect(250, 431, 1029, 510, 8, mc::app::theme->glowColor);
    // button background
    mc::gfx::DrawRoundedRect(255, 436, 1024, 505, 3, mc::app::theme->backgroundColor3);

    // vertical spacer
    mc::gfx::DrawRect(639, 436, 640, 505, mc::app::theme->dialogSeparatorColor);

    // button text
    mc::gfx::DrawText(400, 460, mc::font::Medium, mc::app::theme->highlightColor, "Cancel");
    mc::gfx::DrawText(790, 460, mc::font::Medium, mc::app::theme->highlightColor, "Generate");  
}

/*

void show_patch_dialog(void) {

    while (appletMainLoop()) {
        draw_dialog("Bluetooth exefs patches not found for current firmware.\nGenerate now? (reboot required)");
    }
}
*/


class MissionControl {
    public:
        MissionControl();
        ~MissionControl();

        void run(void);

    private:
        SDL_Texture *m_imgAppLogo;

        std::unique_ptr<SideMenu> m_sideMenu;
        std::vector<std::unique_ptr<mc::ui::Scene>> m_scenes;

};

MissionControl::MissionControl() {
    mc::app::Initialise();
    mc::gfx::Initialise();
    mc::font::Initialise();
    mc::btcore::Initialise();

    m_imgAppLogo = mc::gfx::LoadTexture("romfs:/images/applogo.png");
}

MissionControl::~MissionControl() {
    mc::gfx::DestroyTexture(m_imgAppLogo);

    mc::btcore::Cleanup();
    mc::font::Finalise();
    mc::gfx::Finalise();
    mc::app::Finalise();
}

void MissionControl::run(void) {
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

    /*
    if (!check_bluetooth_patches()) {
        //draw_dialog("Bluetooth exefs patches not found for current firmware.\nGenerate now? (reboot required)");    
        generate_bluetooth_patches();
        reboot();
    }
    */

    /* Start bluetooth discovery */
    rc = mc::btcore::StartDiscovery();
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
        // warning meswsage
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

        //draw_dialog("Test dialog");


        // ================================================================================================================
        // Handle inputs 

        // Read user inputs
        mc::app::ScanInput(&input);

        if ((input.kDown & KEY_PLUS) && !m_scenes[MenuItem_TestController]->hasFocus()) {
            mc::btcore::StopDiscovery();
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

        // ================================================================================================================

        mc::gfx::Present();

        svcSleepThread(1.6e7);
    }

    /* Set exit flag to signal threads to exit */
    mc::app::exitFlag = true;
}

void appletRun(void) {
    auto applet = std::make_unique<MissionControl>();
    applet->run();
}

int main(int argc, char* argv[]) {
    appletLockExit();
    appletRun();
    appletUnlockExit();
    return 0;
}
