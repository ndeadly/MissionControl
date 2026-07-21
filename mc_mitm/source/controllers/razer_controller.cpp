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
#include "razer_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    namespace {

        constexpr u8 TriggerMax = UINT8_MAX;

    }

    void RazerController::ProcessInputData(const bluetooth::HidReport *report) {
        auto razer_report = reinterpret_cast<const RazerReportData *>(&report->data);

        switch(razer_report->id) {
            case 0x01:
                this->MapInputReport0x01(razer_report); break;
            default:
                break;
        }
    }

    void RazerController::MapInputReport0x01(const RazerReportData *src) {
        m_left_stick.SetValuesFrom(
            src->input0x01.left_stick.GetX(),
            src->input0x01.left_stick.GetYInverted()
        );

        m_right_stick.SetValuesFrom(
            src->input0x01.right_stick.GetX(),
            src->input0x01.right_stick.GetYInverted()
        );

        m_buttons.Assign(SwitchButton::Down,    (src->input0x01.buttons.dpad == RazerDPad_S) || (src->input0x01.buttons.dpad == RazerDPad_SE) || (src->input0x01.buttons.dpad == RazerDPad_SW));
        m_buttons.Assign(SwitchButton::Up,      (src->input0x01.buttons.dpad == RazerDPad_N) || (src->input0x01.buttons.dpad == RazerDPad_NE) || (src->input0x01.buttons.dpad == RazerDPad_NW));
        m_buttons.Assign(SwitchButton::Right,   (src->input0x01.buttons.dpad == RazerDPad_E) || (src->input0x01.buttons.dpad == RazerDPad_NE) || (src->input0x01.buttons.dpad == RazerDPad_SE));
        m_buttons.Assign(SwitchButton::Left,    (src->input0x01.buttons.dpad == RazerDPad_W) || (src->input0x01.buttons.dpad == RazerDPad_NW) || (src->input0x01.buttons.dpad == RazerDPad_SW));
        m_buttons.Assign(SwitchButton::A,       src->input0x01.buttons.B);
        m_buttons.Assign(SwitchButton::B,       src->input0x01.buttons.A);
        m_buttons.Assign(SwitchButton::X,       src->input0x01.buttons.Y);
        m_buttons.Assign(SwitchButton::Y,       src->input0x01.buttons.X);
        m_buttons.Assign(SwitchButton::R,       src->input0x01.buttons.R1);
        m_buttons.Assign(SwitchButton::L,       src->input0x01.buttons.L1);
        m_buttons.Assign(SwitchButton::ZR,      src->input0x01.right_trigger > (m_trigger_threshold * TriggerMax));
        m_buttons.Assign(SwitchButton::ZL,      src->input0x01.left_trigger  > (m_trigger_threshold * TriggerMax));
        m_buttons.Assign(SwitchButton::Minus,   src->input0x01.buttons.select);
        m_buttons.Assign(SwitchButton::Plus,    src->input0x01.buttons.start);
        m_buttons.Assign(SwitchButton::StickL,  src->input0x01.buttons.L3);
        m_buttons.Assign(SwitchButton::StickR,  src->input0x01.buttons.R3);
        m_buttons.Assign(SwitchButton::Capture, src->input0x01.buttons.back);
        m_buttons.Assign(SwitchButton::Home,    src->input0x01.buttons.home);
    }

}
