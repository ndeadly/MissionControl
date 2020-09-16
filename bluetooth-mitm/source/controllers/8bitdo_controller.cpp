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
#include "8bitdo_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    void EightBitDoController::ConvertReportFormat(const bluetooth::HidReport *in_report, bluetooth::HidReport *out_report) {
        auto eightbitdo_report = reinterpret_cast<const EightBitDoReportData *>(&in_report->data);
        auto switch_report = reinterpret_cast<SwitchReportData *>(&out_report->data);

        switch(eightbitdo_report->id) {
            case 0x01:
                this->HandleInputReport0x01(eightbitdo_report, switch_report);
                break;
            case 0x03:
                this->HandleInputReport0x03(eightbitdo_report, switch_report);
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

    void EightBitDoController::HandleInputReport0x01(const EightBitDoReportData *src, SwitchReportData *dst) {
        dst->input0x30.buttons.dpad_down   = (src->input0x01.dpad == EightBitDoDPad_S)  ||
                                             (src->input0x01.dpad == EightBitDoDPad_SE) ||
                                             (src->input0x01.dpad == EightBitDoDPad_SW);
        dst->input0x30.buttons.dpad_up     = (src->input0x01.dpad == EightBitDoDPad_N)  ||
                                             (src->input0x01.dpad == EightBitDoDPad_NE) ||
                                             (src->input0x01.dpad == EightBitDoDPad_NW);
        dst->input0x30.buttons.dpad_right  = (src->input0x01.dpad == EightBitDoDPad_E)  ||
                                             (src->input0x01.dpad == EightBitDoDPad_NE) ||
                                             (src->input0x01.dpad == EightBitDoDPad_SE);
        dst->input0x30.buttons.dpad_left   = (src->input0x01.dpad == EightBitDoDPad_W)  ||
                                             (src->input0x01.dpad == EightBitDoDPad_NW) ||
                                             (src->input0x01.dpad == EightBitDoDPad_SW);

    }

    void EightBitDoController::HandleInputReport0x03(const EightBitDoReportData *src, SwitchReportData *dst) {
        dst->input0x30.buttons.A = src->input0x03.buttons.B;
        dst->input0x30.buttons.B = src->input0x03.buttons.A;
        dst->input0x30.buttons.X = src->input0x03.buttons.Y;
        dst->input0x30.buttons.Y = src->input0x03.buttons.X;

        dst->input0x30.buttons.R  = src->input0x03.buttons.R;
        dst->input0x30.buttons.L  = src->input0x03.buttons.L;

        dst->input0x30.buttons.minus = src->input0x03.buttons.select;
        dst->input0x30.buttons.plus  = src->input0x03.buttons.start;

        // Home combo
        dst->input0x30.buttons.home = dst->input0x30.buttons.R && dst->input0x30.buttons.L && dst->input0x30.buttons.minus && dst->input0x30.buttons.plus;
        if (dst->input0x30.buttons.home) {
            dst->input0x30.buttons.R = 0;
            dst->input0x30.buttons.L = 0;
            dst->input0x30.buttons.minus = 0;
            dst->input0x30.buttons.plus = 0;
        }
    }

}
