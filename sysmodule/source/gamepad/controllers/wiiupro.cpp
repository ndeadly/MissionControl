#include <cstring>
#include "gamepad/controllers/wiiupro.hpp"

#include "log.hpp"

namespace mc::controller {

    namespace {

        const constexpr uint8_t wiiupro_joystick_nbits = 12;

        Result send_init_1(const BluetoothAddress *address) {
            const uint8_t data[] = {0x55};
            return write_wiimote_memory(address, 0x04a400f0, data, sizeof(data));
        }

        Result send_init_2(const BluetoothAddress *address) {
            const uint8_t data[] = {0x00};
            return write_wiimote_memory(address, 0x04a400fb, data, sizeof(data));
        }

        Result set_report_mode(const BluetoothAddress *address, uint8_t mode) {
            uint8_t data[] = {0x12, 0x00, mode};
            BluetoothHidData hidData = {};
            hidData.length = sizeof(data);
            std::memcpy(&hidData.data, data, sizeof(data));
            return btdrvWriteHidData(address, &hidData);
        }

        Result set_player_leds(const BluetoothAddress *address, uint8_t mask) {
            uint8_t data[] = {0x11, mask};    // set player led
            BluetoothHidData hidData = {};
            hidData.length = sizeof(data);
            std::memcpy(&hidData.data, data, sizeof(data));
            return btdrvWriteHidData(address, &hidData);
        }

    }

    WiiUProController::WiiUProController(HidInterfaceType iface) : HidGamepad(iface) { 
        this->setInnerDeadzone(0.15);
    }

    void WiiUProController::mapStickValues(JoystickPosition *dst, uint16_t x, uint16_t y) {
        dst->dx = 2*unsigned_to_signed(x, wiiupro_joystick_nbits);
        dst->dy = 2*unsigned_to_signed(y, wiiupro_joystick_nbits);

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

    Result WiiUProController::receiveReport(const HidReport *report) {

        const WiiUProReportData *reportData = reinterpret_cast<const WiiUProReportData *>(&report->data);

        switch(report->id) {
            case 0x20:  //extension connected
                handleInputReport0x20(reportData);
                break;

            case 0x22:  // Acknowledgement
                break;

            case 0x32:  // Buttons + Ext bytes
                break;

            case 0x34:  // Buttons + Ext bytes
                handleInputReport0x34(reportData);
                break;

            default:
                return -1;
        }

        return 0;
    }

    void WiiUProController::handleInputReport0x20(const WiiUProReportData *data) {
        Result rc;
        const BluetoothAddress address = this->m_btInterface->address();

        rc = set_player_leds(&address, WiimoteLEDs_P1);                 
        rc = send_init_1(&address);
        rc = send_init_2(&address);
        rc = set_report_mode(&address, 0x34);                 

    }

    void WiiUProController::handleInputReport0x34(const WiiUProReportData *data) {
        std::memset(&m_state, 0, sizeof(m_state));

        /*
        this->mapStickValues(&m_state.left_stick, data->report0x34.left_stick_x, data->report0x34.left_stick_y, 6);
        this->mapStickValues(&m_state.right_stick, 
            data->report0x34.right_stick_x0 | (data->report0x34.right_stick_x1 << 1) | (data->report0x34.right_stick_x2 << 3), 
            data->report0x34.right_stick_y, 5);
        */
        this->mapStickValues(&m_state.left_stick, data->report0x34.left_stick_x, data->report0x34.left_stick_y);
        this->mapStickValues(&m_state.right_stick, data->report0x34.right_stick_x, data->report0x34.right_stick_y);

        m_state.dpad_left     = !data->report0x34.buttons.dpad_left;
        m_state.dpad_up       = !data->report0x34.buttons.dpad_up;
        m_state.dpad_right    = !data->report0x34.buttons.dpad_right;
        m_state.dpad_down     = !data->report0x34.buttons.dpad_down;

        m_state.A             = !data->report0x34.buttons.A;
        m_state.B             = !data->report0x34.buttons.B;
        m_state.X             = !data->report0x34.buttons.X;
        m_state.Y             = !data->report0x34.buttons.Y;

        m_state.L             = !data->report0x34.buttons.L;
        m_state.ZL            = !data->report0x34.buttons.ZL;
        m_state.lstick_press  = !data->report0x34.buttons.lstick_press;

        m_state.R             = !data->report0x34.buttons.R;
        m_state.ZR            = !data->report0x34.buttons.ZR;
        m_state.rstick_press  = !data->report0x34.buttons.rstick_press;

        m_state.plus          = !data->report0x34.buttons.plus;
        m_state.minus         = !data->report0x34.buttons.minus;
        m_state.home          = !data->report0x34.buttons.home;

        m_virtual->setState(&m_state);
    }

}
