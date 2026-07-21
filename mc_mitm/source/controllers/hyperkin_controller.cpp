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
        SwitchButtons button_state = m_buttons;
        button_state.Assign(SwitchButton::Down,  (src->input0x3f.buttons.dpad == HyperkinDPad_S) || (src->input0x3f.buttons.dpad == HyperkinDPad_SE) || (src->input0x3f.buttons.dpad == HyperkinDPad_SW));
        button_state.Assign(SwitchButton::Up,    (src->input0x3f.buttons.dpad == HyperkinDPad_N) || (src->input0x3f.buttons.dpad == HyperkinDPad_NE) || (src->input0x3f.buttons.dpad == HyperkinDPad_NW));
        button_state.Assign(SwitchButton::Right, (src->input0x3f.buttons.dpad == HyperkinDPad_E) || (src->input0x3f.buttons.dpad == HyperkinDPad_NE) || (src->input0x3f.buttons.dpad == HyperkinDPad_SE));
        button_state.Assign(SwitchButton::Left,  (src->input0x3f.buttons.dpad == HyperkinDPad_W) || (src->input0x3f.buttons.dpad == HyperkinDPad_NW) || (src->input0x3f.buttons.dpad == HyperkinDPad_SW));
        button_state.Assign(SwitchButton::A,     src->input0x3f.buttons.A);
        button_state.Assign(SwitchButton::B,     src->input0x3f.buttons.B);
        button_state.Assign(SwitchButton::X,     src->input0x3f.buttons.X);
        button_state.Assign(SwitchButton::Y,     src->input0x3f.buttons.Y);
        button_state.Assign(SwitchButton::L,     src->input0x3f.buttons.L);
        button_state.Assign(SwitchButton::R,     src->input0x3f.buttons.R);
        button_state.Assign(SwitchButton::Minus, src->input0x3f.buttons.select);
        button_state.Assign(SwitchButton::Plus,  src->input0x3f.buttons.start);
        m_buttons = button_state;
    }

}
