#include "dualshock4.hpp"

#include <cstring>
#include <switch.h>
#include <stratosphere.hpp>

#include "../btdrv_mitm_logging.hpp"

namespace controller {

    namespace {

        const constexpr float scale_factor = float(UINT12_MAX) / UINT8_MAX;

    }

    Dualshock4Controller::Dualshock4Controller(const BluetoothAddress *address)
    : BluetoothController(ControllerType_Dualshock4, address) {

    }

    Result Dualshock4Controller::initialize(void) {
        R_TRY(BluetoothController::initialize());
        
        //uint8_t r = 0xff;
        //uint8_t g = 0x00;
        //uint8_t b = 0x00;
        //Dualshock4OutputReport0x11 report = {0xa2, 0x11, 0xc0, 0x20, 0xf3, 0x04, 0x00, 0x00, 0x00, r, g, b};    

        uint8_t rgb[3];
        randomGet(rgb, sizeof(rgb));
        Dualshock4OutputReport0x11 report = {0xa2, 0x11, 0xc0, 0x20, 0xf3, 0x04, 0x00, 0x00, 0x00, rgb[0], rgb[1], rgb[2]};
        report.crc = crc32Calculate(report.data, sizeof(report.data));
        
        BluetoothHidData hidData = {};
        hidData.length = sizeof(report) - 1;
        std::memcpy(&hidData.data, &report.data[1], hidData.length);

        R_TRY(btdrvSetHidReport(&m_address, HidReportType_OutputReport, &hidData));

        return 0;
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
                BTDRV_LOG_FMT("DS4: RECEIVED REPORT [%02d]", inReport->id);
                break;
        }
    }

    void Dualshock4Controller::handleInputReport0x01(const Dualshock4ReportData *src, SwitchReportData *dst) {
        packStickData(&dst->report0x30.left_stick,
            static_cast<uint16_t>(scale_factor * src->report0x01.left_stick.x) & 0xfff,
            static_cast<uint16_t>(scale_factor * (UINT8_MAX - src->report0x01.left_stick.y)) & 0xfff
        );
        packStickData(&dst->report0x30.right_stick,
            static_cast<uint16_t>(scale_factor * src->report0x01.right_stick.x) & 0xfff,
            static_cast<uint16_t>(scale_factor * (UINT8_MAX - src->report0x01.right_stick.y)) & 0xfff
        );

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
        packStickData(&dst->report0x30.left_stick,
            static_cast<uint16_t>(scale_factor * src->report0x11.left_stick.x) & 0xfff,
            static_cast<uint16_t>(scale_factor * (UINT8_MAX - src->report0x11.left_stick.y)) & 0xfff
        );
        packStickData(&dst->report0x30.right_stick,
            static_cast<uint16_t>(scale_factor * src->report0x11.right_stick.x) & 0xfff,
            static_cast<uint16_t>(scale_factor * (UINT8_MAX - src->report0x11.right_stick.y)) & 0xfff
        );

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
