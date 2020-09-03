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
#include "ouya_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    namespace {

        constexpr float stick_scale_factor = float(UINT12_MAX) / UINT16_MAX;

    }

    void OuyaController::ConvertReportFormat(const bluetooth::HidReport *in_report, bluetooth::HidReport *out_report) {
        auto ouya_report = reinterpret_cast<const OuyaReportData *>(&in_report->data);
        auto switch_report = reinterpret_cast<SwitchReportData *>(&out_report->data);

        switch(ouya_report->id) {
            case 0x03:
                this->HandleInputReport0x03(ouya_report, switch_report);
                break;
            case 0x07:
                this->HandleInputReport0x07(ouya_report, switch_report);
                break;
            default:
                break;
        }

        out_report->size = sizeof(SwitchInputReport0x30) + 1;
        switch_report->id = 0x30;
        switch_report->input0x30.conn_info = 0x0;
        switch_report->input0x30.battery = m_battery | m_charging;
        switch_report->input0x30.timer = os::ConvertToTimeSpan(os::GetSystemTick()).GetMilliSeconds() & 0xff;
    }

    void OuyaController::HandleInputReport0x03(const OuyaReportData *src, SwitchReportData *dst) {
        m_battery = src->input0x03.battery / 52 << 1;

        this->PackStickData(&dst->input0x30.left_stick, STICK_ZERO, STICK_ZERO);
        this->PackStickData(&dst->input0x30.right_stick, STICK_ZERO, STICK_ZERO);
    }
    
    void OuyaController::HandleInputReport0x07(const OuyaReportData *src, SwitchReportData *dst) {
        this->PackStickData(&dst->input0x30.left_stick,
            static_cast<uint16_t>(stick_scale_factor * src->input0x07.left_stick.x) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT16_MAX - src->input0x07.left_stick.y)) & 0xfff
        );
        this->PackStickData(&dst->input0x30.right_stick,
            static_cast<uint16_t>(stick_scale_factor * src->input0x07.right_stick.x) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT16_MAX - src->input0x07.right_stick.y)) & 0xfff
        );
        
        dst->input0x30.buttons.dpad_down    = src->input0x07.buttons.dpad_down;
        dst->input0x30.buttons.dpad_up      = src->input0x07.buttons.dpad_up;
        dst->input0x30.buttons.dpad_right   = src->input0x07.buttons.dpad_right;
        dst->input0x30.buttons.dpad_left    = src->input0x07.buttons.dpad_left;
        
        dst->input0x30.buttons.A = src->input0x07.buttons.A;
        dst->input0x30.buttons.B = src->input0x07.buttons.O;
        dst->input0x30.buttons.X = src->input0x07.buttons.Y;
        dst->input0x30.buttons.Y = src->input0x07.buttons.U;

        dst->input0x30.buttons.R  = src->input0x07.buttons.RB;
        dst->input0x30.buttons.ZR = src->input0x07.buttons.RT;
        dst->input0x30.buttons.L  = src->input0x07.buttons.LB;
        dst->input0x30.buttons.ZL = src->input0x07.buttons.LT;

        dst->input0x30.buttons.minus = 0;
        dst->input0x30.buttons.plus  = 0;

        dst->input0x30.buttons.lstick_press = src->input0x07.buttons.LS;
        dst->input0x30.buttons.rstick_press = src->input0x07.buttons.RS;

        dst->input0x30.buttons.capture = 0;
        dst->input0x30.buttons.home    = src->input0x07.buttons.center_hold;
    }

}
