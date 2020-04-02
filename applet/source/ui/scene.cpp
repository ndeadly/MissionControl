#include "ui/scene.hpp"

namespace mc::ui {

    Scene::Scene() : m_hasFocus(false) {

    }

    void Scene::setFocus(bool focus) {
        m_hasFocus = focus;
    }

    bool Scene::hasFocus(void) {
        return m_hasFocus;
    }

}
