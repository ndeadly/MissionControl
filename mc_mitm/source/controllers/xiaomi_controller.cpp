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
#include "xiaomi_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    namespace {

        constexpr u8 TriggerMax = UINT8_MAX;

        constexpr u8 InitPacket[] = { 0x20, 0x00, 0x00 };  // packet to init vibration apparently

    }

    Result XiaomiController::Initialize() {
        R_TRY(EmulatedSwitchController::Initialize());

        R_TRY(this->WriteDataReport(InitPacket, sizeof(InitPacket)));

        R_SUCCEED();
    }

    void XiaomiController::ParseInputReport(const u8 *report_buffer, size_t size) {
        AMS_UNUSED(size);
        auto report = reinterpret_cast<const XiaomiReportData *>(report_buffer);

        switch(report->id) {
            case 0x04:
                this->MapInputReport0x04(report); break;
            default:
                break;
        }
    }

    void XiaomiController::MapInputReport0x04(const XiaomiReportData *src) {
        auto battery_level = SwitchBatteryLevelConverter::ConvertPercentage(src->input0x04.battery);
        m_power_info.SetBatteryLevel(battery_level);

        auto dpad = DirectionalPad(src->input0x04.dpad);

        m_left_stick.SetValuesFrom(
            src->input0x04.left_stick.GetX(),
            src->input0x04.left_stick.GetYInverted()
        );

        m_right_stick.SetValuesFrom(
            src->input0x04.right_stick.GetX(),
            src->input0x04.right_stick.GetYInverted()
        );

        SwitchButtons button_state = m_buttons;
        button_state.Assign(SwitchButton::Down,   dpad.IsDown());
        button_state.Assign(SwitchButton::Up,     dpad.IsUp());
        button_state.Assign(SwitchButton::Right,  dpad.IsRight());
        button_state.Assign(SwitchButton::Left,   dpad.IsLeft());
        button_state.Assign(SwitchButton::A,      src->input0x04.buttons.B);
        button_state.Assign(SwitchButton::B,      src->input0x04.buttons.A);
        button_state.Assign(SwitchButton::X,      src->input0x04.buttons.Y);
        button_state.Assign(SwitchButton::Y,      src->input0x04.buttons.X);
        button_state.Assign(SwitchButton::R,      src->input0x04.buttons.R1);
        button_state.Assign(SwitchButton::L,      src->input0x04.buttons.L1);
        button_state.Assign(SwitchButton::ZR,     src->input0x04.right_trigger > (m_trigger_threshold * TriggerMax));
        button_state.Assign(SwitchButton::ZL,     src->input0x04.left_trigger  > (m_trigger_threshold * TriggerMax));
        button_state.Assign(SwitchButton::Minus,  src->input0x04.buttons.back);
        button_state.Assign(SwitchButton::Plus,   src->input0x04.buttons.menu);
        button_state.Assign(SwitchButton::StickL, src->input0x04.buttons.lstick_press);
        button_state.Assign(SwitchButton::StickR, src->input0x04.buttons.rstick_press);
        button_state.Assign(SwitchButton::Home,   src->input0x04.home);
        m_buttons = button_state;
    }

}
