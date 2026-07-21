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
#include "gamesir_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    namespace {

        constexpr u8 TriggerMax = UINT8_MAX;

    }

    void GamesirController::ProcessInputData(const bluetooth::HidReport *report) {
        auto gamesir_report = reinterpret_cast<const GamesirReportData *>(&report->data);

        switch(gamesir_report->id) {
            case 0x03:
                this->MapInputReport0x03(gamesir_report); break;
            case 0x07:
                this->MapInputReport0x07(gamesir_report); break;
            case 0x12:
                this->MapInputReport0x12(gamesir_report); break;
            case 0xc4:
                this->MapInputReport0xc4(gamesir_report); break;
            default:
                break;
        }
    }

    void GamesirController::MapInputReport0x03(const GamesirReportData *src) {
        m_left_stick.SetValuesFrom(
            src->input0x03.left_stick.GetX(),
            src->input0x03.left_stick.GetYInverted()
        );

        m_right_stick.SetValuesFrom(
            src->input0x03.right_stick.GetX(),
            src->input0x03.right_stick.GetYInverted()
        );

        SwitchButtons button_state = m_buttons;
        button_state.Assign(SwitchButton::Down,   (src->input0x03.dpad == GamesirDpad2_S) || (src->input0x03.dpad == GamesirDpad2_SE) || (src->input0x03.dpad == GamesirDpad2_SW));
        button_state.Assign(SwitchButton::Up,     (src->input0x03.dpad == GamesirDpad2_N) || (src->input0x03.dpad == GamesirDpad2_NE) || (src->input0x03.dpad == GamesirDpad2_NW));
        button_state.Assign(SwitchButton::Right,  (src->input0x03.dpad == GamesirDpad2_E) || (src->input0x03.dpad == GamesirDpad2_NE) || (src->input0x03.dpad == GamesirDpad2_SE));
        button_state.Assign(SwitchButton::Left,   (src->input0x03.dpad == GamesirDpad2_W) || (src->input0x03.dpad == GamesirDpad2_NW) || (src->input0x03.dpad == GamesirDpad2_SW));
        button_state.Assign(SwitchButton::A,      src->input0x03.buttons.B);
        button_state.Assign(SwitchButton::B,      src->input0x03.buttons.A);
        button_state.Assign(SwitchButton::X,      src->input0x03.buttons.Y);
        button_state.Assign(SwitchButton::Y,      src->input0x03.buttons.X);
        button_state.Assign(SwitchButton::R,      src->input0x03.buttons.RB);
        button_state.Assign(SwitchButton::L,      src->input0x03.buttons.LB);
        button_state.Assign(SwitchButton::ZR,     src->input0x03.right_trigger > (m_trigger_threshold * TriggerMax));
        button_state.Assign(SwitchButton::ZL,     src->input0x03.left_trigger  > (m_trigger_threshold * TriggerMax));
        button_state.Assign(SwitchButton::Minus,  src->input0x03.buttons.select);
        button_state.Assign(SwitchButton::Plus,   src->input0x03.buttons.start);
        button_state.Assign(SwitchButton::StickL, src->input0x03.buttons.L3);
        button_state.Assign(SwitchButton::StickR, src->input0x03.buttons.R3);
        button_state.Assign(SwitchButton::Home,   src->input0x03.buttons.home);
        m_buttons = button_state;
    }

    void GamesirController::MapInputReport0x07(const GamesirReportData *src) {
        m_left_stick.SetValuesFrom(
            src->input0x07.left_stick.GetX(),
            src->input0x07.left_stick.GetYInverted()
        );

        m_right_stick.SetValuesFrom(
            src->input0x07.right_stick.GetX(),
            src->input0x07.right_stick.GetYInverted()
        );

        SwitchButtons button_state = m_buttons;
        button_state.Assign(SwitchButton::Down,   (src->input0x07.dpad == GamesirDpad2_S) || (src->input0x07.dpad == GamesirDpad2_SE) || (src->input0x07.dpad == GamesirDpad2_SW));
        button_state.Assign(SwitchButton::Up,     (src->input0x07.dpad == GamesirDpad2_N) || (src->input0x07.dpad == GamesirDpad2_NE) || (src->input0x07.dpad == GamesirDpad2_NW));
        button_state.Assign(SwitchButton::Right,  (src->input0x07.dpad == GamesirDpad2_E) || (src->input0x07.dpad == GamesirDpad2_NE) || (src->input0x07.dpad == GamesirDpad2_SE));
        button_state.Assign(SwitchButton::Left,   (src->input0x07.dpad == GamesirDpad2_W) || (src->input0x07.dpad == GamesirDpad2_NW) || (src->input0x07.dpad == GamesirDpad2_SW));
        button_state.Assign(SwitchButton::A,      src->input0x07.buttons.B);
        button_state.Assign(SwitchButton::B,      src->input0x07.buttons.A);
        button_state.Assign(SwitchButton::X,      src->input0x07.buttons.Y);
        button_state.Assign(SwitchButton::Y,      src->input0x07.buttons.X);
        button_state.Assign(SwitchButton::R,      src->input0x07.buttons.RB);
        button_state.Assign(SwitchButton::L,      src->input0x07.buttons.LB);
        button_state.Assign(SwitchButton::ZR,     src->input0x07.right_trigger > (m_trigger_threshold * TriggerMax));
        button_state.Assign(SwitchButton::ZL,     src->input0x07.left_trigger  > (m_trigger_threshold * TriggerMax));
        button_state.Assign(SwitchButton::Minus,  src->input0x07.buttons.select);
        button_state.Assign(SwitchButton::Plus,   src->input0x07.buttons.start);
        button_state.Assign(SwitchButton::StickL, src->input0x07.buttons.L3);
        button_state.Assign(SwitchButton::StickR, src->input0x07.buttons.R3);
        button_state.Assign(SwitchButton::Home,   src->input0x07.buttons.home);
        m_buttons = button_state;
    }

    void GamesirController::MapInputReport0x12(const GamesirReportData *src) {
        m_buttons.Assign(SwitchButton::Home, src->input0x12.home);
    }

    void GamesirController::MapInputReport0xc4(const GamesirReportData *src) {
        m_left_stick.SetValuesFrom(
            src->input0xc4.left_stick.GetX(),
            src->input0xc4.left_stick.GetYInverted()
        );

        m_right_stick.SetValuesFrom(
            src->input0xc4.right_stick.GetX(),
            src->input0xc4.right_stick.GetYInverted()
        );

        SwitchButtons button_state = m_buttons;
        button_state.Assign(SwitchButton::Down,   (src->input0xc4.dpad == GamesirDpad_S) || (src->input0xc4.dpad == GamesirDpad_SE) || (src->input0xc4.dpad == GamesirDpad_SW));
        button_state.Assign(SwitchButton::Up,     (src->input0xc4.dpad == GamesirDpad_N) || (src->input0xc4.dpad == GamesirDpad_NE) || (src->input0xc4.dpad == GamesirDpad_NW));
        button_state.Assign(SwitchButton::Right,  (src->input0xc4.dpad == GamesirDpad_E) || (src->input0xc4.dpad == GamesirDpad_NE) || (src->input0xc4.dpad == GamesirDpad_SE));
        button_state.Assign(SwitchButton::Left,   (src->input0xc4.dpad == GamesirDpad_W) || (src->input0xc4.dpad == GamesirDpad_NW) || (src->input0xc4.dpad == GamesirDpad_SW));
        button_state.Assign(SwitchButton::A,      src->input0xc4.buttons.B);
        button_state.Assign(SwitchButton::B,      src->input0xc4.buttons.A);
        button_state.Assign(SwitchButton::X,      src->input0xc4.buttons.Y);
        button_state.Assign(SwitchButton::Y,      src->input0xc4.buttons.X);
        button_state.Assign(SwitchButton::R,      src->input0xc4.buttons.RB);
        button_state.Assign(SwitchButton::L,      src->input0xc4.buttons.LB);
        button_state.Assign(SwitchButton::ZR,     src->input0xc4.right_trigger > (m_trigger_threshold * TriggerMax));
        button_state.Assign(SwitchButton::ZL,     src->input0xc4.left_trigger  > (m_trigger_threshold * TriggerMax));
        button_state.Assign(SwitchButton::Minus,  src->input0xc4.buttons.select);
        button_state.Assign(SwitchButton::Plus,   src->input0xc4.buttons.start);
        button_state.Assign(SwitchButton::StickL, src->input0xc4.buttons.L3);
        button_state.Assign(SwitchButton::StickR, src->input0xc4.buttons.R3);
        m_buttons = button_state;
    }

}
