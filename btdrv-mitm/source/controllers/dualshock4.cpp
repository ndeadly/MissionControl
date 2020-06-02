#include "dualshock4.hpp"
#include <cmath>


namespace controller {

    namespace {

        const constexpr uint8_t dualshock4_joystick_nbits = 8;

    }

    void Dualshock4Controller::convertReportFormat(HidReport *report) {

    }

    void Dualshock4Controller::mapStickValues(JoystickPosition *dst, const Dualshock4StickData *src) {
        /*
        dst->dx = unsigned_to_signed(src->x, dualshock4_joystick_nbits);
        dst->dy = -unsigned_to_signed(src->y, dualshock4_joystick_nbits);

        float angle = atan2(dst->dy, dst->dx);
        float magnitude = hypot(dst->dx, dst->dy);

        if (magnitude < m_innerDeadzone) {
            dst->dx = 0;
            dst->dy = 0;
        }
        else if (magnitude > m_outerDeadzone) {
            dst->dx = JOYSTICK_MAX * cos(angle);
            dst->dy = JOYSTICK_MAX * sin(angle);
        }
        */
    }

    void Dualshock4Controller::handleInputReport0x01(const Dualshock4ReportData *data) {

    }

    void Dualshock4Controller::handleInputReport0x11(const Dualshock4ReportData *data) {

    }

}
