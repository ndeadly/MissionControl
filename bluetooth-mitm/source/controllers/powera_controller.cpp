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
#include "powera_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    namespace {

        const constexpr float stick_scale_factor = float(UINT12_MAX) / UINT8_MAX;

    }

    void PowerAController::ConvertReportFormat(const bluetooth::HidReport *in_report, bluetooth::HidReport *out_report) {
        auto powera_report = reinterpret_cast<const PowerAReportData *>(&in_report->data);
        auto switch_report = reinterpret_cast<SwitchReportData *>(&out_report->data);

        switch(powera_report->id) {
            case 0x03:
                this->HandleInputReport0x03(powera_report, switch_report);
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

    void PowerAController::HandleInputReport0x03(const PowerAReportData *src, SwitchReportData *dst) {
        m_battery = src->input0x03.battery / 52 << 1;

        this->PackStickData(&dst->input0x30.left_stick,
            static_cast<uint16_t>(stick_scale_factor * src->input0x03.left_stick.x) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT8_MAX - src->input0x03.left_stick.y)) & 0xfff
        );
        this->PackStickData(&dst->input0x30.right_stick,
            static_cast<uint16_t>(stick_scale_factor * src->input0x03.right_stick.x) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT8_MAX - src->input0x03.right_stick.y)) & 0xfff
        );
        
        dst->input0x30.buttons.dpad_down   = (src->input0x03.buttons.dpad == PowerADPad_S)  ||
                                             (src->input0x03.buttons.dpad == PowerADPad_SE) ||
                                             (src->input0x03.buttons.dpad == PowerADPad_SW);
        dst->input0x30.buttons.dpad_up     = (src->input0x03.buttons.dpad == PowerADPad_N)  ||
                                             (src->input0x03.buttons.dpad == PowerADPad_NE) ||
                                             (src->input0x03.buttons.dpad == PowerADPad_NW);
        dst->input0x30.buttons.dpad_right  = (src->input0x03.buttons.dpad == PowerADPad_E)  ||
                                             (src->input0x03.buttons.dpad == PowerADPad_NE) ||
                                             (src->input0x03.buttons.dpad == PowerADPad_SE);
        dst->input0x30.buttons.dpad_left   = (src->input0x03.buttons.dpad == PowerADPad_W)  ||
                                             (src->input0x03.buttons.dpad == PowerADPad_NW) ||
                                             (src->input0x03.buttons.dpad == PowerADPad_SW);

        dst->input0x30.buttons.A = src->input0x03.buttons.B;
        dst->input0x30.buttons.B = src->input0x03.buttons.A;
        dst->input0x30.buttons.X = src->input0x03.buttons.Y;
        dst->input0x30.buttons.Y = src->input0x03.buttons.X;

        dst->input0x30.buttons.R  = src->input0x03.buttons.R1;
        dst->input0x30.buttons.ZR = src->input0x03.R2 > 0;
        dst->input0x30.buttons.L  = src->input0x03.buttons.L1;
        dst->input0x30.buttons.ZL = src->input0x03.L2 > 0; 

        dst->input0x30.buttons.minus = src->input0x03.buttons.select;
        dst->input0x30.buttons.plus  = src->input0x03.buttons.start;

        dst->input0x30.buttons.lstick_press = src->input0x03.buttons.L3;
        dst->input0x30.buttons.rstick_press = src->input0x03.buttons.R3;    

        dst->input0x30.buttons.capture  = 0;

        // Home combo
        dst->input0x30.buttons.home = dst->input0x30.buttons.minus && dst->input0x30.buttons.dpad_down;
        if (dst->input0x30.buttons.home) {
            dst->input0x30.buttons.minus = 0;
            dst->input0x30.buttons.dpad_down = 0;
        }
    }

}
