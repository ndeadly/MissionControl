#include "xboxone.hpp"

namespace controller {

    namespace {

        const constexpr uint8_t xboxone_joystick_nbits = 16;

    }

    void XboxOneController::convertReportFormat(HidReport *report) {
        const XboxOneReportData *reportData = reinterpret_cast<const XboxOneReportData *>(&report->data);

        switch(report->id) {
            case 0x01:
                handleInputReport0x01(reportData);
                break;

            case 0x02:
                handleInputReport0x02(reportData);
                break;

            default:
                break;
        }
    }

    void XboxOneController::mapStickValues(JoystickPosition *dst, const XboxOneStickData *src) {
        /*
        dst->dx = unsigned_to_signed(src->x, xboxone_joystick_nbits);
        dst->dy = -unsigned_to_signed(src->y, xboxone_joystick_nbits);

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

    void XboxOneController::handleInputReport0x01(const XboxOneReportData *data) {

    }

    void XboxOneController::handleInputReport0x02(const XboxOneReportData *data) {

    }

}
