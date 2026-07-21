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
        m_left_stick.SetValuesFrom(
            src->input0x01.left_stick.GetX(),
            src->input0x01.left_stick.GetYInverted()
        );

        m_right_stick.SetValuesFrom(
            src->input0x01.right_stick.GetX(),
            src->input0x01.right_stick.GetYInverted()
        );

        SwitchButtons button_state = m_buttons;
        button_state.Assign(SwitchButton::Down,   (src->input0x01.buttons.dpad == LanShenDPad_S) || (src->input0x01.buttons.dpad == LanShenDPad_SE) || (src->input0x01.buttons.dpad == LanShenDPad_SW));
        button_state.Assign(SwitchButton::Up,     (src->input0x01.buttons.dpad == LanShenDPad_N) || (src->input0x01.buttons.dpad == LanShenDPad_NE) || (src->input0x01.buttons.dpad == LanShenDPad_NW));
        button_state.Assign(SwitchButton::Right,  (src->input0x01.buttons.dpad == LanShenDPad_E) || (src->input0x01.buttons.dpad == LanShenDPad_NE) || (src->input0x01.buttons.dpad == LanShenDPad_SE));
        button_state.Assign(SwitchButton::Left,   (src->input0x01.buttons.dpad == LanShenDPad_W) || (src->input0x01.buttons.dpad == LanShenDPad_NW) || (src->input0x01.buttons.dpad == LanShenDPad_SW));
        button_state.Assign(SwitchButton::A,      src->input0x01.buttons.B);
        button_state.Assign(SwitchButton::B,      src->input0x01.buttons.A);
        button_state.Assign(SwitchButton::X,      src->input0x01.buttons.Y);
        button_state.Assign(SwitchButton::Y,      src->input0x01.buttons.X);
        button_state.Assign(SwitchButton::R,      src->input0x01.buttons.R1);
        button_state.Assign(SwitchButton::L,      src->input0x01.buttons.L1);
        button_state.Assign(SwitchButton::ZL,     src->input0x01.buttons.L2);
        button_state.Assign(SwitchButton::ZR,     src->input0x01.buttons.R2);
        // button_state.Assign(SwitchButton::Minus,  src->input0x01.buttons.select);
        button_state.Assign(SwitchButton::Plus,   src->input0x01.buttons.start);
        button_state.Assign(SwitchButton::StickL, src->input0x01.buttons.L3);
        button_state.Assign(SwitchButton::StickR, src->input0x01.buttons.R3);
        m_buttons = button_state;
    }

}
