#pragma once

#include "application.hpp"
#include "ui/scene.hpp"

class SettingsScene : public mc::ui::Scene {

    public:
        void draw(void);
        void handleInput(const mc::app::UserInput *input);

};
