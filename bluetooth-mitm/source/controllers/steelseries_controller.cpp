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
#include "steelseries_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    namespace {

        const constexpr float stick_scale_factor = float(UINT12_MAX) / UINT8_MAX;

    }

    void SteelseriesController::ConvertReportFormat(const bluetooth::HidReport *in_report, bluetooth::HidReport *out_report) {
        auto steelseries_report = reinterpret_cast<const SteelseriesReportData *>(&in_report->data);
        auto switch_report = reinterpret_cast<SwitchReportData *>(&out_report->data);

        switch(steelseries_report->id) {
            case 0x01:
                this->HandleInputReport0x01(steelseries_report, switch_report);
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

    void SteelseriesController::HandleInputReport0x01(const SteelseriesReportData *src, SwitchReportData *dst) {
        this->PackStickData(&dst->input0x30.left_stick,
            static_cast<uint16_t>(stick_scale_factor * -static_cast<int8_t>(~src->input0x01.left_stick.x + 1) + 0x7ff) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT8_MAX + static_cast<int8_t>(~src->input0x01.left_stick.y + 1)) + 0x7ff) & 0xfff
        );
        this->PackStickData(&dst->input0x30.right_stick,
            static_cast<uint16_t>(stick_scale_factor * -static_cast<int8_t>(~src->input0x01.right_stick.x + 1) + 0x7ff) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT8_MAX + static_cast<int8_t>(~src->input0x01.right_stick.y + 1)) + 0x7ff) & 0xfff
        );

        dst->input0x30.buttons.dpad_down    = (src->input0x01.dpad == SteelseriesDPad_S)  ||
                                              (src->input0x01.dpad == SteelseriesDPad_SE) ||
                                              (src->input0x01.dpad == SteelseriesDPad_SW);
        dst->input0x30.buttons.dpad_up      = (src->input0x01.dpad == SteelseriesDPad_N)  ||
                                              (src->input0x01.dpad == SteelseriesDPad_NE) ||
                                              (src->input0x01.dpad == SteelseriesDPad_NW);
        dst->input0x30.buttons.dpad_right   = (src->input0x01.dpad == SteelseriesDPad_E)  ||
                                              (src->input0x01.dpad == SteelseriesDPad_NE) ||
                                              (src->input0x01.dpad == SteelseriesDPad_SE);
        dst->input0x30.buttons.dpad_left    = (src->input0x01.dpad == SteelseriesDPad_W)  ||
                                              (src->input0x01.dpad == SteelseriesDPad_NW) ||
                                              (src->input0x01.dpad == SteelseriesDPad_SW);

        dst->input0x30.buttons.A = src->input0x01.buttons.B;
        dst->input0x30.buttons.B = src->input0x01.buttons.A;
        dst->input0x30.buttons.X = src->input0x01.buttons.Y;
        dst->input0x30.buttons.Y = src->input0x01.buttons.X;

        dst->input0x30.buttons.R  = src->input0x01.buttons.R;
        dst->input0x30.buttons.L  = src->input0x01.buttons.L;

        dst->input0x30.buttons.minus    = src->input0x01.buttons.select;
        dst->input0x30.buttons.plus     = src->input0x01.buttons.start;

        dst->input0x30.buttons.capture  = 0;

        // Home button combo
        dst->input0x30.buttons.home = dst->input0x30.buttons.dpad_down & dst->input0x30.buttons.minus;
        if (dst->input0x30.buttons.home){
            dst->input0x30.buttons.dpad_down    = 0;
            dst->input0x30.buttons.minus        = 0;
        }
        
    }

}
