#pragma once

#include "application.hpp"

enum DialogSelectionType {
    DialogSelectionType_Ok,
    DialogSelectionType_Cancel
};

namespace mc::ui {

    class Dialog {
        
        public:
            Dialog(const char *message) : m_message(message), m_visible(false) {};
            virtual ~Dialog() {};

            bool isVisible(void);
            void show(void);
            void hide(void);

            virtual void accept(void) {};
            virtual void cancel(void) {};

            virtual void draw(void) {};
            virtual void handleInput(const mc::app::UserInput *input) {};
            
        protected:
            const char *m_message;
            bool m_visible;

    };

}
