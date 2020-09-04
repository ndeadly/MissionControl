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
#include "gamestick_controller.hpp"
#include <stratosphere.hpp>
#include <cstring>

namespace ams::controller {

    namespace {

        const constexpr float stick_scale_factor = float(UINT12_MAX) / UINT8_MAX;
        
    }

    void GamestickController::ConvertReportFormat(const bluetooth::HidReport *in_report, bluetooth::HidReport *out_report) {
        auto gamestick_report = reinterpret_cast<const GamestickReportData *>(&in_report->data);
        auto switch_report = reinterpret_cast<SwitchReportData *>(&out_report->data);

        switch(gamestick_report->id) {
            case 0x01:
                this->HandleInputReport0x01(gamestick_report, switch_report);
                break;
            case 0x03:
                this->HandleInputReport0x03(gamestick_report, switch_report);
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

    void GamestickController::HandleInputReport0x01(const GamestickReportData *src, SwitchReportData *dst) {
        dst->input0x30.buttons.minus = src->input0x01.buttons.back;
        dst->input0x30.buttons.home = src->input0x01.buttons.home;
    }

    void GamestickController::HandleInputReport0x03(const GamestickReportData *src, SwitchReportData *dst) {
        this->PackStickData(&dst->input0x30.left_stick,
            static_cast<uint16_t>(stick_scale_factor * src->input0x03.left_stick.x) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT8_MAX - src->input0x03.left_stick.y)) & 0xfff
        );
        this->PackStickData(&dst->input0x30.right_stick,
            static_cast<uint16_t>(stick_scale_factor * src->input0x03.right_stick.x) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT8_MAX - src->input0x03.right_stick.y)) & 0xfff
        );
        
        dst->input0x30.buttons.dpad_down   = (src->input0x03.dpad == GamestickDPad_S)  ||
                                             (src->input0x03.dpad == GamestickDPad_SE) ||
                                             (src->input0x03.dpad == GamestickDPad_SW);
        dst->input0x30.buttons.dpad_up     = (src->input0x03.dpad == GamestickDPad_N)  ||
                                             (src->input0x03.dpad == GamestickDPad_NE) ||
                                             (src->input0x03.dpad == GamestickDPad_NW);
        dst->input0x30.buttons.dpad_right  = (src->input0x03.dpad == GamestickDPad_E)  ||
                                             (src->input0x03.dpad == GamestickDPad_NE) ||
                                             (src->input0x03.dpad == GamestickDPad_SE);
        dst->input0x30.buttons.dpad_left   = (src->input0x03.dpad == GamestickDPad_W)  ||
                                             (src->input0x03.dpad == GamestickDPad_NW) ||
                                             (src->input0x03.dpad == GamestickDPad_SW);
        
        dst->input0x30.buttons.A = src->input0x03.buttons.B;
        dst->input0x30.buttons.B = src->input0x03.buttons.A;
        dst->input0x30.buttons.X = src->input0x03.buttons.Y;
        dst->input0x30.buttons.Y = src->input0x03.buttons.X;

        /*
        dst->input0x30.buttons.ZR = src->input0x03.buttons.start && src->input0x03.buttons.R;
        dst->input0x30.buttons.ZL = src->input0x03.buttons.start && src->input0x03.buttons.L;
        
        if (!dst->input0x30.buttons.ZR)
            dst->input0x30.buttons.R = src->input0x03.buttons.R;

        if (!dst->input0x30.buttons.ZL)
            dst->input0x30.buttons.L = src->input0x03.buttons.L;

        if (!(dst->input0x30.buttons.ZR || dst->input0x30.buttons.ZL))
            dst->input0x30.buttons.plus  = src->input0x03.buttons.start;
        */
        dst->input0x30.buttons.L = src->input0x03.buttons.L;
        dst->input0x30.buttons.R = src->input0x03.buttons.R;

        // Combos for ZL/ZR
        if (dst->input0x30.buttons.dpad_down) {
            dst->input0x30.buttons.ZL = src->input0x03.buttons.L;
            dst->input0x30.buttons.ZR = src->input0x03.buttons.R;
            dst->input0x30.buttons.dpad_down = !(dst->input0x30.buttons.ZL || dst->input0x30.buttons.ZR);
            dst->input0x30.buttons.L = !dst->input0x30.buttons.ZL;
            dst->input0x30.buttons.R = !dst->input0x30.buttons.ZR;
        }

        dst->input0x30.buttons.plus  = src->input0x03.buttons.start;

        dst->input0x30.buttons.lstick_press = src->input0x03.buttons.lstick_press;
        dst->input0x30.buttons.rstick_press = src->input0x03.buttons.rstick_press;

        dst->input0x30.buttons.capture  = 0;        
    }

}
