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

    void PowerAController::ProcessInputData(const bluetooth::HidReport *report) {
        auto powera_report = reinterpret_cast<const PowerAReportData *>(&report->data);

        switch(powera_report->id) {
            case 0x03:
                this->MapInputReport0x03(powera_report); break;
            default:
                break;
        }
    }

    void PowerAController::MapInputReport0x03(const PowerAReportData *src) {
        auto battery_level = SwitchBatteryLevelConverter::ConvertValue(src->input0x03.battery);
        m_power_info.SetBatteryLevel(battery_level);

        m_left_stick.SetValuesFrom(
            src->input0x03.left_stick.GetX(),
            src->input0x03.left_stick.GetYInverted()
        );

        m_right_stick.SetValuesFrom(
            src->input0x03.right_stick.GetX(),
            src->input0x03.right_stick.GetYInverted()
        );

        m_buttons.Assign(SwitchButton::Down,   (src->input0x03.buttons.dpad == PowerADPad_S) || (src->input0x03.buttons.dpad == PowerADPad_SE) || (src->input0x03.buttons.dpad == PowerADPad_SW));
        m_buttons.Assign(SwitchButton::Up,     (src->input0x03.buttons.dpad == PowerADPad_N) || (src->input0x03.buttons.dpad == PowerADPad_NE) || (src->input0x03.buttons.dpad == PowerADPad_NW));
        m_buttons.Assign(SwitchButton::Right,  (src->input0x03.buttons.dpad == PowerADPad_E) || (src->input0x03.buttons.dpad == PowerADPad_NE) || (src->input0x03.buttons.dpad == PowerADPad_SE));
        m_buttons.Assign(SwitchButton::Left,   (src->input0x03.buttons.dpad == PowerADPad_W) || (src->input0x03.buttons.dpad == PowerADPad_NW) || (src->input0x03.buttons.dpad == PowerADPad_SW));
        m_buttons.Assign(SwitchButton::A,      src->input0x03.buttons.B);
        m_buttons.Assign(SwitchButton::B,      src->input0x03.buttons.A);
        m_buttons.Assign(SwitchButton::X,      src->input0x03.buttons.Y);
        m_buttons.Assign(SwitchButton::Y,      src->input0x03.buttons.X);
        m_buttons.Assign(SwitchButton::R,      src->input0x03.buttons.R1);
        m_buttons.Assign(SwitchButton::L,      src->input0x03.buttons.L1);
        m_buttons.Assign(SwitchButton::ZR,     src->input0x03.R2 > 0);
        m_buttons.Assign(SwitchButton::ZL,     src->input0x03.L2 > 0);
        m_buttons.Assign(SwitchButton::Minus,  src->input0x03.buttons.select);
        m_buttons.Assign(SwitchButton::Plus,   src->input0x03.buttons.start);
        m_buttons.Assign(SwitchButton::StickL, src->input0x03.buttons.L3);
        m_buttons.Assign(SwitchButton::StickR, src->input0x03.buttons.R3);
    }

}
