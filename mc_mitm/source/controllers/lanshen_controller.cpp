/*
 * Copyright (c) 2020-2025 ndeadly
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
#include "lanshen_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    void LanShenController::ProcessInputData(const bluetooth::HidReport *report) {
        auto LanShen_report = reinterpret_cast<const LanShenReportData *>(&report->data);

        switch(LanShen_report->id) {
            case 0x01:
                this->MapInputReport0x01(LanShen_report); break;
            default:
                break;
        }
    }

    void LanShenController::MapInputReport0x01(const LanShenReportData *src) {
        m_left_stick  = PackAnalogStickValues(src->input0x01.left_stick.x,  InvertAnalogStickValue(src->input0x01.left_stick.y));
        m_right_stick = PackAnalogStickValues(src->input0x01.right_stick.x, InvertAnalogStickValue(src->input0x01.right_stick.y));
        
        m_buttons.dpad_down  = (src->input0x01.buttons.dpad == LanShenDPad_S)  ||
                               (src->input0x01.buttons.dpad == LanShenDPad_SE) ||
                               (src->input0x01.buttons.dpad == LanShenDPad_SW);
        m_buttons.dpad_up    = (src->input0x01.buttons.dpad == LanShenDPad_N)  ||
                               (src->input0x01.buttons.dpad == LanShenDPad_NE) ||
                               (src->input0x01.buttons.dpad == LanShenDPad_NW);
        m_buttons.dpad_right = (src->input0x01.buttons.dpad == LanShenDPad_E)  ||
                               (src->input0x01.buttons.dpad == LanShenDPad_NE) ||
                               (src->input0x01.buttons.dpad == LanShenDPad_SE);
        m_buttons.dpad_left  = (src->input0x01.buttons.dpad == LanShenDPad_W)  ||
                               (src->input0x01.buttons.dpad == LanShenDPad_NW) ||
                               (src->input0x01.buttons.dpad == LanShenDPad_SW);

        m_buttons.A = src->input0x01.buttons.B;
        m_buttons.B = src->input0x01.buttons.A;
        m_buttons.X = src->input0x01.buttons.Y;
        m_buttons.Y = src->input0x01.buttons.X;

        m_buttons.R  = src->input0x01.buttons.R1;
        m_buttons.ZR = src->input0x01.buttons.R2;
        m_buttons.L  = src->input0x01.buttons.L1;
        m_buttons.ZL = src->input0x01.buttons.L2;

        //m_buttons.minus = src->input0x01.buttons.select;
        m_buttons.plus  = src->input0x01.buttons.start;

        m_buttons.lstick_press = src->input0x01.buttons.L3;
        m_buttons.rstick_press = src->input0x01.buttons.R3;
    }

}
