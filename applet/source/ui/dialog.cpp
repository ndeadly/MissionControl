#include "ui/dialog.hpp"

namespace mc::ui {

    bool Dialog::isVisible(void) {
        return m_visible;
    }

    void Dialog::show(void) {
        m_visible = true;
    }

    void Dialog::hide(void) {
        m_visible = false;
    }
}
