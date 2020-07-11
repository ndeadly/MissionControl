#include "xboxone.hpp"

#include <cstring>
#include <stratosphere.hpp>

#include "../btdrv_mitm_logging.hpp"

namespace ams::controller {

    namespace {

        const constexpr float stickScaleFactor = float(UINT12_MAX) / UINT16_MAX;

    }

    XboxOneController::XboxOneController(const bluetooth::Address *address) 
    : FakeSwitchController(ControllerType_XboxOne, address) {

    }

    void XboxOneController::convertReportFormat(const bluetooth::HidReport *inReport, bluetooth::HidReport *outReport) {
        auto xboxReport = reinterpret_cast<const XboxOneReportData *>(&inReport->data);
        auto switchReport = reinterpret_cast<SwitchReportData *>(&outReport->data);

        outReport->size = 0x31;
        switchReport->id = 0x30;
        switchReport->input0x30.conn_info = 0x0;
        switchReport->input0x30.battery = 0x8;

        switch(xboxReport->id) {
            case 0x01:
                this->handleInputReport0x01(xboxReport, switchReport);
                break;

            case 0x02:
                this->handleInputReport0x02(xboxReport, switchReport);
                break;

            case 0x04:
                this->handleInputReport0x04(xboxReport, switchReport);
                break;

            default:
                BTDRV_LOG_FMT("XBONE: RECEIVED REPORT [0x%02x]", xboxReport->id);
                break;
        }
    }

    void XboxOneController::handleInputReport0x01(const XboxOneReportData *src, SwitchReportData *dst) {
        packStickData(&dst->input0x30.left_stick,
            static_cast<uint16_t>(stickScaleFactor * src->input0x01.left_stick.x) & 0xfff,
            static_cast<uint16_t>(stickScaleFactor * (UINT16_MAX - src->input0x01.left_stick.y)) & 0xfff
        );
        packStickData(&dst->input0x30.right_stick,
            static_cast<uint16_t>(stickScaleFactor * src->input0x01.right_stick.x) & 0xfff,
            static_cast<uint16_t>(stickScaleFactor * (UINT16_MAX - src->input0x01.right_stick.y)) & 0xfff
        );

        dst->input0x30.buttons.dpad_down   = (src->input0x01.buttons.dpad == XboxOneDPad_S)  ||
                                             (src->input0x01.buttons.dpad == XboxOneDPad_SE) ||
                                             (src->input0x01.buttons.dpad == XboxOneDPad_SW);
        dst->input0x30.buttons.dpad_up     = (src->input0x01.buttons.dpad == XboxOneDPad_N)  ||
                                             (src->input0x01.buttons.dpad == XboxOneDPad_NE) ||
                                             (src->input0x01.buttons.dpad == XboxOneDPad_NW);
        dst->input0x30.buttons.dpad_right  = (src->input0x01.buttons.dpad == XboxOneDPad_E)  ||
                                             (src->input0x01.buttons.dpad == XboxOneDPad_NE) ||
                                             (src->input0x01.buttons.dpad == XboxOneDPad_SE);
        dst->input0x30.buttons.dpad_left   = (src->input0x01.buttons.dpad == XboxOneDPad_W)  ||
                                             (src->input0x01.buttons.dpad == XboxOneDPad_NW) ||
                                             (src->input0x01.buttons.dpad == XboxOneDPad_SW);

        dst->input0x30.buttons.A = src->input0x01.buttons.B;
        dst->input0x30.buttons.B = src->input0x01.buttons.A;
        dst->input0x30.buttons.X = src->input0x01.buttons.Y;
        dst->input0x30.buttons.Y = src->input0x01.buttons.X;

        dst->input0x30.buttons.R  = src->input0x01.buttons.RB;
        dst->input0x30.buttons.ZR = src->input0x01.right_trigger > 0;
        dst->input0x30.buttons.L  = src->input0x01.buttons.LB;
        dst->input0x30.buttons.ZL = src->input0x01.left_trigger > 0;

        dst->input0x30.buttons.minus = src->input0x01.buttons.view;
        dst->input0x30.buttons.plus  = src->input0x01.buttons.menu;

        dst->input0x30.buttons.lstick_press = src->input0x01.buttons.lstick_press;
        dst->input0x30.buttons.rstick_press = src->input0x01.buttons.rstick_press;

        dst->input0x30.buttons.capture = 0;
    }

    void XboxOneController::handleInputReport0x02(const XboxOneReportData *src, SwitchReportData *dst) {
        packStickData(&dst->input0x30.left_stick, STICK_ZERO, STICK_ZERO);
        packStickData(&dst->input0x30.right_stick, STICK_ZERO, STICK_ZERO);
        std::memset(&dst->input0x30.buttons, 0, sizeof(SwitchButtonData));

        dst->input0x30.buttons.home = src->input0x02.guide;
    }

    void XboxOneController::handleInputReport0x04(const XboxOneReportData *src, SwitchReportData *dst) {
        BTDRV_LOG_FMT("Xbox One battery flags: online: %d, mode: %d, capacity: %d", 
            src->input0x04.online,
            src->input0x04.mode,
            src->input0x04.capacity
        );

        packStickData(&dst->input0x30.left_stick, STICK_ZERO, STICK_ZERO);
        packStickData(&dst->input0x30.right_stick, STICK_ZERO, STICK_ZERO);
        std::memset(&dst->input0x30.buttons, 0, sizeof(SwitchButtonData));

    }

}
