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
#include "steelseries_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    namespace {

        constexpr u8 TriggerMax = UINT8_MAX;
        constexpr u16 FixedTriggerThreshold = 0x7ff;

    }

    void SteelseriesController::ProcessInputData(const u8 *report_buffer, size_t size) {
        auto report = reinterpret_cast<const SteelseriesReportData *>(report_buffer);

        switch(report->id) {
            case 0x01:
                if (size == sizeof(SteelseriesInputReport0x01_v2) + 1) {
                    this->MapInputReport0x01_v2(report);
                } else {
                    this->MapInputReport0x01(report);
                }
                break;
            case 0x02:
                this->MapInputReport0x02(report); break;
            case 0x12:
                this->MapInputReport0x12(report); break;
            case 0xC4:
                this->MapInputReport0xC4(report); break;
            default:
                // Todo: handle this properly
                this->MapMfiInputReport(report);
                break;
        }
    }

    void SteelseriesController::MapInputReport0x01(const SteelseriesReportData *src) {
        m_left_stick.SetValuesFrom(
            src->input0x01.left_stick.GetX(),
            src->input0x01.left_stick.GetYInverted()
        );

        m_right_stick.SetValuesFrom(
            src->input0x01.right_stick.GetX(),
            src->input0x01.right_stick.GetYInverted()
        );

        SwitchButtons button_state = m_buttons;
        button_state.Assign(SwitchButton::Down,  (src->input0x01.dpad == SteelseriesDPad_S) || (src->input0x01.dpad == SteelseriesDPad_SE) || (src->input0x01.dpad == SteelseriesDPad_SW));
        button_state.Assign(SwitchButton::Up,    (src->input0x01.dpad == SteelseriesDPad_N) || (src->input0x01.dpad == SteelseriesDPad_NE) || (src->input0x01.dpad == SteelseriesDPad_NW));
        button_state.Assign(SwitchButton::Right, (src->input0x01.dpad == SteelseriesDPad_E) || (src->input0x01.dpad == SteelseriesDPad_NE) || (src->input0x01.dpad == SteelseriesDPad_SE));
        button_state.Assign(SwitchButton::Left,  (src->input0x01.dpad == SteelseriesDPad_W) || (src->input0x01.dpad == SteelseriesDPad_NW) || (src->input0x01.dpad == SteelseriesDPad_SW));
        button_state.Assign(SwitchButton::A,     src->input0x01.buttons.B);
        button_state.Assign(SwitchButton::B,     src->input0x01.buttons.A);
        button_state.Assign(SwitchButton::X,     src->input0x01.buttons.Y);
        button_state.Assign(SwitchButton::Y,     src->input0x01.buttons.X);
        button_state.Assign(SwitchButton::R,     src->input0x01.buttons.R1);
        button_state.Assign(SwitchButton::L,     src->input0x01.buttons.L1);
        button_state.Assign(SwitchButton::Minus, src->input0x01.buttons.select);
        button_state.Assign(SwitchButton::Plus,  src->input0x01.buttons.start);
        m_buttons = button_state;
    }

    void SteelseriesController::MapInputReport0x01_v2(const SteelseriesReportData *src) {
        m_left_stick.SetValuesFrom(
            src->input0x01_v2.left_stick.GetX(),
            src->input0x01_v2.left_stick.GetYInverted()
        );

        m_right_stick.SetValuesFrom(
            src->input0x01_v2.right_stick.GetX(),
            src->input0x01_v2.right_stick.GetYInverted()
        );

        SwitchButtons button_state = m_buttons;
        button_state.Assign(SwitchButton::Down,   (src->input0x01_v2.dpad == SteelseriesDPad_S) || (src->input0x01_v2.dpad == SteelseriesDPad_SE) || (src->input0x01_v2.dpad == SteelseriesDPad_SW));
        button_state.Assign(SwitchButton::Up,     (src->input0x01_v2.dpad == SteelseriesDPad_N) || (src->input0x01_v2.dpad == SteelseriesDPad_NE) || (src->input0x01_v2.dpad == SteelseriesDPad_NW));
        button_state.Assign(SwitchButton::Right,  (src->input0x01_v2.dpad == SteelseriesDPad_E) || (src->input0x01_v2.dpad == SteelseriesDPad_NE) || (src->input0x01_v2.dpad == SteelseriesDPad_SE));
        button_state.Assign(SwitchButton::Left,   (src->input0x01_v2.dpad == SteelseriesDPad_W) || (src->input0x01_v2.dpad == SteelseriesDPad_NW) || (src->input0x01_v2.dpad == SteelseriesDPad_SW));
        button_state.Assign(SwitchButton::A,      src->input0x01_v2.buttons.B);
        button_state.Assign(SwitchButton::B,      src->input0x01_v2.buttons.A);
        button_state.Assign(SwitchButton::X,      src->input0x01_v2.buttons.Y);
        button_state.Assign(SwitchButton::Y,      src->input0x01_v2.buttons.X);
        button_state.Assign(SwitchButton::R,      src->input0x01_v2.buttons.R1);
        button_state.Assign(SwitchButton::L,      src->input0x01_v2.buttons.L1);
        button_state.Assign(SwitchButton::ZR,     src->input0x01_v2.right_trigger > FixedTriggerThreshold);
        button_state.Assign(SwitchButton::ZL,     src->input0x01_v2.left_trigger  > FixedTriggerThreshold);
        button_state.Assign(SwitchButton::StickR, src->input0x01_v2.buttons.R3);
        button_state.Assign(SwitchButton::StickL, src->input0x01_v2.buttons.L3);
        button_state.Assign(SwitchButton::Plus,   src->input0x01_v2.buttons.start);
        m_buttons = button_state;
    }

    void SteelseriesController::MapInputReport0x02(const SteelseriesReportData *src) {
        SwitchButtons button_state = m_buttons;
        button_state.Assign(SwitchButton::Minus, src->input0x02.select);
        button_state.Assign(SwitchButton::Home,  src->input0x02.home);
        m_buttons = button_state;
    }

    void SteelseriesController::MapInputReport0x12(const SteelseriesReportData *src) {
        m_buttons.Assign(SwitchButton::Home, src->input0x12.home);
    }

    void SteelseriesController::MapInputReport0xC4(const SteelseriesReportData *src) {
        m_left_stick.SetValuesFrom(
            src->input0xC4.left_stick.GetX(),
            src->input0xC4.left_stick.GetYInverted()
        );

        m_right_stick.SetValuesFrom(
            src->input0xC4.right_stick.GetX(),
            src->input0xC4.right_stick.GetYInverted()
        );

        SwitchButtons button_state = m_buttons;
        button_state.Assign(SwitchButton::Down,   (src->input0xC4.dpad == SteelseriesDPad2_S) || (src->input0xC4.dpad == SteelseriesDPad2_SE) || (src->input0xC4.dpad == SteelseriesDPad2_SW));
        button_state.Assign(SwitchButton::Up,     (src->input0xC4.dpad == SteelseriesDPad2_N) || (src->input0xC4.dpad == SteelseriesDPad2_NE) || (src->input0xC4.dpad == SteelseriesDPad2_NW));
        button_state.Assign(SwitchButton::Right,  (src->input0xC4.dpad == SteelseriesDPad2_E) || (src->input0xC4.dpad == SteelseriesDPad2_NE) || (src->input0xC4.dpad == SteelseriesDPad2_SE));
        button_state.Assign(SwitchButton::Left,   (src->input0xC4.dpad == SteelseriesDPad2_W) || (src->input0xC4.dpad == SteelseriesDPad2_NW) || (src->input0xC4.dpad == SteelseriesDPad2_SW));
        button_state.Assign(SwitchButton::A,      src->input0xC4.buttons.B);
        button_state.Assign(SwitchButton::B,      src->input0xC4.buttons.A);
        button_state.Assign(SwitchButton::X,      src->input0xC4.buttons.Y);
        button_state.Assign(SwitchButton::Y,      src->input0xC4.buttons.X);
        button_state.Assign(SwitchButton::R,      src->input0xC4.buttons.R1);
        button_state.Assign(SwitchButton::L,      src->input0xC4.buttons.L1);
        button_state.Assign(SwitchButton::ZR,     src->input0xC4.right_trigger > (m_trigger_threshold * TriggerMax));
        button_state.Assign(SwitchButton::ZL,     src->input0xC4.left_trigger  > (m_trigger_threshold * TriggerMax));
        button_state.Assign(SwitchButton::StickL, src->input0xC4.buttons.L3);
        button_state.Assign(SwitchButton::StickR, src->input0xC4.buttons.R3);
        button_state.Assign(SwitchButton::Minus,  src->input0xC4.buttons.select);
        button_state.Assign(SwitchButton::Plus,   src->input0xC4.buttons.start);
        m_buttons = button_state;
    }

    void SteelseriesController::MapMfiInputReport(const SteelseriesReportData *src) {
        m_left_stick.SetValuesFrom(
            src->input_mfi.left_stick.GetX(),
            src->input_mfi.left_stick.GetY()
        );

        m_right_stick.SetValuesFrom(
            src->input_mfi.right_stick.GetX(),
            src->input_mfi.right_stick.GetY()
        );

        SwitchButtons button_state = m_buttons;
        button_state.Assign(SwitchButton::Up,    src->input_mfi.buttons.dpad_up    > 0);
        button_state.Assign(SwitchButton::Right, src->input_mfi.buttons.dpad_right > 0);
        button_state.Assign(SwitchButton::Down,  src->input_mfi.buttons.dpad_down  > 0);
        button_state.Assign(SwitchButton::Left,  src->input_mfi.buttons.dpad_left  > 0);
        button_state.Assign(SwitchButton::A,     src->input_mfi.buttons.A > 0);
        button_state.Assign(SwitchButton::B,     src->input_mfi.buttons.B > 0);
        button_state.Assign(SwitchButton::X,     src->input_mfi.buttons.X > 0);
        button_state.Assign(SwitchButton::Y,     src->input_mfi.buttons.Y > 0);
        button_state.Assign(SwitchButton::R,     src->input_mfi.buttons.R1 > 0);
        button_state.Assign(SwitchButton::L,     src->input_mfi.buttons.L1 > 0);
        button_state.Assign(SwitchButton::ZR,    src->input_mfi.buttons.R2 > 0);
        button_state.Assign(SwitchButton::ZL,    src->input_mfi.buttons.L2 > 0);
        button_state.Assign(SwitchButton::Home,  src->input_mfi.buttons.menu);
        m_buttons = button_state;
    }

}
