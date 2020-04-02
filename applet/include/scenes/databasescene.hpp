#pragma once

#include "application.hpp"
#include "ui/scene.hpp"

class DatabaseScene : public mc::ui::Scene {

    public:
        DatabaseScene();
        ~DatabaseScene();
        
        void draw(void);
        void handleInput(const mc::app::UserInput *input);

    private:
        SDL_Texture *m_imgIcons;

};
