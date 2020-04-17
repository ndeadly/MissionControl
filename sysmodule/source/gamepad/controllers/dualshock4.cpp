#include <cstring>
#include <cmath>
#include "gamepad/controllers/dualshock4.hpp"

namespace mc::controller {

    namespace {

        const constexpr uint8_t dualshock4_joystick_nbits = 8;

    }

    Dualshock4Controller::Dualshock4Controller(HidInterfaceType iface) : HidGamepad(iface) { 
        this->setInnerDeadzone(0.1);
    }

    void Dualshock4Controller::mapStickValues(JoystickPosition *dst, const Dualshock4StickData *src) {
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
    }

    Result Dualshock4Controller::receiveReport(const HidReport *report) {

        const Dualshock4ReportData *reportData = reinterpret_cast<const Dualshock4ReportData *>(&report->data);

        switch(report->id) {
            case 0x01:
                handleInputReport0x01(reportData);
                break;

            case 0x11:
                handleInputReport0x11(reportData);
                break;

            default:
                return -1;
        }
        
        return 0;
    }

    void Dualshock4Controller::handleInputReport0x01(const Dualshock4ReportData *data) {
        std::memset(&m_state, 0, sizeof(m_state));

        this->mapStickValues(&m_state.left_stick, &data->report0x01.left_stick);
        this->mapStickValues(&m_state.right_stick, &data->report0x01.right_stick);

        m_state.dpad_left   = (data->report0x01.buttons.dpad == Dualshock4DPad_W)  ||
                                      (data->report0x01.buttons.dpad == Dualshock4DPad_NW) ||
                                      (data->report0x01.buttons.dpad == Dualshock4DPad_SW);
        m_state.dpad_up     = (data->report0x11.buttons.dpad == Dualshock4DPad_N)  ||
                                      (data->report0x01.buttons.dpad == Dualshock4DPad_NE) ||
                                      (data->report0x01.buttons.dpad == Dualshock4DPad_NW);
        m_state.dpad_right  = (data->report0x11.buttons.dpad == Dualshock4DPad_E)  ||
                                      (data->report0x01.buttons.dpad == Dualshock4DPad_NE) ||
                                      (data->report0x01.buttons.dpad == Dualshock4DPad_SE);
        m_state.dpad_down   = (data->report0x11.buttons.dpad == Dualshock4DPad_S)  ||
                                      (data->report0x01.buttons.dpad == Dualshock4DPad_SE) ||
                                      (data->report0x01.buttons.dpad == Dualshock4DPad_SW);

        m_state.Y             = data->report0x01.buttons.square;
        m_state.X             = data->report0x01.buttons.triangle;
        m_state.B             = data->report0x01.buttons.cross;
        m_state.A             = data->report0x01.buttons.circle;

        m_state.L             = data->report0x01.buttons.L1;
        m_state.ZL            = data->report0x01.buttons.L2;
        m_state.lstick_press  = data->report0x01.buttons.L3;

        m_state.R             = data->report0x01.buttons.R1;
        m_state.ZR            = data->report0x01.buttons.R2;
        m_state.rstick_press  = data->report0x01.buttons.R3;

        m_state.minus         = data->report0x01.buttons.share;
        m_state.plus          = data->report0x01.buttons.options;
        m_state.capture       = data->report0x01.buttons.tpad;
        m_state.home          = data->report0x01.buttons.ps;

        m_virtual->setState(&m_state);
    }

    void Dualshock4Controller::handleInputReport0x11(const Dualshock4ReportData *data) {
        std::memset(&m_state, 0, sizeof(m_state));

        this->mapStickValues(&m_state.left_stick, &data->report0x11.left_stick);
        this->mapStickValues(&m_state.right_stick, &data->report0x11.right_stick);

        m_state.dpad_left   = (data->report0x11.buttons.dpad == Dualshock4DPad_W)  ||
                                      (data->report0x11.buttons.dpad == Dualshock4DPad_NW) ||
                                      (data->report0x11.buttons.dpad == Dualshock4DPad_SW);
        m_state.dpad_up     = (data->report0x11.buttons.dpad == Dualshock4DPad_N)  ||
                                      (data->report0x11.buttons.dpad == Dualshock4DPad_NE) ||
                                      (data->report0x11.buttons.dpad == Dualshock4DPad_NW);
        m_state.dpad_right  = (data->report0x11.buttons.dpad == Dualshock4DPad_E)  ||
                                      (data->report0x11.buttons.dpad == Dualshock4DPad_NE) ||
                                      (data->report0x11.buttons.dpad == Dualshock4DPad_SE);
        m_state.dpad_down   = (data->report0x11.buttons.dpad == Dualshock4DPad_S)  ||
                                      (data->report0x11.buttons.dpad == Dualshock4DPad_SE) ||
                                      (data->report0x11.buttons.dpad == Dualshock4DPad_SW);

        m_state.Y             = data->report0x11.buttons.square;
        m_state.X             = data->report0x11.buttons.triangle;
        m_state.B             = data->report0x11.buttons.cross;
        m_state.A             = data->report0x11.buttons.circle;

        m_state.L             = data->report0x11.buttons.L1;
        m_state.ZL            = data->report0x11.buttons.L2;
        m_state.lstick_press  = data->report0x11.buttons.L3;

        m_state.R             = data->report0x11.buttons.R1;
        m_state.ZR            = data->report0x11.buttons.R2;
        m_state.rstick_press  = data->report0x11.buttons.R3;

        m_state.minus         = data->report0x11.buttons.share;
        m_state.plus          = data->report0x11.buttons.options;
        m_state.capture       = data->report0x11.buttons.tpad;
        m_state.home          = data->report0x11.buttons.ps;

        m_virtual->setState(&m_state);


    }

}
