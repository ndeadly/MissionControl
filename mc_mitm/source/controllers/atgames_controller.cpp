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
#include "atgames_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    namespace {

        constexpr float StickScaleFactor = float(SwitchAnalogStick::MaximumValue) / UINT8_MAX;

    }

    void AtGamesController::ProcessInputData(const u8 *report_buffer, size_t size) {
        AMS_UNUSED(size);
        auto report = reinterpret_cast<const AtGamesReportData *>(report_buffer);

        switch(report->id) {
            case 0x01:
                this->MapInputReport0x01(report); break;
            default:
                break;
        }
    }

    void AtGamesController::MapInputReport0x01(const AtGamesReportData *src) {
        SwitchButtons button_state = m_buttons;

        auto dpad = DirectionalPad(src->input0x01.dpad);

        if (!m_arcadepanel) {
            // Checking if any of the optional Arcade Control Panel buttons are pressed and and switching the mapping
            if (src->input0x01.a_button || src->input0x01.b_button || src->input0x01.c_button || src->input0x01.x_button || src->input0x01.y_button || src->input0x01.z_button) {
                m_arcadepanel = true;
            }

            m_left_stick.SetValues(
                SwitchAnalogStick::CenterValue + 0x7ff * (src->input0x01.nudge_left - src->input0x01.nudge_right),
                SwitchAnalogStick::CenterValue
            );

            m_right_stick.SetValues(
                SwitchAnalogStick::CenterValue,
                static_cast<u16>(StickScaleFactor * (UINT8_MAX - src->input0x01.right_stick.x)) & SwitchAnalogStick::MaximumValue
            );
            
            button_state.Assign(SwitchButton::Down,  dpad.IsDown());
            button_state.Assign(SwitchButton::Up,    dpad.IsUp());
            button_state.Assign(SwitchButton::Right, dpad.IsRight());
            button_state.Assign(SwitchButton::Left,  dpad.IsLeft());
            button_state.Assign(SwitchButton::A,     src->input0x01.play);
            button_state.Assign(SwitchButton::B,     src->input0x01.rewind);
            button_state.Assign(SwitchButton::Y,     src->input0x01.nudge_front);
            button_state.Assign(SwitchButton::R,     src->input0x01.flipper_right);
            button_state.Assign(SwitchButton::L,     src->input0x01.flipper_left);
            button_state.Assign(SwitchButton::ZR,    src->input0x01.flipper_right);
            button_state.Assign(SwitchButton::ZL,    src->input0x01.flipper_left);
            button_state.Assign(SwitchButton::Plus,  src->input0x01.home_twirl);
        } else {
            m_left_stick.SetValues(
                SwitchAnalogStick::CenterValue + 0x7ff * (src->input0x01.nudge_left - src->input0x01.nudge_right),
                SwitchAnalogStick::CenterValue + 0x7ff * (src->input0x01.nudge_front)
            );

            m_right_stick.SetValues(
                SwitchAnalogStick::CenterValue,
                static_cast<u16>(StickScaleFactor * (UINT8_MAX - src->input0x01.right_stick.x)) & SwitchAnalogStick::MaximumValue
            );

            button_state.Assign(SwitchButton::Down,  dpad.IsDown());
            button_state.Assign(SwitchButton::Up,    dpad.IsUp());
            button_state.Assign(SwitchButton::Right, dpad.IsRight());
            button_state.Assign(SwitchButton::Left,  dpad.IsLeft());
            button_state.Assign(SwitchButton::A,     src->input0x01.a_button);
            button_state.Assign(SwitchButton::B,     src->input0x01.b_button);
            button_state.Assign(SwitchButton::X,     src->input0x01.x_button);
            button_state.Assign(SwitchButton::Y,     src->input0x01.y_button);
            button_state.Assign(SwitchButton::R,     src->input0x01.c_button);
            button_state.Assign(SwitchButton::L,     src->input0x01.z_button);
            button_state.Assign(SwitchButton::ZR,    src->input0x01.flipper_right);
            button_state.Assign(SwitchButton::ZL,    src->input0x01.flipper_left);
            button_state.Assign(SwitchButton::Minus, src->input0x01.rewind);
            button_state.Assign(SwitchButton::Plus,  src->input0x01.play);
            button_state.Assign(SwitchButton::Home,  src->input0x01.home_twirl);
        }

        m_buttons = button_state;
    }

}
