#pragma once

#include <memory>
#include <vector>
#include <switch.h>

namespace mc::ui {

    class Element {
        public:
            Element(std::shared_ptr<Element> parent = nullptr);
            //~Element();

            void setPosition(uint16_t x, uint16_t y);
            void setPositionX(uint16_t x);
            void setPositionY(uint16_t x);
            void setWidth(uint16_t width);
            void setHeight(uint16_t height);
            void setSize(uint16_t width, uint16_t height);
            void setRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height);

            uint16_t positionX(void);
            uint16_t positionY(void);
            uint16_t width(void);
            uint16_t height(void);

            void setParent(std::shared_ptr<Element> parent);
            void addElement(std::shared_ptr<Element> element);
            void removeElement(std::shared_ptr<Element> element);

            void update(void);
            void draw(void);

        private:
            uint16_t m_posX;
            uint16_t m_posY;
            uint16_t m_width;
            uint16_t m_height;

            std::weak_ptr<Element> m_parent;
            std::vector<std::shared_ptr<Element>> m_children;

    };

}
