#include <cstring>
#include <cmath>
#include "gamepad/controllers/xboxone.hpp"

namespace mc::controller {

    namespace {

        const constexpr uint8_t xboxone_joystick_nbits = 16;

    }

    XboxOneController::XboxOneController(HidInterfaceType iface) : HidGamepad(iface) { 
        this->setInnerDeadzone(0.1);
    }

    void XboxOneController::mapStickValues(JoystickPosition *dst, const XboxOneStickData *src) {
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
    }

    Result XboxOneController::receiveReport(const HidReport *report) {
        
        const XboxOneReportData *reportData = reinterpret_cast<const XboxOneReportData *>(&report->data);

        switch(report->id) {
            case 0x01:
                handleInputReport0x01(reportData);
                break;

            case 0x02:
                handleInputReport0x02(reportData);
                break;

            default:
                return -1;
        }

        return 0;
    }

    void XboxOneController::handleInputReport0x01(const XboxOneReportData *data) {
        std::memset(&m_state, 0, sizeof(m_state));

        this->mapStickValues(&m_state.left_stick, &data->report0x01.left_stick);
        this->mapStickValues(&m_state.right_stick, &data->report0x01.right_stick);

        m_state.dpad_left   = (data->report0x01.buttons.dpad == XboxOneDPad_W)  ||
                                      (data->report0x01.buttons.dpad == XboxOneDPad_NW) ||
                                      (data->report0x01.buttons.dpad == XboxOneDPad_SW);
        m_state.dpad_up     = (data->report0x01.buttons.dpad == XboxOneDPad_N)  ||
                                      (data->report0x01.buttons.dpad == XboxOneDPad_NE) ||
                                      (data->report0x01.buttons.dpad == XboxOneDPad_NW);
        m_state.dpad_right  = (data->report0x01.buttons.dpad == XboxOneDPad_E)  ||
                                      (data->report0x01.buttons.dpad == XboxOneDPad_NE) ||
                                      (data->report0x01.buttons.dpad == XboxOneDPad_SE);
        m_state.dpad_down   = (data->report0x01.buttons.dpad == XboxOneDPad_S)  ||
                                      (data->report0x01.buttons.dpad == XboxOneDPad_SE) ||
                                      (data->report0x01.buttons.dpad == XboxOneDPad_SW);

        m_state.A             = data->report0x01.buttons.B;
        m_state.B             = data->report0x01.buttons.A;
        m_state.X             = data->report0x01.buttons.Y;
        m_state.Y             = data->report0x01.buttons.X;

        m_state.L             = data->report0x01.buttons.LB;
        m_state.ZL            = data->report0x01.left_trigger > 0;
        m_state.lstick_press  = data->report0x01.buttons.lstick_press;

        m_state.R             = data->report0x01.buttons.RB;
        m_state.ZR            = data->report0x01.right_trigger > 0;
        m_state.rstick_press  = data->report0x01.buttons.rstick_press;

        m_state.minus         = data->report0x01.buttons.view;
        m_state.plus          = data->report0x01.buttons.menu;
        //m_state.capture       = ;
        //m_state.home          = ;                    

        m_virtual->setState(&m_state);
    }

    void XboxOneController::handleInputReport0x02(const XboxOneReportData *data) {
        std::memset(&m_state, 0, sizeof(m_state));

        m_state.home = data->report0x02.guide;

        m_virtual->setState(&m_state);
    }

}
