#pragma once

#include <switch.h>

namespace mc::ui {

    struct MenuItem {
        const char *name;
        
    };

    class Menu {
        public:
            Menu();
            ~Menu();

            void draw(void);
            
        private:

    };

}
