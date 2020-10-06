/*
 * Copyright (c) 2020 ndeadly
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
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

        const RGBColour player_led_colours[] = {
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
        RGBColour colour = player_led_colours[i];
        return this->SetLightbarColour(colour);
    }

    Result Dualshock4Controller::SetLightbarColour(RGBColour colour) {
        m_led_colour = colour;
        return this->UpdateControllerState();
    }

    void Dualshock4Controller::UpdateControllerState(const bluetooth::HidReport *report) {
        auto ds4_report = reinterpret_cast<const Dualshock4ReportData *>(&report->data);

        switch(ds4_report->id) {
            case 0x01:
                this->HandleInputReport0x01(ds4_report);
                break;
            case 0x11:
                this->HandleInputReport0x11(ds4_report);
                break;
            default:
                break;
        }
    }

    void Dualshock4Controller::HandleInputReport0x01(const Dualshock4ReportData *src) {       
        this->PackStickData(&m_left_stick,
            static_cast<uint16_t>(stick_scale_factor * src->input0x01.left_stick.x) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT8_MAX - src->input0x01.left_stick.y)) & 0xfff
        );
        this->PackStickData(&m_right_stick,
            static_cast<uint16_t>(stick_scale_factor * src->input0x01.right_stick.x) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT8_MAX - src->input0x01.right_stick.y)) & 0xfff
        );

        this->MapButtons(&src->input0x01.buttons);
    }

    void Dualshock4Controller::HandleInputReport0x11(const Dualshock4ReportData *src) {
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

        this->PackStickData(&m_left_stick,
            static_cast<uint16_t>(stick_scale_factor * src->input0x11.left_stick.x) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT8_MAX - src->input0x11.left_stick.y)) & 0xfff
        );
        this->PackStickData(&m_right_stick,
            static_cast<uint16_t>(stick_scale_factor * src->input0x11.right_stick.x) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT8_MAX - src->input0x11.right_stick.y)) & 0xfff
        );

        this->MapButtons(&src->input0x11.buttons);
    }

    void Dualshock4Controller::MapButtons(const Dualshock4ButtonData *buttons) {
        m_buttons.dpad_down   = (buttons->dpad == Dualshock4DPad_S)  ||
                                (buttons->dpad == Dualshock4DPad_SE) ||
                                (buttons->dpad == Dualshock4DPad_SW);
        m_buttons.dpad_up     = (buttons->dpad == Dualshock4DPad_N)  ||
                                (buttons->dpad == Dualshock4DPad_NE) ||
                                (buttons->dpad == Dualshock4DPad_NW);
        m_buttons.dpad_right  = (buttons->dpad == Dualshock4DPad_E)  ||
                                (buttons->dpad == Dualshock4DPad_NE) ||
                                (buttons->dpad == Dualshock4DPad_SE);
        m_buttons.dpad_left   = (buttons->dpad == Dualshock4DPad_W)  ||
                                (buttons->dpad == Dualshock4DPad_NW) ||
                                (buttons->dpad == Dualshock4DPad_SW);

        m_buttons.A = buttons->circle;
        m_buttons.B = buttons->cross;
        m_buttons.X = buttons->triangle;
        m_buttons.Y = buttons->square;

        m_buttons.R  = buttons->R1;
        m_buttons.ZR = buttons->R2;
        m_buttons.L  = buttons->L1;
        m_buttons.ZL = buttons->L2;

        m_buttons.minus = buttons->share;
        m_buttons.plus  = buttons->options;

        m_buttons.lstick_press = buttons->L3;
        m_buttons.rstick_press = buttons->R3;

        m_buttons.capture = buttons->tpad;
        m_buttons.home    = buttons->ps;
    }

    Result Dualshock4Controller::UpdateControllerState(void) {
        Dualshock4OutputReport0x11 report = {0xa2, 0x11, 0xc0, 0x20, 0xf3, 0x04, 0x00, 0x00, 0x00, m_led_colour.r, m_led_colour.g, m_led_colour.b};
        report.crc = crc32Calculate(report.data, sizeof(report.data));

        s_output_report.size = sizeof(report) - 1;
        std::memcpy(s_output_report.data, &report.data[1], s_output_report.size);

        return bluetooth::hid::report::SendHidReport(&m_address, &s_output_report);
    }

}
