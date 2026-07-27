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
#include "bionik_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    namespace {

        constexpr u8 TriggerMax = UINT8_MAX;

    }

    void BionikController::ProcessInputData(const u8 *report_buffer, size_t size) {
        AMS_UNUSED(size);
        auto report = reinterpret_cast<const BionikReportData *>(report_buffer);

        switch(report->id) {
            case 0x03:
                this->MapInputReport0x03(report); break;
            case 0x04:
                this->MapInputReport0x04(report); break;
            default:
                break;
        }
    }

    void BionikController::MapInputReport0x03(const BionikReportData *src) {
        m_left_stick.SetValuesFrom(
            src->input0x03.left_stick.GetX(),
            src->input0x03.left_stick.GetYInverted()
        );

        m_right_stick.SetValuesFrom(
            src->input0x03.right_stick.GetX(),
            src->input0x03.right_stick.GetYInverted()
        );

        SwitchButtons button_state = m_buttons;
        button_state.Assign(SwitchButton::Down,  (src->input0x03.buttons.dpad == BionikDPad_S) || (src->input0x03.buttons.dpad == BionikDPad_SE) || (src->input0x03.buttons.dpad == BionikDPad_SW));
        button_state.Assign(SwitchButton::Up,    (src->input0x03.buttons.dpad == BionikDPad_N) || (src->input0x03.buttons.dpad == BionikDPad_NE) || (src->input0x03.buttons.dpad == BionikDPad_NW));
        button_state.Assign(SwitchButton::Right, (src->input0x03.buttons.dpad == BionikDPad_E) || (src->input0x03.buttons.dpad == BionikDPad_NE) || (src->input0x03.buttons.dpad == BionikDPad_SE));
        button_state.Assign(SwitchButton::Left,  (src->input0x03.buttons.dpad == BionikDPad_W) || (src->input0x03.buttons.dpad == BionikDPad_NW) || (src->input0x03.buttons.dpad == BionikDPad_SW));
        button_state.Assign(SwitchButton::A,      src->input0x03.buttons.B);
        button_state.Assign(SwitchButton::B,      src->input0x03.buttons.A);
        button_state.Assign(SwitchButton::X,      src->input0x03.buttons.Y);
        button_state.Assign(SwitchButton::Y,      src->input0x03.buttons.X);
        button_state.Assign(SwitchButton::R,      src->input0x03.buttons.R1);
        button_state.Assign(SwitchButton::L,      src->input0x03.buttons.L1);
        button_state.Assign(SwitchButton::ZR,     src->input0x03.right_trigger > (m_trigger_threshold * TriggerMax));
        button_state.Assign(SwitchButton::ZL,     src->input0x03.left_trigger  > (m_trigger_threshold * TriggerMax));
        button_state.Assign(SwitchButton::StickL, src->input0x03.buttons.L3);
        button_state.Assign(SwitchButton::StickR, src->input0x03.buttons.R3);
        button_state.Assign(SwitchButton::Plus,   src->input0x03.buttons.start);
        m_buttons = button_state;
    }

    void BionikController::MapInputReport0x04(const BionikReportData *src) {
        SwitchButtons button_state = m_buttons;
        button_state.Assign(SwitchButton::Minus, src->input0x04.buttons.back);
        button_state.Assign(SwitchButton::Home,  src->input0x04.buttons.home);
        m_buttons = button_state;
    }

}
