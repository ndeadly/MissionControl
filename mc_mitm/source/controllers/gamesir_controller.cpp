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

    void GamesirController::ParseInputReport(const u8 *report_buffer, size_t size) {
        AMS_UNUSED(size);
        auto report = reinterpret_cast<const GamesirReportData *>(report_buffer);

        switch(report->id) {
            case 0x03:
                this->MapInputReport0x03(report); break;
            case 0x07:
                this->MapInputReport0x07(report); break;
            case 0x12:
                this->MapInputReport0x12(report); break;
            case 0xC4:
                this->MapInputReport0xC4(report); break;
            default:
                break;
        }
    }

    void GamesirController::MapInputReport0x03(const GamesirReportData *src) {
        auto dpad = DirectionalPad(src->input0x03.dpad);

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
        auto dpad = DirectionalPad(src->input0x07.dpad);

        m_left_stick.SetValuesFrom(
            src->input0x07.left_stick.GetX(),
            src->input0x07.left_stick.GetYInverted()
        );

        m_right_stick.SetValuesFrom(
            src->input0x07.right_stick.GetX(),
            src->input0x07.right_stick.GetYInverted()
        );

        SwitchButtons button_state = m_buttons;
        button_state.Assign(SwitchButton::Down,   dpad.IsDown());
        button_state.Assign(SwitchButton::Up,     dpad.IsUp());
        button_state.Assign(SwitchButton::Right,  dpad.IsRight());
        button_state.Assign(SwitchButton::Left,   dpad.IsLeft());
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

    void GamesirController::MapInputReport0xC4(const GamesirReportData *src) {
        auto dpad = DirectionalPad(src->input0xC4.dpad);

        m_left_stick.SetValuesFrom(
            src->input0xC4.left_stick.GetX(),
            src->input0xC4.left_stick.GetYInverted()
        );

        m_right_stick.SetValuesFrom(
            src->input0xC4.right_stick.GetX(),
            src->input0xC4.right_stick.GetYInverted()
        );

        SwitchButtons button_state = m_buttons;
        button_state.Assign(SwitchButton::Down,   dpad.IsDown());
        button_state.Assign(SwitchButton::Up,     dpad.IsUp());
        button_state.Assign(SwitchButton::Right,  dpad.IsRight());
        button_state.Assign(SwitchButton::Left,   dpad.IsLeft());
        button_state.Assign(SwitchButton::A,      src->input0xC4.buttons.B);
        button_state.Assign(SwitchButton::B,      src->input0xC4.buttons.A);
        button_state.Assign(SwitchButton::X,      src->input0xC4.buttons.Y);
        button_state.Assign(SwitchButton::Y,      src->input0xC4.buttons.X);
        button_state.Assign(SwitchButton::R,      src->input0xC4.buttons.RB);
        button_state.Assign(SwitchButton::L,      src->input0xC4.buttons.LB);
        button_state.Assign(SwitchButton::ZR,     src->input0xC4.right_trigger > (m_trigger_threshold * TriggerMax));
        button_state.Assign(SwitchButton::ZL,     src->input0xC4.left_trigger  > (m_trigger_threshold * TriggerMax));
        button_state.Assign(SwitchButton::Minus,  src->input0xC4.buttons.select);
        button_state.Assign(SwitchButton::Plus,   src->input0xC4.buttons.start);
        button_state.Assign(SwitchButton::StickL, src->input0xC4.buttons.L3);
        button_state.Assign(SwitchButton::StickR, src->input0xC4.buttons.R3);
        m_buttons = button_state;
    }

}
