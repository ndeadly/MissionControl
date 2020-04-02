#pragma once

#include "application.hpp"
#include "ui/scene.hpp"

enum MenuOptions {
    MenuItem_PairController,
    MenuItem_TestController,
    //MenuItem_ConfigureControls,
    MenuItem_ViewDatabase,
    MenuItem_Settings,
};

class SideMenu : public mc::ui::Scene {

    public:
        SideMenu() : mc::ui::Scene(), m_selectionIdx(0) {};

        void draw(void);
        void handleInput(const mc::app::UserInput *input);

        int  index(void) { return m_selectionIdx; }
        void setIndex(int index) { m_selectionIdx = index; };

    private:
        int m_selectionIdx;

};
