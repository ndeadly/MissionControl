#pragma once

#include "application.hpp"
#include "ui/scene.hpp"

class PairingScene : public mc::ui::Scene {

    public:
        PairingScene();
        ~PairingScene();

        void draw(void);
        void handleInput(const mc::app::UserInput *input);

    private:
        SDL_Texture *m_imgIcons;

        int m_selectionIdx;
};
