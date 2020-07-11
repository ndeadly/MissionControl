#include "wiimote.hpp"
#include <stratosphere.hpp>

#include "../btdrv_mitm_logging.hpp"

namespace ams::controller {

    WiimoteController::WiimoteController(const bluetooth::Address *address)  
    : WiiController(ControllerType_Wiimote, address) {

    }

    Result WiimoteController::initialize(void) {
        R_TRY(WiiController::initialize());

        R_TRY(this->setReportMode(&m_address, 0x31));

        return 0;
    }

    void WiimoteController::convertReportFormat(const bluetooth::HidReport *inReport, bluetooth::HidReport *outReport) {
        auto wiiReport = reinterpret_cast<const WiiReportData *>(&inReport->data);
        auto switchReport = reinterpret_cast<SwitchReportData *>(&outReport->data);

        switch(wiiReport->id) {
            case 0x20:  //extension connected
                this->handleInputReport0x20(wiiReport, switchReport);
                break;

            case 0x30:
                this->handleInputReport0x30(wiiReport, switchReport);
                break;

            case 0x31:
                this->handleInputReport0x31(wiiReport, switchReport);
                break;

            default:
                BTDRV_LOG_FMT("WIIMOTE: RECEIVED REPORT [0x%02x]", wiiReport->id);
                break;
        }

        outReport->size = 0x31;
        switchReport->id = 0x30;
        switchReport->input0x30.conn_info = 0x0;
        switchReport->input0x30.battery = m_battery | m_charging;
        switchReport->input0x30.timer = os::ConvertToTimeSpan(os::GetSystemTick()).GetMilliSeconds() & 0xff;
    }

    void WiimoteController::handleInputReport0x20(const WiiReportData *src, SwitchReportData *dst) {
        m_battery = convert8bitBatteryLevel(src->input0x20.battery);
    }

    void WiimoteController::handleInputReport0x30(const WiiReportData *src, SwitchReportData *dst) {
        packStickData(&dst->input0x30.left_stick,  STICK_ZERO, STICK_ZERO);
        packStickData(&dst->input0x30.right_stick, STICK_ZERO, STICK_ZERO);

        // Orientation vertical
        //dst->input0x30.buttons.dpad_down   = src->input0x30.buttons.dpad_down;
        //dst->input0x30.buttons.dpad_up     = src->input0x30.buttons.dpad_up;
        //dst->input0x30.buttons.dpad_right  = src->input0x30.buttons.dpad_right;
        //dst->input0x30.buttons.dpad_left   = src->input0x30.buttons.dpad_left;

        dst->input0x30.buttons.dpad_down   = src->input0x30.buttons.dpad_left;
        dst->input0x30.buttons.dpad_up     = src->input0x30.buttons.dpad_right;
        dst->input0x30.buttons.dpad_right  = src->input0x30.buttons.dpad_down;
        dst->input0x30.buttons.dpad_left   = src->input0x30.buttons.dpad_up;

        dst->input0x30.buttons.A = src->input0x30.buttons.two;
        dst->input0x30.buttons.B = src->input0x30.buttons.one;

        dst->input0x30.buttons.R = src->input0x30.buttons.A;
        dst->input0x30.buttons.L = src->input0x30.buttons.B;

        dst->input0x30.buttons.minus   = src->input0x30.buttons.minus;
        dst->input0x30.buttons.plus    = src->input0x30.buttons.plus;
        
        dst->input0x30.buttons.home    = src->input0x30.buttons.home;
    }

    void WiimoteController::handleInputReport0x31(const WiiReportData *src, SwitchReportData *dst) {
        packStickData(&dst->input0x30.left_stick,  STICK_ZERO, STICK_ZERO);
        packStickData(&dst->input0x30.right_stick, STICK_ZERO, STICK_ZERO);

        dst->input0x30.buttons.dpad_down   = src->input0x31.buttons.dpad_left;
        dst->input0x30.buttons.dpad_up     = src->input0x31.buttons.dpad_right;
        dst->input0x30.buttons.dpad_right  = src->input0x31.buttons.dpad_down;
        dst->input0x30.buttons.dpad_left   = src->input0x31.buttons.dpad_up;

        dst->input0x30.buttons.A = src->input0x31.buttons.two;
        dst->input0x30.buttons.B = src->input0x31.buttons.one;

        dst->input0x30.buttons.R = src->input0x31.buttons.A;
        dst->input0x30.buttons.L = src->input0x31.buttons.B;

        dst->input0x30.buttons.minus   = src->input0x31.buttons.minus;
        dst->input0x30.buttons.plus    = src->input0x31.buttons.plus;
        
        dst->input0x30.buttons.home    = src->input0x31.buttons.home;

        // Todo: Accelerometer data
    }

}
