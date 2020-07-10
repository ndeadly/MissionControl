#include "wiimote.hpp"
#include "switchcontroller.hpp"
#include <stratosphere.hpp>
#include "../bluetooth/bluetooth_hid_report.hpp"

#include "../btdrv_mitm_logging.hpp"

namespace controller {

    WiimoteController::WiimoteController(const BluetoothAddress *address)  
    : WiiController(ControllerType_Wiimote, address) {

    }

    Result WiimoteController::initialize(void) {
        R_TRY(WiiController::initialize());
        R_TRY(setReportMode(&m_address, 0x31));

        return 0;
    }

    void WiimoteController::convertReportFormat(const HidReport *inReport, HidReport *outReport) {
        auto wiiData = reinterpret_cast<const WiimoteReportData *>(&inReport->data);
        auto switchData = reinterpret_cast<SwitchReportData *>(&outReport->data);

        outReport->type = 0x31;
        outReport->id = 0x30;

        switchData->report0x30.conn_info = 0x0;
        switchData->report0x30.battery = 0x8;

        switch(inReport->id) {
            case 0x30:
                handleInputReport0x30(wiiData, switchData);
                break;

            case 0x31:
                handleInputReport0x31(wiiData, switchData);
                break;

            default:
                BTDRV_LOG_FMT("WIIMOTE: RECEIVED REPORT [0x%02x]", inReport->id);
                break;
        }
    }

    void WiimoteController::handleInputReport0x30(const WiimoteReportData *src, SwitchReportData *dst) {
        packStickData(&dst->report0x30.left_stick,  STICK_ZERO, STICK_ZERO);
        packStickData(&dst->report0x30.right_stick, STICK_ZERO, STICK_ZERO);

        // Orientation vertical
        //dst->report0x30.buttons.dpad_down   = src->report0x30.buttons.dpad_down;
        //dst->report0x30.buttons.dpad_up     = src->report0x30.buttons.dpad_up;
        //dst->report0x30.buttons.dpad_right  = src->report0x30.buttons.dpad_right;
        //dst->report0x30.buttons.dpad_left   = src->report0x30.buttons.dpad_left;

        dst->report0x30.buttons.dpad_down   = src->report0x30.buttons.dpad_left;
        dst->report0x30.buttons.dpad_up     = src->report0x30.buttons.dpad_right;
        dst->report0x30.buttons.dpad_right  = src->report0x30.buttons.dpad_down;
        dst->report0x30.buttons.dpad_left   = src->report0x30.buttons.dpad_up;

        dst->report0x30.buttons.A = src->report0x30.buttons.two;
        dst->report0x30.buttons.B = src->report0x30.buttons.one;

        dst->report0x30.buttons.R = src->report0x30.buttons.A;
        dst->report0x30.buttons.L = src->report0x30.buttons.B;

        dst->report0x30.buttons.minus   = src->report0x30.buttons.minus;
        dst->report0x30.buttons.plus    = src->report0x30.buttons.plus;
        
        dst->report0x30.buttons.home    = src->report0x30.buttons.home;
    }

    void WiimoteController::handleInputReport0x31(const WiimoteReportData *src, SwitchReportData *dst) {
        packStickData(&dst->report0x30.left_stick,  STICK_ZERO, STICK_ZERO);
        packStickData(&dst->report0x30.right_stick, STICK_ZERO, STICK_ZERO);

        dst->report0x30.buttons.dpad_down   = src->report0x31.buttons.dpad_left;
        dst->report0x30.buttons.dpad_up     = src->report0x31.buttons.dpad_right;
        dst->report0x30.buttons.dpad_right  = src->report0x31.buttons.dpad_down;
        dst->report0x30.buttons.dpad_left   = src->report0x31.buttons.dpad_up;

        dst->report0x30.buttons.A = src->report0x31.buttons.two;
        dst->report0x30.buttons.B = src->report0x31.buttons.one;

        dst->report0x30.buttons.R = src->report0x31.buttons.A;
        dst->report0x30.buttons.L = src->report0x31.buttons.B;

        dst->report0x30.buttons.minus   = src->report0x31.buttons.minus;
        dst->report0x30.buttons.plus    = src->report0x31.buttons.plus;
        
        dst->report0x30.buttons.home    = src->report0x31.buttons.home;

        // Todo: Accelerometer data
    }

}
