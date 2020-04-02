#pragma once

#include "application.hpp"
#include "ui/scene.hpp"

class TestControllerScene : public mc::ui::Scene{

    public:
        TestControllerScene();
        ~TestControllerScene();

        void draw(void);
        void handleInput(const mc::app::UserInput *input);

    private:
        SDL_Texture *m_imgswitchpro;
        
};

