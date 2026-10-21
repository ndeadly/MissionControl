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
#include "stadia_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    void StadiaController::ParseInputReport(const u8 *report_buffer, size_t size) {
        auto report = reinterpret_cast<const StadiaReportData *>(report_buffer);

        switch (report->id) {
            case 0x03:
                this->MapInputReport0x03(report);
                break;
            default:
                break;
        }
    }

    void StadiaController::MapInputReport0x03(const StadiaReportData *src) {
        auto dpad = static_cast<StadiaDpadDirection>(src->input0x03.dpad);

        m_left_stick.SetValuesFrom(
            src->input0x03.left_stick.GetX(),
            src->input0x03.left_stick.GetYInverted()
        );

        m_right_stick.SetValuesFrom(
            src->input0x03.right_stick.GetX(),
            src->input0x03.right_stick.GetYInverted()
        );

        SwitchButtons button_state = m_buttons;

        constexpr u8 TriggerMax = 0xff;
        button_state.Assign(SwitchButton::ZL, src->input0x03.left_trigger  > (m_trigger_threshold * TriggerMax));
        button_state.Assign(SwitchButton::ZR, src->input0x03.right_trigger > (m_trigger_threshold * TriggerMax));

        button_state.Assign(SwitchButton::Up,    dpad == StadiaDpad_N  || dpad == StadiaDpad_NE || dpad == StadiaDpad_NW);
        button_state.Assign(SwitchButton::Down,  dpad == StadiaDpad_S  || dpad == StadiaDpad_SE || dpad == StadiaDpad_SW);
        button_state.Assign(SwitchButton::Left,  dpad == StadiaDpad_W  || dpad == StadiaDpad_NW || dpad == StadiaDpad_SW);
        button_state.Assign(SwitchButton::Right, dpad == StadiaDpad_E  || dpad == StadiaDpad_NE || dpad == StadiaDpad_SE);

        // Face buttons: map by position to match Switch convention (A=right, B=bottom, X=top, Y=left)
        button_state.Assign(SwitchButton::A, src->input0x03.buttons1.b);
        button_state.Assign(SwitchButton::B, src->input0x03.buttons1.a);
        button_state.Assign(SwitchButton::X, src->input0x03.buttons1.y);
        button_state.Assign(SwitchButton::Y, src->input0x03.buttons1.x);

        button_state.Assign(SwitchButton::L, src->input0x03.buttons1.lb);
        button_state.Assign(SwitchButton::R, src->input0x03.buttons1.rb);

        button_state.Assign(SwitchButton::StickL, src->input0x03.buttons0.lstick);
        button_state.Assign(SwitchButton::StickR, src->input0x03.buttons1.rstick);

        button_state.Assign(SwitchButton::Plus,    src->input0x03.buttons0.start);
        button_state.Assign(SwitchButton::Minus,   src->input0x03.buttons0.select);
        button_state.Assign(SwitchButton::Home,    src->input0x03.buttons0.home);
        button_state.Assign(SwitchButton::Capture, src->input0x03.buttons0.capture);

        m_buttons = button_state;
    }

}
