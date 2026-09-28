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
#include "ouya_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    namespace {

        constexpr u16 TriggerMax = UINT16_MAX;

    }

    void OuyaController::ParseInputReport(const u8 *report_buffer, size_t size) {
        AMS_UNUSED(size);
        auto report = reinterpret_cast<const OuyaReportData *>(report_buffer);

        switch(report->id) {
            case 0x03:
                this->MapInputReport0x03(report); break;
            case 0x07:
                this->MapInputReport0x07(report); break;
            default:
                break;
        }
    }

    void OuyaController::MapInputReport0x03(const OuyaReportData *src) {
        auto battery_level = SwitchBatteryLevelConverter::ConvertValue(src->input0x03.battery);
        m_power_info.SetBatteryLevel(battery_level);
    }
    
    void OuyaController::MapInputReport0x07(const OuyaReportData *src) {
        m_left_stick.SetValuesFrom(
            src->input0x07.left_stick.GetX(),
            src->input0x07.left_stick.GetYInverted()
        );

        m_right_stick.SetValuesFrom(
            src->input0x07.right_stick.GetX(),
            src->input0x07.right_stick.GetYInverted()
        );

        SwitchButtons button_state = m_buttons;
        button_state.Assign(SwitchButton::Down,   src->input0x07.buttons.dpad_down);
        button_state.Assign(SwitchButton::Up,     src->input0x07.buttons.dpad_up);
        button_state.Assign(SwitchButton::Right,  src->input0x07.buttons.dpad_right);
        button_state.Assign(SwitchButton::Left,   src->input0x07.buttons.dpad_left);
        button_state.Assign(SwitchButton::A,      src->input0x07.buttons.A);
        button_state.Assign(SwitchButton::B,      src->input0x07.buttons.O);
        button_state.Assign(SwitchButton::X,      src->input0x07.buttons.Y);
        button_state.Assign(SwitchButton::Y,      src->input0x07.buttons.U);
        button_state.Assign(SwitchButton::R,      src->input0x07.buttons.RB);
        button_state.Assign(SwitchButton::L,      src->input0x07.buttons.LB);
        button_state.Assign(SwitchButton::ZR,     src->input0x07.right_trigger > (m_trigger_threshold * TriggerMax));
        button_state.Assign(SwitchButton::ZL,     src->input0x07.left_trigger  > (m_trigger_threshold * TriggerMax));
        button_state.Assign(SwitchButton::StickL, src->input0x07.buttons.LS);
        button_state.Assign(SwitchButton::StickR, src->input0x07.buttons.RS);
        button_state.Assign(SwitchButton::Home,   src->input0x07.buttons.center_hold);
        m_buttons = button_state;
    }

}
