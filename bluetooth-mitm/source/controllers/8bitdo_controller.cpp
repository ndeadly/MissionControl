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

    void EightBitDoController::UpdateControllerState(const bluetooth::HidReport *report) {
        auto eightbitdo_report = reinterpret_cast<const EightBitDoReportData *>(&report->data);

        switch(eightbitdo_report->id) {
            case 0x01:
                this->HandleInputReport0x01(eightbitdo_report);
                break;
            case 0x03:
                this->HandleInputReport0x03(eightbitdo_report);
                break;
            default:
                break;
        }
    }

    void EightBitDoController::HandleInputReport0x01(const EightBitDoReportData *src) {
        m_buttons.dpad_down   = (src->input0x01.dpad == EightBitDoDPad_S)  ||
                                (src->input0x01.dpad == EightBitDoDPad_SE) ||
                                (src->input0x01.dpad == EightBitDoDPad_SW);
        m_buttons.dpad_up     = (src->input0x01.dpad == EightBitDoDPad_N)  ||
                                (src->input0x01.dpad == EightBitDoDPad_NE) ||
                                (src->input0x01.dpad == EightBitDoDPad_NW);
        m_buttons.dpad_right  = (src->input0x01.dpad == EightBitDoDPad_E)  ||
                                (src->input0x01.dpad == EightBitDoDPad_NE) ||
                                (src->input0x01.dpad == EightBitDoDPad_SE);
        m_buttons.dpad_left   = (src->input0x01.dpad == EightBitDoDPad_W)  ||
                                (src->input0x01.dpad == EightBitDoDPad_NW) ||
                                (src->input0x01.dpad == EightBitDoDPad_SW);
    }

    void EightBitDoController::HandleInputReport0x03(const EightBitDoReportData *src) {
        m_buttons.A = src->input0x03.buttons.B;
        m_buttons.B = src->input0x03.buttons.A;
        m_buttons.X = src->input0x03.buttons.Y;
        m_buttons.Y = src->input0x03.buttons.X;

        m_buttons.R  = src->input0x03.buttons.R;
        m_buttons.L  = src->input0x03.buttons.L;

        m_buttons.minus = src->input0x03.buttons.select;
        m_buttons.plus  = src->input0x03.buttons.start;
    }

}
