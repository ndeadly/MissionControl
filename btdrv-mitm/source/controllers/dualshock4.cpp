#include "dualshock4.hpp"

#include <cstring>
#include <switch.h>
#include <stratosphere.hpp>

#include "../btdrv_mitm_logging.hpp"

namespace ams::controller {

    namespace {

        const constexpr float stickScaleFactor = float(UINT12_MAX) / UINT8_MAX;

        const Dualshock4LedColour playerLedColours[] = {
            {0x00, 0x00, 0x7f}, // blue
            {0x7f, 0x00, 0x00}, // red
            {0x00, 0x7f, 0x00}, // green
            {0x7f, 0x00, 0x7f}  // pink
        };

    }

    Dualshock4Controller::Dualshock4Controller(const bluetooth::Address *address)
    : BluetoothController(ControllerType_Dualshock4, address) {

    }

    Result Dualshock4Controller::initialize(void) {
        R_TRY(BluetoothController::initialize());

        u8 ind;
        randomGet(&ind, 1);
        auto colour = playerLedColours[ind >> 6];

        Dualshock4OutputReport0x11 report = {0xa2, 0x11, 0xc0, 0x20, 0xf3, 0x04, 0x00, 0x00, 0x00, colour.r, colour.g, colour.b};
        report.crc = crc32Calculate(report.data, sizeof(report.data));

        bluetooth::HidReport hidReport = {};
        hidReport.size = sizeof(report) - 1;
        std::memcpy(&hidReport.data, &report.data[1], hidReport.size);

        R_TRY(btdrvSetHidReport(&m_address, HidReportType_OutputReport, &hidReport));

        return 0;
    }

    void Dualshock4Controller::convertReportFormat(const bluetooth::HidReport *inReport, bluetooth::HidReport *outReport) {
        auto ds4Report = reinterpret_cast<const Dualshock4ReportData *>(&inReport->data);
        auto switchReport = reinterpret_cast<SwitchReportData *>(&outReport->data);

        outReport->size = 0x31;
        switchReport->id = 0x30;
        switchReport->input0x30.conn_info = 0x0;
        switchReport->input0x30.battery = 0x8;

        switch(ds4Report->id) {
            case 0x01:
                this->handleInputReport0x01(ds4Report, switchReport);
                break;

            case 0x11:
                this->handleInputReport0x11(ds4Report, switchReport);
                break;

            default:
                BTDRV_LOG_FMT("DS4: RECEIVED REPORT [0x%02x]", ds4Report->id);
                break;
        }
    }

    void Dualshock4Controller::handleInputReport0x01(const Dualshock4ReportData *src, SwitchReportData *dst) {
        packStickData(&dst->input0x30.left_stick,
            static_cast<uint16_t>(stickScaleFactor * src->input0x01.left_stick.x) & 0xfff,
            static_cast<uint16_t>(stickScaleFactor * (UINT8_MAX - src->input0x01.left_stick.y)) & 0xfff
        );
        packStickData(&dst->input0x30.right_stick,
            static_cast<uint16_t>(stickScaleFactor * src->input0x01.right_stick.x) & 0xfff,
            static_cast<uint16_t>(stickScaleFactor * (UINT8_MAX - src->input0x01.right_stick.y)) & 0xfff
        );

        dst->input0x30.buttons.dpad_down   = (src->input0x01.buttons.dpad == Dualshock4DPad_S)  ||
                                             (src->input0x01.buttons.dpad == Dualshock4DPad_SE) ||
                                             (src->input0x01.buttons.dpad == Dualshock4DPad_SW);
        dst->input0x30.buttons.dpad_up     = (src->input0x01.buttons.dpad == Dualshock4DPad_N)  ||
                                             (src->input0x01.buttons.dpad == Dualshock4DPad_NE) ||
                                             (src->input0x01.buttons.dpad == Dualshock4DPad_NW);
        dst->input0x30.buttons.dpad_right  = (src->input0x01.buttons.dpad == Dualshock4DPad_E)  ||
                                             (src->input0x01.buttons.dpad == Dualshock4DPad_NE) ||
                                             (src->input0x01.buttons.dpad == Dualshock4DPad_SE);
        dst->input0x30.buttons.dpad_left   = (src->input0x01.buttons.dpad == Dualshock4DPad_W)  ||
                                             (src->input0x01.buttons.dpad == Dualshock4DPad_NW) ||
                                             (src->input0x01.buttons.dpad == Dualshock4DPad_SW);

        dst->input0x30.buttons.A = src->input0x01.buttons.circle;
        dst->input0x30.buttons.B = src->input0x01.buttons.cross;
        dst->input0x30.buttons.X = src->input0x01.buttons.triangle;
        dst->input0x30.buttons.Y = src->input0x01.buttons.square;

        dst->input0x30.buttons.R  = src->input0x01.buttons.R1;
        dst->input0x30.buttons.ZR = src->input0x01.buttons.R2;
        dst->input0x30.buttons.L  = src->input0x01.buttons.L1;
        dst->input0x30.buttons.ZL = src->input0x01.buttons.L2;

        dst->input0x30.buttons.minus = src->input0x01.buttons.share;
        dst->input0x30.buttons.plus  = src->input0x01.buttons.options;

        dst->input0x30.buttons.lstick_press = src->input0x01.buttons.L3;
        dst->input0x30.buttons.rstick_press = src->input0x01.buttons.R3;

        dst->input0x30.buttons.capture = src->input0x01.buttons.tpad;
        dst->input0x30.buttons.home    = src->input0x01.buttons.ps;
    }

    void Dualshock4Controller::handleInputReport0x11(const Dualshock4ReportData *src, SwitchReportData *dst) {
        //dst->input0x30.battery = (((src->report0x11.battery / 64) + 1) << 1) & 0xf ;

        packStickData(&dst->input0x30.left_stick,
            static_cast<uint16_t>(stickScaleFactor * src->input0x11.left_stick.x) & 0xfff,
            static_cast<uint16_t>(stickScaleFactor * (UINT8_MAX - src->input0x11.left_stick.y)) & 0xfff
        );
        packStickData(&dst->input0x30.right_stick,
            static_cast<uint16_t>(stickScaleFactor * src->input0x11.right_stick.x) & 0xfff,
            static_cast<uint16_t>(stickScaleFactor * (UINT8_MAX - src->input0x11.right_stick.y)) & 0xfff
        );

        dst->input0x30.buttons.dpad_down   = (src->input0x11.buttons.dpad == Dualshock4DPad_S)  ||
                                             (src->input0x11.buttons.dpad == Dualshock4DPad_SE) ||
                                             (src->input0x11.buttons.dpad == Dualshock4DPad_SW);
        dst->input0x30.buttons.dpad_up     = (src->input0x11.buttons.dpad == Dualshock4DPad_N)  ||
                                             (src->input0x11.buttons.dpad == Dualshock4DPad_NE) ||
                                             (src->input0x11.buttons.dpad == Dualshock4DPad_NW);
        dst->input0x30.buttons.dpad_right  = (src->input0x11.buttons.dpad == Dualshock4DPad_E)  ||
                                             (src->input0x11.buttons.dpad == Dualshock4DPad_NE) ||
                                             (src->input0x11.buttons.dpad == Dualshock4DPad_SE);
        dst->input0x30.buttons.dpad_left   = (src->input0x11.buttons.dpad == Dualshock4DPad_W)  ||
                                             (src->input0x11.buttons.dpad == Dualshock4DPad_NW) ||
                                             (src->input0x11.buttons.dpad == Dualshock4DPad_SW);

        dst->input0x30.buttons.A = src->input0x11.buttons.circle;
        dst->input0x30.buttons.B = src->input0x11.buttons.cross;
        dst->input0x30.buttons.X = src->input0x11.buttons.triangle;
        dst->input0x30.buttons.Y = src->input0x11.buttons.square;

        dst->input0x30.buttons.R  = src->input0x11.buttons.R1;
        dst->input0x30.buttons.ZR = src->input0x11.buttons.R2;
        dst->input0x30.buttons.L  = src->input0x11.buttons.L1;
        dst->input0x30.buttons.ZL = src->input0x11.buttons.L2;

        dst->input0x30.buttons.minus = src->input0x11.buttons.share;
        dst->input0x30.buttons.plus  = src->input0x11.buttons.options;

        dst->input0x30.buttons.lstick_press = src->input0x11.buttons.L3;
        dst->input0x30.buttons.rstick_press = src->input0x11.buttons.R3;

        dst->input0x30.buttons.capture = src->input0x11.buttons.tpad;
        dst->input0x30.buttons.home    = src->input0x11.buttons.ps;
    }

}
