#include <cstring>
#include "controllers/wiimote.hpp"

namespace mc::controller {

    Result WiimoteController::receiveReport(const HidReport *report) {

        const WiimoteReportData *reportData = reinterpret_cast<const WiimoteReportData *>(&report->data);
        
        switch(report->id) {
            case 0x30:
                handleInputReport0x30(reportData);
                break;

            default:
                return -1;
        }

        return 0;
    }

    void WiimoteController::handleInputReport0x30(const WiimoteReportData *data) {
        std::memset(&m_state, 0, sizeof(m_state));

        m_state.dpad_left     = data->report0x30.buttons.dpad_up;
        m_state.dpad_up       = data->report0x30.buttons.dpad_right;
        m_state.dpad_right    = data->report0x30.buttons.dpad_down;
        m_state.dpad_down     = data->report0x30.buttons.dpad_left;

        m_state.A             = data->report0x30.buttons.two;
        m_state.B             = data->report0x30.buttons.one;

        m_state.L             = data->report0x30.buttons.B;
        m_state.R             = data->report0x30.buttons.A;

        m_state.plus          = data->report0x30.buttons.plus;
        m_state.minus         = data->report0x30.buttons.minus;
        m_state.home          = data->report0x30.buttons.home;

        m_virtual->setState(&m_state);
    }

}
