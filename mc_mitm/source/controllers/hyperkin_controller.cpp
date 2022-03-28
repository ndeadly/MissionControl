/*
 * Copyright (c) 2020-2022 ndeadly
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
#include "hyperkin_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    namespace {

    }

    void HyperkinController::ProcessInputData(const bluetooth::HidReport *report) {
        auto hyperkin_report = reinterpret_cast<const HyperkinReportData *>(&report->data);

        switch(hyperkin_report->id) {
            case 0x3f:
                this->MapInputReport0x3f(hyperkin_report); break;
            default:
                break;
        }
    }

    void HyperkinController::MapInputReport0x3f(const HyperkinReportData *src) {
        m_buttons.dpad_down   = (src->input0x3f.buttons.dpad == HyperkinDPad_S)  ||
                                (src->input0x3f.buttons.dpad == HyperkinDPad_SE) ||
                                (src->input0x3f.buttons.dpad == HyperkinDPad_SW);
        m_buttons.dpad_up     = (src->input0x3f.buttons.dpad == HyperkinDPad_N)  ||
                                (src->input0x3f.buttons.dpad == HyperkinDPad_NE) ||
                                (src->input0x3f.buttons.dpad == HyperkinDPad_NW);
        m_buttons.dpad_right  = (src->input0x3f.buttons.dpad == HyperkinDPad_E)  ||
                                (src->input0x3f.buttons.dpad == HyperkinDPad_NE) ||
                                (src->input0x3f.buttons.dpad == HyperkinDPad_SE);
        m_buttons.dpad_left   = (src->input0x3f.buttons.dpad == HyperkinDPad_W)  ||
                                (src->input0x3f.buttons.dpad == HyperkinDPad_NW) ||
                                (src->input0x3f.buttons.dpad == HyperkinDPad_SW);

        m_buttons.A = src->input0x3f.buttons.A;
        m_buttons.B = src->input0x3f.buttons.B;
        m_buttons.X = src->input0x3f.buttons.X;
        m_buttons.Y = src->input0x3f.buttons.Y;

        m_buttons.L = src->input0x3f.buttons.L;
        m_buttons.R = src->input0x3f.buttons.R;

        m_buttons.minus = src->input0x3f.buttons.select;
        m_buttons.plus  = src->input0x3f.buttons.start;
    }

}
