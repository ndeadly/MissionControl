#pragma once

#include <memory>
#include <vector>

#include "gfx/graphics.hpp"
#include "ui/dialog.hpp"
#include "scenes/sidemenu.hpp"

class MainApplet {
    public:
        MainApplet();
        ~MainApplet();

        void run(void);

    private:
        SDL_Texture *m_imgAppLogo;

        std::unique_ptr<mc::ui::Dialog> m_activeDialog;
        std::unique_ptr<SideMenu> m_sideMenu;
        std::vector<std::unique_ptr<mc::ui::Scene>> m_scenes;

};
