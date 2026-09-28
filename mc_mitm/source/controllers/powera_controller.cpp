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
#include "powera_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    void PowerAController::ParseInputReport(const u8 *report_buffer, size_t size) {
        AMS_UNUSED(size);
        auto report = reinterpret_cast<const PowerAReportData *>(report_buffer);

        switch(report->id) {
            case 0x03:
                this->MapInputReport0x03(report); break;
            default:
                break;
        }
    }

    void PowerAController::MapInputReport0x03(const PowerAReportData *src) {
        auto battery_level = SwitchBatteryLevelConverter::ConvertValue(src->input0x03.battery);
        m_power_info.SetBatteryLevel(battery_level);

        auto dpad = DirectionalPad(static_cast<DirectionalPadType<0, 0xF>>(src->input0x03.buttons.dpad));

        m_left_stick.SetValuesFrom(
            src->input0x03.left_stick.GetX(),
            src->input0x03.left_stick.GetYInverted()
        );

        m_right_stick.SetValuesFrom(
            src->input0x03.right_stick.GetX(),
            src->input0x03.right_stick.GetYInverted()
        );

        SwitchButtons button_state = m_buttons;
        button_state.Assign(SwitchButton::Down,   dpad.IsDown());
        button_state.Assign(SwitchButton::Up,     dpad.IsUp());
        button_state.Assign(SwitchButton::Right,  dpad.IsRight());
        button_state.Assign(SwitchButton::Left,   dpad.IsLeft());
        button_state.Assign(SwitchButton::A,      src->input0x03.buttons.B);
        button_state.Assign(SwitchButton::B,      src->input0x03.buttons.A);
        button_state.Assign(SwitchButton::X,      src->input0x03.buttons.Y);
        button_state.Assign(SwitchButton::Y,      src->input0x03.buttons.X);
        button_state.Assign(SwitchButton::R,      src->input0x03.buttons.R1);
        button_state.Assign(SwitchButton::L,      src->input0x03.buttons.L1);
        button_state.Assign(SwitchButton::ZR,     src->input0x03.R2 > 0);
        button_state.Assign(SwitchButton::ZL,     src->input0x03.L2 > 0);
        button_state.Assign(SwitchButton::Minus,  src->input0x03.buttons.select);
        button_state.Assign(SwitchButton::Plus,   src->input0x03.buttons.start);
        button_state.Assign(SwitchButton::StickL, src->input0x03.buttons.L3);
        button_state.Assign(SwitchButton::StickR, src->input0x03.buttons.R3);
        m_buttons = button_state;
    }

}
