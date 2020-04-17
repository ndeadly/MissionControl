#include <cstring>
#include <cmath>
#include "gamepad/controllers/switchpro.hpp"

namespace mc::controller {

    namespace {

        const constexpr uint8_t switchpro_joystick_nbits = 12;

    }

    void SwitchProController::mapStickValues(JoystickPosition *dst, const SwitchProStickData *src) {
        dst->dx = unsigned_to_signed(src->x, switchpro_joystick_nbits);
        dst->dy = -unsigned_to_signed(src->y, switchpro_joystick_nbits);

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

    Result SwitchProController::receiveReport(const HidReport *report) {

        const SwitchProReportData *reportData = reinterpret_cast<const SwitchProReportData *>(&report->data);

        switch(report->id) {
            case 0x30:
                handleInputReport0x30(reportData);
                break;

            default:
                return -1;
        }
        
        return 0;
    }

    void SwitchProController::handleInputReport0x30(const SwitchProReportData *data) {
        std::memset(&m_state, 0, sizeof(m_state));

        this->mapStickValues(&m_state.left_stick, &data->report0x30.left_stick);
        this->mapStickValues(&m_state.right_stick, &data->report0x30.right_stick);

        m_state.dpad_left       = data->report0x30.buttons.dpad_left;
        m_state.dpad_up         = data->report0x30.buttons.dpad_up;
        m_state.dpad_right      = data->report0x30.buttons.dpad_right;
        m_state.dpad_down       = data->report0x30.buttons.dpad_down;

        m_state.A               = data->report0x30.buttons.A;
        m_state.B               = data->report0x30.buttons.B;
        m_state.X               = data->report0x30.buttons.X;
        m_state.Y               = data->report0x30.buttons.Y;

        m_state.L               = data->report0x30.buttons.L;
        m_state.ZL              = data->report0x30.buttons.ZL;
        m_state.lstick_press    = data->report0x30.buttons.lstick_press;

        m_state.R               = data->report0x30.buttons.R;
        m_state.ZR              = data->report0x30.buttons.ZR;
        m_state.rstick_press    = data->report0x30.buttons.rstick_press;

        m_state.minus           = data->report0x30.buttons.minus;
        m_state.plus                    = data->report0x30.buttons.plus;
        m_state.capture         = data->report0x30.buttons.capture;
        m_state.home            = data->report0x30.buttons.home;

        m_virtual->setState(&m_state);
    }

}
