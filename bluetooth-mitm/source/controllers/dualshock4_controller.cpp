/*
 * Copyright (C) 2020 ndeadly
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "dualshock4_controller.hpp"
#include <switch.h>
#include <stratosphere.hpp>
#include <cstring>

namespace ams::controller {

    namespace {

        const constexpr float stick_scale_factor = float(UINT12_MAX) / UINT8_MAX;

        const Dualshock4LedColour player_led_colours[] = {
            {0x00, 0x00, 0x3f}, // blue
            {0x3f, 0x00, 0x00}, // red
            {0x00, 0x3f, 0x00}, // green
            {0x3f, 0x00, 0x3f}  // pink
        };

    }

    Result Dualshock4Controller::Initialize(void) {
        R_TRY(EmulatedSwitchController::Initialize());
        R_TRY(this->UpdateControllerState());

        return ams::ResultSuccess();
    }

    Result Dualshock4Controller::SetPlayerLed(uint8_t led_mask) {
        uint8_t i = 0;
        while (led_mask >>= 1) { ++i; }
        Dualshock4LedColour colour = player_led_colours[i];
        return this->SetLightbarColour(colour);
    }

    Result Dualshock4Controller::SetLightbarColour(Dualshock4LedColour colour) {
        m_led_colour = colour;
        return this->UpdateControllerState();
    }

    void Dualshock4Controller::ConvertReportFormat(const bluetooth::HidReport *in_report, bluetooth::HidReport *out_report) {
        auto ds4_report = reinterpret_cast<const Dualshock4ReportData *>(&in_report->data);
        auto switch_report = reinterpret_cast<SwitchReportData *>(&out_report->data);

        switch(ds4_report->id) {
            case 0x01:
                this->HandleInputReport0x01(ds4_report, switch_report);
                break;
            case 0x11:
                this->HandleInputReport0x11(ds4_report, switch_report);
                break;
            default:
                break;
        }

        out_report->size = sizeof(SwitchInputReport0x30) + 1;
        switch_report->id = 0x30;
        switch_report->input0x30.conn_info = 0x0;
        switch_report->input0x30.battery = m_battery | m_charging;
        std::memset(switch_report->input0x30.motion, 0, sizeof(switch_report->input0x30.motion));
        switch_report->input0x30.timer = os::ConvertToTimeSpan(os::GetSystemTick()).GetMilliSeconds() & 0xff;
    }

    void Dualshock4Controller::HandleInputReport0x01(const Dualshock4ReportData *src, SwitchReportData *dst) {       
        this->PackStickData(&dst->input0x30.left_stick,
            static_cast<uint16_t>(stick_scale_factor * src->input0x01.left_stick.x) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT8_MAX - src->input0x01.left_stick.y)) & 0xfff
        );
        this->PackStickData(&dst->input0x30.right_stick,
            static_cast<uint16_t>(stick_scale_factor * src->input0x01.right_stick.x) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT8_MAX - src->input0x01.right_stick.y)) & 0xfff
        );

        this->MapButtons(&src->input0x01.buttons, dst);
    }

    void Dualshock4Controller::HandleInputReport0x11(const Dualshock4ReportData *src, SwitchReportData *dst) {
        if (!src->input0x11.usb || src->input0x11.battery_level > 10)
            m_charging = false;
        else
            m_charging = true;

        uint8_t battery_level = src->input0x11.battery_level;
        if (!src->input0x11.usb)
            battery_level++;
        if (battery_level > 10)
            battery_level = 10;

        m_battery = static_cast<uint8_t>(8 * (battery_level + 1) / 10) & 0x0e;

        this->PackStickData(&dst->input0x30.left_stick,
            static_cast<uint16_t>(stick_scale_factor * src->input0x11.left_stick.x) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT8_MAX - src->input0x11.left_stick.y)) & 0xfff
        );
        this->PackStickData(&dst->input0x30.right_stick,
            static_cast<uint16_t>(stick_scale_factor * src->input0x11.right_stick.x) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT8_MAX - src->input0x11.right_stick.y)) & 0xfff
        );

        this->MapButtons(&src->input0x11.buttons, dst);
    }

    void Dualshock4Controller::MapButtons(const Dualshock4ButtonData *buttons, SwitchReportData *dst) {
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

    Result Dualshock4Controller::UpdateControllerState(void) {
        Dualshock4OutputReport0x11 report = {0xa2, 0x11, 0xc0, 0x20, 0xf3, 0x04, 0x00, 0x00, 0x00, m_led_colour.r, m_led_colour.g, m_led_colour.b};
        report.crc = crc32Calculate(report.data, sizeof(report.data));

        s_output_report.size = sizeof(report) - 1;
        std::memcpy(s_output_report.data, &report.data[1], s_output_report.size);

        R_TRY(bluetooth::hid::report::SendHidReport(&m_address, &s_output_report));

        return ams::ResultSuccess();
    }

}
