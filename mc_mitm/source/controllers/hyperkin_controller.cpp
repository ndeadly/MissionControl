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

    void HyperkinController::ParseInputReport(const u8 *report_buffer, size_t size) {
        AMS_UNUSED(size);
        auto report = reinterpret_cast<const HyperkinReportData *>(report_buffer);

        switch(report->id) {
            case 0x3F:
                this->MapInputReport0x3F(report); break;
            default:
                break;
        }
    }

    void HyperkinController::MapInputReport0x3F(const HyperkinReportData *src) {
        auto dpad = DirectionalPad(src->input0x3F.dpad);

        SwitchButtons button_state = m_buttons;
        button_state.Assign(SwitchButton::Down,  dpad.IsDown());
        button_state.Assign(SwitchButton::Up,    dpad.IsUp());
        button_state.Assign(SwitchButton::Right, dpad.IsRight());
        button_state.Assign(SwitchButton::Left,  dpad.IsLeft());
        button_state.Assign(SwitchButton::A,     src->input0x3F.buttons.A);
        button_state.Assign(SwitchButton::B,     src->input0x3F.buttons.B);
        button_state.Assign(SwitchButton::X,     src->input0x3F.buttons.X);
        button_state.Assign(SwitchButton::Y,     src->input0x3F.buttons.Y);
        button_state.Assign(SwitchButton::L,     src->input0x3F.buttons.L);
        button_state.Assign(SwitchButton::R,     src->input0x3F.buttons.R);
        button_state.Assign(SwitchButton::Minus, src->input0x3F.buttons.select);
        button_state.Assign(SwitchButton::Plus,  src->input0x3F.buttons.start);
        m_buttons = button_state;
    }

}
