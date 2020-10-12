/*
 * Copyright (c) 2020-2021 ndeadly
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
#include "xbox_one_controller.hpp"
#include <stratosphere.hpp>
#include <cstring>

namespace ams::controller {

    namespace {
        
        constexpr float stick_scale_factor = float(UINT12_MAX) / UINT16_MAX;

    }

    }

    Result XboxOneController::SetVibration(const SwitchRumbleData *left, const SwitchRumbleData *right) {
        auto report = reinterpret_cast<XboxOneReportData *>(s_output_report.data);
        s_output_report.size = sizeof(XboxOneOutputReport0x03) + 1;
        report->id = 0x03;
        report->output0x03.enable                = 0x3;
        report->output0x03.magnitude_strong      = static_cast<uint8_t>(100 * (float(left->low_band_amp) / UINT8_MAX));
        report->output0x03.magnitude_weak        = static_cast<uint8_t>(100 * (float(right->high_band_amp) / UINT8_MAX));
        report->output0x03.pulse_sustain_10ms    = 1;
        report->output0x03.pulse_release_10ms    = 0;
        report->output0x03.loop_count            = 0;

        return bluetooth::hid::report::SendHidReport(&m_address, &s_output_report);
    }

    void XboxOneController::UpdateControllerState(const bluetooth::HidReport *report) {
        auto xbox_report = reinterpret_cast<const XboxOneReportData *>(&report->data);

        switch(xbox_report->id) {
            case 0x01:
                this->HandleInputReport0x01(xbox_report);
                break;
            case 0x04:
                this->HandleInputReport0x04(xbox_report);
                break;
            default:
                break;
        }
    }

    void XboxOneController::HandleInputReport0x01(const XboxOneReportData *src) {
        m_left_stick = this->PackStickData(
            static_cast<uint16_t>(stick_scale_factor * src->input0x01.left_stick.x) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT16_MAX - src->input0x01.left_stick.y)) & 0xfff
        );
        m_right_stick = this->PackStickData(
            static_cast<uint16_t>(stick_scale_factor * src->input0x01.right_stick.x) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT16_MAX - src->input0x01.right_stick.y)) & 0xfff
        );

        m_buttons.dpad_down   = (src->input0x01.buttons.dpad == XboxOneDPad_S)  ||
                                (src->input0x01.buttons.dpad == XboxOneDPad_SE) ||
                                (src->input0x01.buttons.dpad == XboxOneDPad_SW);
        m_buttons.dpad_up     = (src->input0x01.buttons.dpad == XboxOneDPad_N)  ||
                                (src->input0x01.buttons.dpad == XboxOneDPad_NE) ||
                                (src->input0x01.buttons.dpad == XboxOneDPad_NW);
        m_buttons.dpad_right  = (src->input0x01.buttons.dpad == XboxOneDPad_E)  ||
                                (src->input0x01.buttons.dpad == XboxOneDPad_NE) ||
                                (src->input0x01.buttons.dpad == XboxOneDPad_SE);
        m_buttons.dpad_left   = (src->input0x01.buttons.dpad == XboxOneDPad_W)  ||
                                (src->input0x01.buttons.dpad == XboxOneDPad_NW) ||
                                (src->input0x01.buttons.dpad == XboxOneDPad_SW);

        m_buttons.A = src->input0x01.buttons.B;
        m_buttons.B = src->input0x01.buttons.A;
        m_buttons.X = src->input0x01.buttons.Y;
        m_buttons.Y = src->input0x01.buttons.X;

        m_buttons.R  = src->input0x01.buttons.RB;
        m_buttons.ZR = src->input0x01.right_trigger > 0;
        m_buttons.L  = src->input0x01.buttons.LB;
        m_buttons.ZL = src->input0x01.left_trigger > 0;

        m_buttons.minus = src->input0x01.buttons.view;
        m_buttons.plus  = src->input0x01.buttons.menu;

        m_buttons.lstick_press = src->input0x01.buttons.lstick_press;
        m_buttons.rstick_press = src->input0x01.buttons.rstick_press;

        m_buttons.home     = src->input0x01.buttons.guide;
    }

    void XboxOneController::HandleInputReport0x04(const XboxOneReportData *src) {
        m_battery = src->input0x04.capacity << 1;
        m_charging = src->input0x04.charging;
    }

}
