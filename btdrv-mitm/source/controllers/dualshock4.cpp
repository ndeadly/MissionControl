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

    Result Dualshock4Controller::initialize(void) {
        R_TRY(EmulatedSwitchController::initialize());
        R_TRY(this->updateControllerState());

        return ams::ResultSuccess();
    }

    Result Dualshock4Controller::setPlayerLed(u8 led_mask) {
        u8 i = 0;
        while (led_mask >>= 1) { ++i; }
        Dualshock4LedColour colour = playerLedColours[i];

        Dualshock4OutputReport0x11 raw = {0xa2, 0x11, 0xc0, 0x20, 0xf3, 0x04, 0x00, 0x00, 0x00, colour.r, colour.g, colour.b};
        raw.crc = crc32Calculate(raw.data, sizeof(raw.data));

        m_outputReport.size = sizeof(raw) - 1;
        std::memcpy(&m_outputReport.data, &raw.data[1], m_outputReport.size);

        return ams::ResultSuccess();
    }

    Result Dualshock4Controller::setLightbarColour(Dualshock4LedColour colour) {
        m_ledColour = colour;
        return this->updateControllerState();
    }

    void Dualshock4Controller::convertReportFormat(const bluetooth::HidReport *inReport, bluetooth::HidReport *outReport) {
        auto ds4Report = reinterpret_cast<const Dualshock4ReportData *>(&inReport->data);
        auto switchReport = reinterpret_cast<SwitchReportData *>(&outReport->data);

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

        outReport->size = sizeof(SwitchInputReport0x30) + 1;
        switchReport->id = 0x30;
        switchReport->input0x30.conn_info = 0x0;
        switchReport->input0x30.battery = m_battery | m_charging;
        switchReport->input0x30.timer = os::ConvertToTimeSpan(os::GetSystemTick()).GetMilliSeconds() & 0xff;
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

        this->mapButtons(&src->input0x01.buttons, dst);
    }

    void Dualshock4Controller::handleInputReport0x11(const Dualshock4ReportData *src, SwitchReportData *dst) {
        if (!src->input0x11.usb || src->input0x11.battery_level > 10)
            m_charging = false;
        else
            m_charging = true;

        m_battery = src->input0x11.battery_level & 0xe;

        packStickData(&dst->input0x30.left_stick,
            static_cast<uint16_t>(stickScaleFactor * src->input0x11.left_stick.x) & 0xfff,
            static_cast<uint16_t>(stickScaleFactor * (UINT8_MAX - src->input0x11.left_stick.y)) & 0xfff
        );
        packStickData(&dst->input0x30.right_stick,
            static_cast<uint16_t>(stickScaleFactor * src->input0x11.right_stick.x) & 0xfff,
            static_cast<uint16_t>(stickScaleFactor * (UINT8_MAX - src->input0x11.right_stick.y)) & 0xfff
        );

        this->mapButtons(&src->input0x11.buttons, dst);
    }

    void Dualshock4Controller::mapButtons(const Dualshock4ButtonData *buttons, SwitchReportData *dst) {
        dst->input0x30.buttons.dpad_down   = (buttons->dpad == Dualshock4DPad_S)  ||
                                             (buttons->dpad == Dualshock4DPad_SE) ||
                                             (buttons->dpad == Dualshock4DPad_SW);
        dst->input0x30.buttons.dpad_up     = (buttons->dpad == Dualshock4DPad_N)  ||
                                             (buttons->dpad == Dualshock4DPad_NE) ||
                                             (buttons->dpad == Dualshock4DPad_NW);
        dst->input0x30.buttons.dpad_right  = (buttons->dpad == Dualshock4DPad_E)  ||
                                             (buttons->dpad == Dualshock4DPad_NE) ||
                                             (buttons->dpad == Dualshock4DPad_SE);
        dst->input0x30.buttons.dpad_left   = (buttons->dpad == Dualshock4DPad_W)  ||
                                             (buttons->dpad == Dualshock4DPad_NW) ||
                                             (buttons->dpad == Dualshock4DPad_SW);

        dst->input0x30.buttons.A = buttons->circle;
        dst->input0x30.buttons.B = buttons->cross;
        dst->input0x30.buttons.X = buttons->triangle;
        dst->input0x30.buttons.Y = buttons->square;

        dst->input0x30.buttons.R  = buttons->R1;
        dst->input0x30.buttons.ZR = buttons->R2;
        dst->input0x30.buttons.L  = buttons->L1;
        dst->input0x30.buttons.ZL = buttons->L2;

        dst->input0x30.buttons.minus = buttons->share;
        dst->input0x30.buttons.plus  = buttons->options;

        dst->input0x30.buttons.lstick_press = buttons->L3;
        dst->input0x30.buttons.rstick_press = buttons->R3;

        dst->input0x30.buttons.capture = buttons->tpad;
        dst->input0x30.buttons.home    = buttons->ps;
    }

    Result Dualshock4Controller::updateControllerState(void) {
        Dualshock4OutputReport0x11 report = {0xa2, 0x11, 0xc0, 0x20, 0xf3, 0x04, 0x00, 0x00, 0x00, m_ledColour.r, m_ledColour.g, m_ledColour.b};
        report.crc = crc32Calculate(report.data, sizeof(report.data));

        bluetooth::HidReport hidReport = {};
        hidReport.size = sizeof(report) - 1;
        std::memcpy(&hidReport.data, &report.data[1], hidReport.size);

        R_TRY(btdrvWriteHidData(&m_address, &hidReport));

        return ams::ResultSuccess();
    }

}
