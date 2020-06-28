#include <cstring>
#include <cmath>
#include <stratosphere.hpp>

#include "xboxone.hpp"
#include "hdlsvirtualcontroller.hpp"

namespace controller {

    namespace {

        const constexpr uint8_t xboxone_joystick_nbits = 16;

    }

    XboxOneController::XboxOneController(const BluetoothAddress *address) 
    : BluetoothController(ControllerType_XboxOne, address) {
        //if (ams::hos::GetVersion() >= ams::hos::Version_7_0_0)
            //m_virtualController = std::make_unique<HdlsVirtualController>();
    }

    void XboxOneController::convertReportFormat(const HidReport *inReport, HidReport *outReport) {
        auto xboxData = reinterpret_cast<const XboxOneReportData *>(&inReport->data);
        auto switchData = reinterpret_cast<SwitchReportData *>(&outReport->data);

        outReport->type = 0x31;
        outReport->id = 0x30;

        switchData->report0x30.conn_info = 0x0;
        switchData->report0x30.battery = 0x8;

        switch(inReport->id) {
            case 0x01:
                handleInputReport0x01(xboxData, switchData);
                break;

            case 0x02:
                handleInputReport0x02(xboxData, switchData);
                break;

            default:
                break;
        }
    }

    void XboxOneController::mapStickValues(JoystickPosition *dst, const XboxOneStickData *src) {
        /*
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
        */
    }

    void XboxOneController::handleInputReport0x01(const XboxOneReportData *src, SwitchReportData *dst) {
        
        
        dst->report0x30.buttons.dpad_down   = (src->report0x01.buttons.dpad == XboxOneDPad_S)  ||
                                              (src->report0x01.buttons.dpad == XboxOneDPad_SE) ||
                                              (src->report0x01.buttons.dpad == XboxOneDPad_SW);
        dst->report0x30.buttons.dpad_up     = (src->report0x01.buttons.dpad == XboxOneDPad_N)  ||
                                              (src->report0x01.buttons.dpad == XboxOneDPad_NE) ||
                                              (src->report0x01.buttons.dpad == XboxOneDPad_NW);
        dst->report0x30.buttons.dpad_right  = (src->report0x01.buttons.dpad == XboxOneDPad_E)  ||
                                              (src->report0x01.buttons.dpad == XboxOneDPad_NE) ||
                                              (src->report0x01.buttons.dpad == XboxOneDPad_SE);
        dst->report0x30.buttons.dpad_left   = (src->report0x01.buttons.dpad == XboxOneDPad_W)  ||
                                              (src->report0x01.buttons.dpad == XboxOneDPad_NW) ||
                                              (src->report0x01.buttons.dpad == XboxOneDPad_SW);

        dst->report0x30.buttons.A = src->report0x01.buttons.B;
        dst->report0x30.buttons.B = src->report0x01.buttons.A;
        dst->report0x30.buttons.X = src->report0x01.buttons.Y;
        dst->report0x30.buttons.Y = src->report0x01.buttons.X;

        dst->report0x30.buttons.R  = src->report0x01.buttons.RB;
        dst->report0x30.buttons.ZR = src->report0x01.right_trigger > 0;
        dst->report0x30.buttons.L  = src->report0x01.buttons.LB;
        dst->report0x30.buttons.ZL = src->report0x01.left_trigger > 0;

        dst->report0x30.buttons.minus = src->report0x01.buttons.view;
        dst->report0x30.buttons.plus  = src->report0x01.buttons.menu;

        dst->report0x30.buttons.lstick_press = src->report0x01.buttons.lstick_press;
        dst->report0x30.buttons.rstick_press = src->report0x01.buttons.rstick_press;
    }

    void XboxOneController::handleInputReport0x02(const XboxOneReportData *src, SwitchReportData *dst) {
        dst->report0x30.buttons.home = src->report0x02.guide;
    }

}
