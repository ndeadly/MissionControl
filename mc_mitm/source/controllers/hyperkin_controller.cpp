/*
 * Copyright (c) 2020-2026 ndeadly
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
        m_buttons.Assign(SwitchButton::Down,  (src->input0x3f.buttons.dpad == HyperkinDPad_S) || (src->input0x3f.buttons.dpad == HyperkinDPad_SE) || (src->input0x3f.buttons.dpad == HyperkinDPad_SW));
        m_buttons.Assign(SwitchButton::Up,    (src->input0x3f.buttons.dpad == HyperkinDPad_N) || (src->input0x3f.buttons.dpad == HyperkinDPad_NE) || (src->input0x3f.buttons.dpad == HyperkinDPad_NW));
        m_buttons.Assign(SwitchButton::Right, (src->input0x3f.buttons.dpad == HyperkinDPad_E) || (src->input0x3f.buttons.dpad == HyperkinDPad_NE) || (src->input0x3f.buttons.dpad == HyperkinDPad_SE));
        m_buttons.Assign(SwitchButton::Left,  (src->input0x3f.buttons.dpad == HyperkinDPad_W) || (src->input0x3f.buttons.dpad == HyperkinDPad_NW) || (src->input0x3f.buttons.dpad == HyperkinDPad_SW));
        m_buttons.Assign(SwitchButton::A,     src->input0x3f.buttons.A);
        m_buttons.Assign(SwitchButton::B,     src->input0x3f.buttons.B);
        m_buttons.Assign(SwitchButton::X,     src->input0x3f.buttons.X);
        m_buttons.Assign(SwitchButton::Y,     src->input0x3f.buttons.Y);
        m_buttons.Assign(SwitchButton::L,     src->input0x3f.buttons.L);
        m_buttons.Assign(SwitchButton::R,     src->input0x3f.buttons.R);
        m_buttons.Assign(SwitchButton::Minus, src->input0x3f.buttons.select);
        m_buttons.Assign(SwitchButton::Plus,  src->input0x3f.buttons.start);
    }

}
