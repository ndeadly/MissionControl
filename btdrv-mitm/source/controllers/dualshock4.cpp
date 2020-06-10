#include <cstring>
#include <cmath>

#include "dualshock4.hpp"
#include "../btdrv_mitm_logging.hpp"

#include "hdlsvirtualcontroller.hpp"


namespace controller {

    namespace {

        const constexpr uint8_t dualshock4_joystick_nbits = 8;

    }

    Dualshock4Controller::Dualshock4Controller(const BluetoothAddress *address)
    : BluetoothController(ControllerType_Dualshock4, address) {
        m_virtualController = std::make_unique<HdlsVirtualController>();
    }

    Result Dualshock4Controller::initialize(void) {
        BTDRV_LOG_FMT("Dualshock4Controller::initialize");
        return BluetoothController::initialize();
    }

    void Dualshock4Controller::convertReportFormat(const HidReport *inReport, HidReport *outReport) {
        auto ds4Data = reinterpret_cast<const Dualshock4ReportData *>(&inReport->data);
        auto switchData = reinterpret_cast<SwitchReportData *>(&outReport->data);

        outReport->type = 0x31;
        outReport->id = 0x30;

        switchData->report0x30.conn_info = 0x0;
        switchData->report0x30.battery = 0x8;

        switch(inReport->id) {
            case 0x01:
                handleInputReport0x01(ds4Data, switchData);
                break;

            case 0x11:
                handleInputReport0x11(ds4Data, switchData);
                break;

            default:
                break;
        }
    }

    void Dualshock4Controller::mapStickValues(JoystickPosition *dst, const Dualshock4StickData *src) {
        /*
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
        */
    }

    void Dualshock4Controller::handleInputReport0x01(const Dualshock4ReportData *src, SwitchReportData *dst) {


        dst->report0x30.buttons.dpad_down   = (src->report0x01.buttons.dpad == Dualshock4DPad_S)  ||
                                              (src->report0x01.buttons.dpad == Dualshock4DPad_SE) ||
                                              (src->report0x01.buttons.dpad == Dualshock4DPad_SW);
        dst->report0x30.buttons.dpad_up     = (src->report0x01.buttons.dpad == Dualshock4DPad_N)  ||
                                              (src->report0x01.buttons.dpad == Dualshock4DPad_NE) ||
                                              (src->report0x01.buttons.dpad == Dualshock4DPad_NW);
        dst->report0x30.buttons.dpad_right  = (src->report0x01.buttons.dpad == Dualshock4DPad_E)  ||
                                              (src->report0x01.buttons.dpad == Dualshock4DPad_NE) ||
                                              (src->report0x01.buttons.dpad == Dualshock4DPad_SE);
        dst->report0x30.buttons.dpad_left   = (src->report0x01.buttons.dpad == Dualshock4DPad_W)  ||
                                              (src->report0x01.buttons.dpad == Dualshock4DPad_NW) ||
                                              (src->report0x01.buttons.dpad == Dualshock4DPad_SW);

        dst->report0x30.buttons.A = src->report0x01.buttons.circle;
        dst->report0x30.buttons.B = src->report0x01.buttons.cross;
        dst->report0x30.buttons.X = src->report0x01.buttons.triangle;
        dst->report0x30.buttons.Y = src->report0x01.buttons.square;

        dst->report0x30.buttons.R  = src->report0x01.buttons.R1;
        dst->report0x30.buttons.ZR = src->report0x01.buttons.R2;
        dst->report0x30.buttons.L  = src->report0x01.buttons.L1;
        dst->report0x30.buttons.ZL = src->report0x01.buttons.L2;

        dst->report0x30.buttons.minus = src->report0x01.buttons.share;
        dst->report0x30.buttons.plus  = src->report0x01.buttons.options;

        dst->report0x30.buttons.lstick_press = src->report0x01.buttons.L3;
        dst->report0x30.buttons.rstick_press = src->report0x01.buttons.R3;

        dst->report0x30.buttons.capture       = src->report0x01.buttons.tpad;
        dst->report0x30.buttons.home          = src->report0x01.buttons.ps;
    }

    void Dualshock4Controller::handleInputReport0x11(const Dualshock4ReportData *src, SwitchReportData *dst) {


        dst->report0x30.buttons.dpad_down   = (src->report0x11.buttons.dpad == Dualshock4DPad_S)  ||
                                              (src->report0x11.buttons.dpad == Dualshock4DPad_SE) ||
                                              (src->report0x11.buttons.dpad == Dualshock4DPad_SW);
        dst->report0x30.buttons.dpad_up     = (src->report0x11.buttons.dpad == Dualshock4DPad_N)  ||
                                              (src->report0x11.buttons.dpad == Dualshock4DPad_NE) ||
                                              (src->report0x11.buttons.dpad == Dualshock4DPad_NW);
        dst->report0x30.buttons.dpad_right  = (src->report0x11.buttons.dpad == Dualshock4DPad_E)  ||
                                              (src->report0x11.buttons.dpad == Dualshock4DPad_NE) ||
                                              (src->report0x11.buttons.dpad == Dualshock4DPad_SE);
        dst->report0x30.buttons.dpad_left   = (src->report0x11.buttons.dpad == Dualshock4DPad_W)  ||
                                              (src->report0x11.buttons.dpad == Dualshock4DPad_NW) ||
                                              (src->report0x11.buttons.dpad == Dualshock4DPad_SW);

        dst->report0x30.buttons.A = src->report0x11.buttons.circle;
        dst->report0x30.buttons.B = src->report0x11.buttons.cross;
        dst->report0x30.buttons.X = src->report0x11.buttons.triangle;
        dst->report0x30.buttons.Y = src->report0x11.buttons.square;

        dst->report0x30.buttons.R  = src->report0x11.buttons.R1;
        dst->report0x30.buttons.ZR = src->report0x11.buttons.R2;
        dst->report0x30.buttons.L  = src->report0x11.buttons.L1;
        dst->report0x30.buttons.ZL = src->report0x11.buttons.L2;

        dst->report0x30.buttons.minus = src->report0x11.buttons.share;
        dst->report0x30.buttons.plus  = src->report0x11.buttons.options;

        dst->report0x30.buttons.lstick_press = src->report0x11.buttons.L3;
        dst->report0x30.buttons.rstick_press = src->report0x11.buttons.R3;

        dst->report0x30.buttons.capture       = src->report0x11.buttons.tpad;
        dst->report0x30.buttons.home          = src->report0x11.buttons.ps;
    }

}
