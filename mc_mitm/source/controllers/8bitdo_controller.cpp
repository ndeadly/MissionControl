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
#include "8bitdo_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    namespace {

        constexpr u8 TriggerMax = UINT8_MAX;

        constexpr u16 DpadStickPositions[] = { SwitchAnalogStick::MinimumValue, SwitchAnalogStick::CenterValue, SwitchAnalogStick::MaximumValue };

    }

    void EightBitDoController::ProcessInputData(const u8 *report_buffer, size_t size) {
        auto report = reinterpret_cast<const EightBitDoReportData *>(report_buffer);

        switch(report->id) {
            case 0x01:
                this->MapInputReport0x01(report); break;
            case 0x03:
                this->MapInputReport0x03(report, size == 11 ? EightBitDoReportFormat_ZeroV1 : EightBitDoReportFormat_ZeroV2); break;
            default:
                break;
        }
    }

    void EightBitDoController::MapInputReport0x01(const EightBitDoReportData *src) {
        SwitchButtons button_state = m_buttons;

        if (m_controller_type == EightBitDoControllerType_Zero) {
            bool dpad_down  = (src->input0x01_v1.dpad == EightBitDoDPadV1_S)  ||
                              (src->input0x01_v1.dpad == EightBitDoDPadV1_SE) ||
                              (src->input0x01_v1.dpad == EightBitDoDPadV1_SW);
            bool dpad_up    = (src->input0x01_v1.dpad == EightBitDoDPadV1_N)  ||
                              (src->input0x01_v1.dpad == EightBitDoDPadV1_NE) ||
                              (src->input0x01_v1.dpad == EightBitDoDPadV1_NW);
            bool dpad_right = (src->input0x01_v1.dpad == EightBitDoDPadV1_E)  ||
                              (src->input0x01_v1.dpad == EightBitDoDPadV1_NE) ||
                              (src->input0x01_v1.dpad == EightBitDoDPadV1_SE);
            bool dpad_left  = (src->input0x01_v1.dpad == EightBitDoDPadV1_W)  ||
                              (src->input0x01_v1.dpad == EightBitDoDPadV1_NW) ||
                              (src->input0x01_v1.dpad == EightBitDoDPadV1_SW);

            // Map dpad as left stick
            m_left_stick.SetValues(
                DpadStickPositions[1 + dpad_right - dpad_left],
                DpadStickPositions[1 + dpad_up - dpad_down]
            );
        } else {
            m_left_stick.SetValuesFrom(
                src->input0x01_v2.left_stick.GetX(),
                src->input0x01_v2.left_stick.GetYInverted()
            );

            m_right_stick.SetValuesFrom(
                src->input0x01_v2.right_stick.GetX(),
                src->input0x01_v2.right_stick.GetYInverted()
            );

            button_state.Assign(SwitchButton::Down,  (src->input0x01_v2.dpad == EightBitDoDPadV2_S) || (src->input0x01_v2.dpad == EightBitDoDPadV2_SE) || (src->input0x01_v2.dpad == EightBitDoDPadV2_SW));
            button_state.Assign(SwitchButton::Up,    (src->input0x01_v2.dpad == EightBitDoDPadV2_N) || (src->input0x01_v2.dpad == EightBitDoDPadV2_NE) || (src->input0x01_v2.dpad == EightBitDoDPadV2_NW));
            button_state.Assign(SwitchButton::Right, (src->input0x01_v2.dpad == EightBitDoDPadV2_E) || (src->input0x01_v2.dpad == EightBitDoDPadV2_NE) || (src->input0x01_v2.dpad == EightBitDoDPadV2_SE));
            button_state.Assign(SwitchButton::Left,  (src->input0x01_v2.dpad == EightBitDoDPadV2_W) || (src->input0x01_v2.dpad == EightBitDoDPadV2_NW) || (src->input0x01_v2.dpad == EightBitDoDPadV2_SW));
            button_state.Assign(SwitchButton::A,     src->input0x01_v2.buttons.B);
            button_state.Assign(SwitchButton::B,     src->input0x01_v2.buttons.A);
            button_state.Assign(SwitchButton::X,     src->input0x01_v2.buttons.Y);
            button_state.Assign(SwitchButton::Y,     src->input0x01_v2.buttons.X);
            button_state.Assign(SwitchButton::L,     src->input0x01_v2.buttons.L1);
            button_state.Assign(SwitchButton::R,     src->input0x01_v2.buttons.R1);
            button_state.Assign(SwitchButton::ZL,    src->input0x01_v2.left_trigger  > (m_trigger_threshold * TriggerMax));
            button_state.Assign(SwitchButton::ZR,    src->input0x01_v2.right_trigger > (m_trigger_threshold * TriggerMax));

            if (m_controller_type == EightBitDoControllerType_Sn30ProXboxCloud) {
                button_state.Assign(SwitchButton::Minus,  src->input0x01_v2.buttons.v1.select);
                button_state.Assign(SwitchButton::Plus,   src->input0x01_v2.buttons.v1.start);
                button_state.Assign(SwitchButton::StickL, src->input0x01_v2.buttons.v1.L3);
                button_state.Assign(SwitchButton::StickR, src->input0x01_v2.buttons.v1.R3);
                button_state.Assign(SwitchButton::Home,   src->input0x01_v2.buttons.v1.home);
            } else {
                button_state.Assign(SwitchButton::Minus,  src->input0x01_v2.buttons.v2.select);
                button_state.Assign(SwitchButton::Plus,   src->input0x01_v2.buttons.v2.start);
                button_state.Assign(SwitchButton::StickL, src->input0x01_v2.buttons.v2.L3);
                button_state.Assign(SwitchButton::StickR, src->input0x01_v2.buttons.v2.R3);
                button_state.Assign(SwitchButton::Home,   src->input0x01_v2.buttons.v2.home);
            }
        }

        m_buttons = button_state;
    }

    void EightBitDoController::MapInputReport0x03(const EightBitDoReportData *src, EightBitDoReportFormat fmt) {
        SwitchButtons button_state = m_buttons;

        if (m_controller_type == EightBitDoControllerType_Zero) {
            if (fmt == EightBitDoReportFormat_ZeroV1) {
                button_state.Assign(SwitchButton::A,     src->input0x03_v1.buttons.B);
                button_state.Assign(SwitchButton::B,     src->input0x03_v1.buttons.A);
                button_state.Assign(SwitchButton::X,     src->input0x03_v1.buttons.Y);
                button_state.Assign(SwitchButton::Y,     src->input0x03_v1.buttons.X);
                button_state.Assign(SwitchButton::R,     src->input0x03_v1.buttons.R1);
                button_state.Assign(SwitchButton::L,     src->input0x03_v1.buttons.L1);
                button_state.Assign(SwitchButton::Minus, src->input0x03_v1.buttons.v2.select);
                button_state.Assign(SwitchButton::Plus,  src->input0x03_v1.buttons.v2.start);
            } else if (fmt == EightBitDoReportFormat_ZeroV2) {
                bool dpad_down  = src->input0x03_v2.left_stick.y == 0xff;
                bool dpad_up    = src->input0x03_v2.left_stick.y == 0x00;
                bool dpad_right = src->input0x03_v2.left_stick.x == 0xff;
                bool dpad_left  = src->input0x03_v2.left_stick.x == 0x00;

                // Map dpad as left stick
                m_left_stick.SetValues(
                    DpadStickPositions[1 + dpad_right - dpad_left],
                    DpadStickPositions[1 + dpad_up - dpad_down]
                );

                button_state.Assign(SwitchButton::A,     src->input0x03_v2.buttons.B);
                button_state.Assign(SwitchButton::B,     src->input0x03_v2.buttons.A);
                button_state.Assign(SwitchButton::X,     src->input0x03_v2.buttons.Y);
                button_state.Assign(SwitchButton::Y,     src->input0x03_v2.buttons.X);
                button_state.Assign(SwitchButton::R,     src->input0x03_v2.buttons.R1);
                button_state.Assign(SwitchButton::L,     src->input0x03_v2.buttons.L1);
                button_state.Assign(SwitchButton::Minus, src->input0x03_v2.buttons.v2.select);
                button_state.Assign(SwitchButton::Plus,  src->input0x03_v2.buttons.v2.start);
            }
        } else {
            m_left_stick.SetValuesFrom(
                src->input0x03_v3.left_stick.GetX(),
                src->input0x03_v3.left_stick.GetYInverted()
            );

            m_right_stick.SetValuesFrom(
                src->input0x03_v3.right_stick.GetX(),
                src->input0x03_v3.right_stick.GetYInverted()
            );

            button_state.Assign(SwitchButton::Down,  (src->input0x03_v3.dpad == EightBitDoDPadV2_S) || (src->input0x03_v3.dpad == EightBitDoDPadV2_SE) || (src->input0x03_v3.dpad == EightBitDoDPadV2_SW));
            button_state.Assign(SwitchButton::Up,    (src->input0x03_v3.dpad == EightBitDoDPadV2_N) || (src->input0x03_v3.dpad == EightBitDoDPadV2_NE) || (src->input0x03_v3.dpad == EightBitDoDPadV2_NW));
            button_state.Assign(SwitchButton::Right, (src->input0x03_v3.dpad == EightBitDoDPadV2_E) || (src->input0x03_v3.dpad == EightBitDoDPadV2_NE) || (src->input0x03_v3.dpad == EightBitDoDPadV2_SE));
            button_state.Assign(SwitchButton::Left,  (src->input0x03_v3.dpad == EightBitDoDPadV2_W) || (src->input0x03_v3.dpad == EightBitDoDPadV2_NW) || (src->input0x03_v3.dpad == EightBitDoDPadV2_SW));
            button_state.Assign(SwitchButton::A,      src->input0x03_v3.buttons.B);
            button_state.Assign(SwitchButton::B,      src->input0x03_v3.buttons.A);
            button_state.Assign(SwitchButton::X,      src->input0x03_v3.buttons.Y);
            button_state.Assign(SwitchButton::Y,      src->input0x03_v3.buttons.X);
            button_state.Assign(SwitchButton::L,      src->input0x03_v3.buttons.L1);
            button_state.Assign(SwitchButton::R,      src->input0x03_v3.buttons.R1);
            button_state.Assign(SwitchButton::ZL,     src->input0x03_v3.left_trigger  > (m_trigger_threshold * TriggerMax));
            button_state.Assign(SwitchButton::ZR,     src->input0x03_v3.right_trigger > (m_trigger_threshold * TriggerMax));
            button_state.Assign(SwitchButton::Minus,  src->input0x03_v3.buttons.v2.select);
            button_state.Assign(SwitchButton::Plus,   src->input0x03_v3.buttons.v2.start);
            button_state.Assign(SwitchButton::StickL, src->input0x03_v3.buttons.v2.L3);
            button_state.Assign(SwitchButton::StickR, src->input0x03_v3.buttons.v2.R3);
            button_state.Assign(SwitchButton::Home,   src->input0x03_v3.buttons.v2.home);

            auto battery_level = SwitchBatteryLevelConverter::ConvertPercentage(src->input0x03_v3.battery);
            m_power_info.SetBatteryLevel(battery_level);
        }

        m_buttons = button_state;
    }

}
