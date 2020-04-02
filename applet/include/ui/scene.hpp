#pragma once

#include "application.hpp"
#include "element.hpp"

namespace mc::ui {

    class Scene {
        public:
            Scene();
            virtual ~Scene() {};

            void setFocus(bool focus);
            bool hasFocus(void);

            bool requestFocus(Scene *scene);
            bool releaseFocus(Scene *scene);

            virtual void draw(void) {};
            virtual void handleInput(const mc::app::UserInput *input) {};

        private:
            bool m_hasFocus;
        
    };

}
