
#include "wiiupro.hpp"

#include <stratosphere.hpp>

#include "../btdrv_mitm_logging.hpp"

namespace ams::controller {

    WiiUProController::WiiUProController(const bluetooth::Address *address) 
    : WiiController(ControllerType_WiiUPro, address) {

    }

    Result WiiUProController::initialize(void) {
        WiiController::initialize();
        
        // This should actually probably be run in response to report 0x20
        R_TRY(this->sendInit1(&m_address));
        R_TRY(this->sendInit2(&m_address));
        R_TRY(this->setReportMode(&m_address, 0x34));

        return 0;
    }

    Result WiiUProController::sendInit1(const bluetooth::Address *address) {
        const uint8_t data[] = {0x55};
        return this->writeMemory(address, 0x04a400f0, data, sizeof(data));
    }

    Result WiiUProController::sendInit2(const bluetooth::Address *address) {
        const uint8_t data[] = {0x00};
        return this->writeMemory(address, 0x04a400fb, data, sizeof(data));
    }

    void WiiUProController::convertReportFormat(const bluetooth::HidReport *inReport, bluetooth::HidReport *outReport) {
        auto wiiUReport = reinterpret_cast<const WiiUProReportData *>(&inReport->data);
        auto switchReport = reinterpret_cast<SwitchReportData *>(&outReport->data);

        outReport->size = 0x31;
        switchReport->id = 0x30;
        switchReport->report0x30.conn_info = 0x0;
        switchReport->report0x30.battery = 0x8;

        switch(wiiUReport->id) {
            case 0x20:  //extension connected
                this->handleInputReport0x20(wiiUReport, switchReport);
                break;

            //case 0x22:  // Acknowledgement
                //break;

            //case 0x32:  // Buttons + Ext bytes
                //break;

            case 0x34:  // Buttons + Ext bytes
                this->handleInputReport0x34(wiiUReport, switchReport);
                break;

            default:
                BTDRV_LOG_FMT("WIIUPRO: RECEIVED REPORT [0x%02x]", wiiUReport->id);
                break;
        }
    }

    void WiiUProController::handleInputReport0x20(const WiiUProReportData *src, SwitchReportData *dst) {

    }

    void WiiUProController::handleInputReport0x34(const WiiUProReportData *src, SwitchReportData *dst) {
        packStickData(&dst->report0x30.left_stick,
            ((3 * (src->report0x34.left_stick_x - STICK_ZERO)) >> 1) + STICK_ZERO, 
            ((3 * (src->report0x34.left_stick_y - STICK_ZERO)) >> 1) + STICK_ZERO
        );
        packStickData(&dst->report0x30.right_stick,
            ((3 * (src->report0x34.right_stick_x - STICK_ZERO)) >> 1) + STICK_ZERO,
            ((3 * (src->report0x34.right_stick_y - STICK_ZERO)) >> 1) + STICK_ZERO
        );

        dst->report0x30.buttons.dpad_down   = !src->report0x34.buttons.dpad_down;
        dst->report0x30.buttons.dpad_up     = !src->report0x34.buttons.dpad_up;
        dst->report0x30.buttons.dpad_right  = !src->report0x34.buttons.dpad_right;
        dst->report0x30.buttons.dpad_left   = !src->report0x34.buttons.dpad_left;

        dst->report0x30.buttons.A = !src->report0x34.buttons.A;
        dst->report0x30.buttons.B = !src->report0x34.buttons.B;
        dst->report0x30.buttons.X = !src->report0x34.buttons.X;
        dst->report0x30.buttons.Y = !src->report0x34.buttons.Y;

        dst->report0x30.buttons.R  = !src->report0x34.buttons.R;
        dst->report0x30.buttons.ZR = !src->report0x34.buttons.ZR;
        dst->report0x30.buttons.L  = !src->report0x34.buttons.L;
        dst->report0x30.buttons.ZL = !src->report0x34.buttons.ZL;

        dst->report0x30.buttons.minus = !src->report0x34.buttons.minus;
        dst->report0x30.buttons.plus  = !src->report0x34.buttons.plus;

        dst->report0x30.buttons.lstick_press = !src->report0x34.buttons.lstick_press;
        dst->report0x30.buttons.rstick_press = !src->report0x34.buttons.rstick_press;

        dst->report0x30.buttons.home = !src->report0x34.buttons.home;
    }

}
