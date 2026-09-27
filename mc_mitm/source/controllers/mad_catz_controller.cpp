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
#include "mad_catz_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    namespace {

        constexpr u8 TriggerMax = UINT8_MAX;
        constexpr float MediaModeStickScaleFactor = float(SwitchAnalogStick::MaximumValue) / 39;

    }

    void MadCatzController::ProcessInputData(const u8 *report_buffer, size_t size) {
        AMS_UNUSED(size);
        auto report = reinterpret_cast<const MadCatzReportData *>(report_buffer);

        switch(report->id) {
            case 0x01:
                this->MapInputReport0x01(report); break;
            case 0x02:
                this->MapInputReport0x02(report); break;
            case 0x81:
                this->MapInputReport0x81(report); break;
            case 0x82:
                this->MapInputReport0x82(report); break;
            case 0x83:
                this->MapInputReport0x83(report); break;
            default:
                break;
        }
    }

    void MadCatzController::MapInputReport0x01(const MadCatzReportData *src) {
        auto dpad = DirectionalPad(src->input0x01.dpad);

        m_left_stick.SetValuesFrom(
            src->input0x01.left_stick.GetX(),
            src->input0x01.left_stick.GetYInverted()
        );

        m_right_stick.SetValuesFrom(
            src->input0x01.right_stick.GetX(),
            src->input0x01.right_stick.GetYInverted()
        );

        SwitchButtons button_state = m_buttons;
        button_state.Assign(SwitchButton::Down,   dpad.IsDown());
        button_state.Assign(SwitchButton::Up,     dpad.IsUp());
        button_state.Assign(SwitchButton::Right,  dpad.IsRight());
        button_state.Assign(SwitchButton::Left,   dpad.IsLeft());
        button_state.Assign(SwitchButton::A,      src->input0x01.buttons.B);
        button_state.Assign(SwitchButton::B,      src->input0x01.buttons.A);
        button_state.Assign(SwitchButton::X,      src->input0x01.buttons.Y);
        button_state.Assign(SwitchButton::Y,      src->input0x01.buttons.X);
        button_state.Assign(SwitchButton::R,      src->input0x01.buttons.R1);
        button_state.Assign(SwitchButton::L,      src->input0x01.buttons.L1);
        button_state.Assign(SwitchButton::ZR,     src->input0x01.right_trigger > (m_trigger_threshold * TriggerMax));
        button_state.Assign(SwitchButton::ZL,     src->input0x01.left_trigger  > (m_trigger_threshold * TriggerMax));
        button_state.Assign(SwitchButton::Minus,  src->input0x01.buttons.select);
        button_state.Assign(SwitchButton::Plus,   src->input0x01.buttons.start);
        button_state.Assign(SwitchButton::StickL, src->input0x01.buttons.L3);
        button_state.Assign(SwitchButton::StickR, src->input0x01.buttons.R3);
        // button_state.Assign(SwitchButton::Home,   src->input0x01.buttons.home);
        m_buttons = button_state;
    }

    void MadCatzController::MapInputReport0x02(const MadCatzReportData *src) {
        m_buttons.Assign(SwitchButton::Home, src->input0x02.play);
    }

    void MadCatzController::MapInputReport0x81(const MadCatzReportData *src) {
        auto dpad = DirectionalPad(src->input0x81.dpad);

        m_left_stick.SetValuesFrom(
            src->input0x81.left_stick.GetX(),
            src->input0x81.left_stick.GetYInverted()
        );

        m_right_stick.SetValuesFrom(
            src->input0x81.right_stick.GetX(),
            src->input0x81.right_stick.GetYInverted()
        );

        SwitchButtons button_state = m_buttons;
        button_state.Assign(SwitchButton::Down,   dpad.IsDown());
        button_state.Assign(SwitchButton::Up,     dpad.IsUp());
        button_state.Assign(SwitchButton::Right,  dpad.IsRight());
        button_state.Assign(SwitchButton::Left,   dpad.IsLeft());
        button_state.Assign(SwitchButton::A,      src->input0x81.buttons.B);
        button_state.Assign(SwitchButton::B,      src->input0x81.buttons.A);
        button_state.Assign(SwitchButton::X,      src->input0x81.buttons.Y);
        button_state.Assign(SwitchButton::Y,      src->input0x81.buttons.X);
        button_state.Assign(SwitchButton::R,      src->input0x81.buttons.R1);
        button_state.Assign(SwitchButton::L,      src->input0x81.buttons.L1);
        button_state.Assign(SwitchButton::ZR,     src->input0x81.right_trigger > (m_trigger_threshold * TriggerMax));
        button_state.Assign(SwitchButton::ZL,     src->input0x81.left_trigger  > (m_trigger_threshold * TriggerMax));
        button_state.Assign(SwitchButton::Minus,  src->input0x81.buttons.select);
        button_state.Assign(SwitchButton::Plus,   src->input0x81.buttons.start);
        button_state.Assign(SwitchButton::StickL, src->input0x81.buttons.L3);
        button_state.Assign(SwitchButton::StickR, src->input0x81.buttons.R3);
        m_buttons = button_state;
    }

    void MadCatzController::MapInputReport0x82(const MadCatzReportData *src) {
        SwitchButtons button_state = m_buttons;
        button_state.Assign(SwitchButton::Up,    src->input0x82.buttons.dpad_up);
        button_state.Assign(SwitchButton::Down,  src->input0x82.buttons.dpad_down);
        button_state.Assign(SwitchButton::Left,  src->input0x82.buttons.dpad_left);
        button_state.Assign(SwitchButton::Right, src->input0x82.buttons.dpad_right);
        button_state.Assign(SwitchButton::A,     src->input0x82.buttons.B);
        button_state.Assign(SwitchButton::X,     src->input0x82.buttons.Y);
        button_state.Assign(SwitchButton::Y,     src->input0x82.buttons.X);
        button_state.Assign(SwitchButton::R,     src->input0x82.buttons.R1);
        button_state.Assign(SwitchButton::L,     src->input0x82.buttons.L1);
        button_state.Assign(SwitchButton::Minus, src->input0x82.buttons.select);
        m_buttons = button_state;
    }

    void MadCatzController::MapInputReport0x83(const MadCatzReportData *src) {
        m_left_stick.SetValues(
            std::clamp<u16>(MediaModeStickScaleFactor * -src->input0x83.left_stick.x + 0x7ff, SwitchAnalogStick::MinimumValue, SwitchAnalogStick::MaximumValue),
            std::clamp<u16>(MediaModeStickScaleFactor *  src->input0x83.left_stick.y + 0x7ff, SwitchAnalogStick::MinimumValue, SwitchAnalogStick::MaximumValue)
        );

        SwitchButtons button_state = m_buttons;
        button_state.Assign(SwitchButton::ZR,     src->input0x83.buttons.R2);
        button_state.Assign(SwitchButton::ZL,     src->input0x83.buttons.L2);
        button_state.Assign(SwitchButton::StickL, src->input0x83.buttons.L3);
        button_state.Assign(SwitchButton::StickR, src->input0x83.buttons.R3);
        m_buttons = button_state;
    }

}
