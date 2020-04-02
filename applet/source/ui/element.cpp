#include "ui/element.hpp"

namespace mc::ui {

    Element::Element(std::shared_ptr<Element> parent) {

    }

    void Element::setPosition(uint16_t x, uint16_t y) {
        m_posX = x;
        m_posY = y;
    }

    void Element::setPositionX(uint16_t x) {
        m_posX = x;
    }

    void Element::setPositionY(uint16_t y) {
        m_posY = y;
    }

    void Element::setWidth(uint16_t width) {
        m_width = width;
    }

    void Element::setHeight(uint16_t height) {
        m_height = height;
    }

    void Element::setSize(uint16_t width, uint16_t height) {
        m_width = width;
        m_height = height;
    }

    void Element::setRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
        m_posX = x;
        m_posY = y;
        m_width = width;
        m_height = height;
    }

    uint16_t Element::positionX(void) {
        return m_posX;
    }

    uint16_t Element::positionY(void) {
        return m_posY;
    }

    uint16_t Element::width(void) {
        return m_width;
    }

    uint16_t Element::height(void) {
        return m_height;
    }

    void Element::setParent(std::shared_ptr<Element> parent) {

    }

    void Element::addElement(std::shared_ptr<Element> element) {
        m_children.push_back(element);
        //element->setParent(std:make_we this);
    }

    void Element::removeElement(std::shared_ptr<Element> element) {
        auto it = m_children.begin();
        while (it != m_children.end()) {
            if ((*it) == element) {
                it = m_children.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    void Element::update(void) {

    }

    void Element::draw(void) {
        this->update();

        for (auto child : m_children) {
            child->draw();
        }
    }

}
