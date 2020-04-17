#include <cstring>
#include "gamepad/controllers/wiimote.hpp"
#include "byteswap.h"

#include "log.hpp"

namespace mc::controller {

    WiimoteController::WiimoteController(HidInterfaceType iface) : HidGamepad(iface) { 
        //this->setInnerDeadzone(0.1);

        /*
        uint8_t reportdata[] = {0x11, 0x10};    // set player led

        BluetoothHidData hidData = {};
        hidData.length = sizeof(reportdata);
        std::memcpy(&hidData.data, &reportdata, sizeof(reportdata));

        btdrvHidSetReport(&address, HidReportType_OutputReport, &hidData);
        */
    }

    Result write_wiimote_memory(const BluetoothAddress *bd_addr, uint32_t address, const uint8_t *data, uint8_t length) {
        const struct {
            uint8_t id;
            uint32_t address;
            uint8_t length;
            uint8_t data[0x10];
        } __attribute__((packed)) report = {0x16, __bswap_32(address), length, *data};
        //std::memcpy(report.data, data, length);

        BluetoothHidData hidData = {};
        hidData.length = sizeof(report);
        std::memcpy(&hidData.data, &report, sizeof(report));

        mc::log::Write("Writing to wii remote control registers");
        mc::log::WriteData(&hidData.data, hidData.length);

        return btdrvWriteHidData(bd_addr, &hidData);
    }


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
