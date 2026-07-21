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
#include "atari_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    namespace {

        constexpr u16 TriggerMax = 0x3ff;

    }

    void AtariController::ProcessInputData(const bluetooth::HidReport *report) {
        auto atari_report = reinterpret_cast<const AtariReportData *>(&report->data);

        switch(atari_report->id) {
            case 0x01:
                this->MapInputReport0x01(atari_report); break;
            case 0x02:
                this->MapInputReport0x02(atari_report); break;
            default:
                break;
        }
    }

    void AtariController::MapInputReport0x01(const AtariReportData *src) {
        m_left_stick.SetValuesFrom(
            src->input0x01.left_stick.GetX(),
            src->input0x01.left_stick.GetYInverted()
        );

        m_right_stick.SetValuesFrom(
            src->input0x01.right_stick.GetX(),
            src->input0x01.right_stick.GetYInverted()
        );

        SwitchButtons button_state = m_buttons;
        button_state.Assign(SwitchButton::Down,  (src->input0x01.buttons.dpad == AtariDPad_S) || (src->input0x01.buttons.dpad == AtariDPad_SE) || (src->input0x01.buttons.dpad == AtariDPad_SW));
        button_state.Assign(SwitchButton::Up,    (src->input0x01.buttons.dpad == AtariDPad_N) || (src->input0x01.buttons.dpad == AtariDPad_NE) || (src->input0x01.buttons.dpad == AtariDPad_NW));
        button_state.Assign(SwitchButton::Right, (src->input0x01.buttons.dpad == AtariDPad_E) || (src->input0x01.buttons.dpad == AtariDPad_NE) || (src->input0x01.buttons.dpad == AtariDPad_SE));
        button_state.Assign(SwitchButton::Left,  (src->input0x01.buttons.dpad == AtariDPad_W) || (src->input0x01.buttons.dpad == AtariDPad_NW) || (src->input0x01.buttons.dpad == AtariDPad_SW));
        button_state.Assign(SwitchButton::A,      src->input0x01.buttons.B);
        button_state.Assign(SwitchButton::B,      src->input0x01.buttons.A);
        button_state.Assign(SwitchButton::X,      src->input0x01.buttons.Y);
        button_state.Assign(SwitchButton::Y,      src->input0x01.buttons.X);
        button_state.Assign(SwitchButton::R,      src->input0x01.buttons.RB);
        button_state.Assign(SwitchButton::L,      src->input0x01.buttons.LB);
        button_state.Assign(SwitchButton::ZR,     src->input0x01.right_trigger > (m_trigger_threshold * TriggerMax));
        button_state.Assign(SwitchButton::ZL,     src->input0x01.left_trigger  > (m_trigger_threshold * TriggerMax));
        button_state.Assign(SwitchButton::StickL, src->input0x01.buttons.L3);
        button_state.Assign(SwitchButton::StickR, src->input0x01.buttons.R3);
        button_state.Assign(SwitchButton::Minus,  src->input0x01.buttons.back);
        button_state.Assign(SwitchButton::Plus,   src->input0x01.buttons.menu);
        button_state.Assign(SwitchButton::Home,   src->input0x01.buttons.home);
        m_buttons = button_state;
    }

    void AtariController::MapInputReport0x02(const AtariReportData *src) {
        AMS_UNUSED(src);
    }

}
