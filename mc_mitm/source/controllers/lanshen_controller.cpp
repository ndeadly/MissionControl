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

        m_buttons.Assign(SwitchButton::Down,   (src->input0x01.buttons.dpad == LanShenDPad_S) || (src->input0x01.buttons.dpad == LanShenDPad_SE) || (src->input0x01.buttons.dpad == LanShenDPad_SW));
        m_buttons.Assign(SwitchButton::Up,     (src->input0x01.buttons.dpad == LanShenDPad_N) || (src->input0x01.buttons.dpad == LanShenDPad_NE) || (src->input0x01.buttons.dpad == LanShenDPad_NW));
        m_buttons.Assign(SwitchButton::Right,  (src->input0x01.buttons.dpad == LanShenDPad_E) || (src->input0x01.buttons.dpad == LanShenDPad_NE) || (src->input0x01.buttons.dpad == LanShenDPad_SE));
        m_buttons.Assign(SwitchButton::Left,   (src->input0x01.buttons.dpad == LanShenDPad_W) || (src->input0x01.buttons.dpad == LanShenDPad_NW) || (src->input0x01.buttons.dpad == LanShenDPad_SW));
        m_buttons.Assign(SwitchButton::A,      src->input0x01.buttons.B);
        m_buttons.Assign(SwitchButton::B,      src->input0x01.buttons.A);
        m_buttons.Assign(SwitchButton::X,      src->input0x01.buttons.Y);
        m_buttons.Assign(SwitchButton::Y,      src->input0x01.buttons.X);
        m_buttons.Assign(SwitchButton::R,      src->input0x01.buttons.R1);
        m_buttons.Assign(SwitchButton::L,      src->input0x01.buttons.L1);
        m_buttons.Assign(SwitchButton::ZL,     src->input0x01.buttons.L2);
        m_buttons.Assign(SwitchButton::ZR,     src->input0x01.buttons.R2);
        // m_buttons.Assign(SwitchButton::Minus,  src->input0x01.buttons.select);
        m_buttons.Assign(SwitchButton::Plus,   src->input0x01.buttons.start);
        m_buttons.Assign(SwitchButton::StickL, src->input0x01.buttons.L3);
        m_buttons.Assign(SwitchButton::StickR, src->input0x01.buttons.R3);
    }

}
