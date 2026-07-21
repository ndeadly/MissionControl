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
#include "gamestick_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    void GamestickController::ProcessInputData(const bluetooth::HidReport *report) {
        auto gamestick_report = reinterpret_cast<const GamestickReportData *>(&report->data);

        switch(gamestick_report->id) {
            case 0x01:
                this->MapInputReport0x01(gamestick_report); break;
            case 0x03:
                this->MapInputReport0x03(gamestick_report); break;
            default:
                break;
        }
    }

    void GamestickController::MapInputReport0x01(const GamestickReportData *src) {
        SwitchButtons button_state = m_buttons;
        button_state.Assign(SwitchButton::Minus, src->input0x01.buttons.back);
        button_state.Assign(SwitchButton::Home,  src->input0x01.buttons.home);
        m_buttons = button_state;
    }

    void GamestickController::MapInputReport0x03(const GamestickReportData *src) {
        m_left_stick.SetValuesFrom(
            src->input0x03.left_stick.GetX(),
            src->input0x03.left_stick.GetYInverted()
        );

        m_right_stick.SetValuesFrom(
            src->input0x03.right_stick.GetX(),
            src->input0x03.right_stick.GetYInverted()
        );

        SwitchButtons button_state = m_buttons;
        button_state.Assign(SwitchButton::Down,   (src->input0x03.dpad == GamestickDPad_S) || (src->input0x03.dpad == GamestickDPad_SE) || (src->input0x03.dpad == GamestickDPad_SW));
        button_state.Assign(SwitchButton::Up,     (src->input0x03.dpad == GamestickDPad_N) || (src->input0x03.dpad == GamestickDPad_NE) || (src->input0x03.dpad == GamestickDPad_NW));
        button_state.Assign(SwitchButton::Right,  (src->input0x03.dpad == GamestickDPad_E) || (src->input0x03.dpad == GamestickDPad_NE) || (src->input0x03.dpad == GamestickDPad_SE));
        button_state.Assign(SwitchButton::Left,   (src->input0x03.dpad == GamestickDPad_W) || (src->input0x03.dpad == GamestickDPad_NW) || (src->input0x03.dpad == GamestickDPad_SW));
        button_state.Assign(SwitchButton::A,      src->input0x03.buttons.B);
        button_state.Assign(SwitchButton::B,      src->input0x03.buttons.A);
        button_state.Assign(SwitchButton::X,      src->input0x03.buttons.Y);
        button_state.Assign(SwitchButton::Y,      src->input0x03.buttons.X);
        button_state.Assign(SwitchButton::L,      src->input0x03.buttons.L);
        button_state.Assign(SwitchButton::R,      src->input0x03.buttons.R);
        button_state.Assign(SwitchButton::Plus,   src->input0x03.buttons.start);
        button_state.Assign(SwitchButton::StickL, src->input0x03.buttons.lstick_press);
        button_state.Assign(SwitchButton::StickR, src->input0x03.buttons.rstick_press);

        if (button_state.Test(SwitchButton::Down)) {
            button_state.Assign(SwitchButton::ZL,   src->input0x03.buttons.L);
            button_state.Assign(SwitchButton::ZR,   src->input0x03.buttons.R);

            button_state.Assign(SwitchButton::Down, !button_state.Any(SwitchButton::ZL | SwitchButton::ZR));
            button_state.Assign(SwitchButton::L,    !button_state.Test(SwitchButton::ZL));
            button_state.Assign(SwitchButton::R,    !button_state.Test(SwitchButton::ZR));
        }

        m_buttons = button_state;
    }

}
