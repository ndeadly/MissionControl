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
#include "icade_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    void ICadeController::ProcessInputData(const u8 *report_buffer, size_t size) {
        AMS_UNUSED(size);
        auto report = reinterpret_cast<const ICadeReportData *>(report_buffer);

        SwitchButtons button_state = m_buttons;
        if (report->id == 0x01) {
            for (unsigned int i = 0; i < sizeof(report->input0x01.keys); ++i) {
                switch (report->input0x01.keys[i]) {
                    case 0x1a: button_state.Set(SwitchButton::Up);      break; // w (joystick up pressed)
                    case 0x08: button_state.Clear(SwitchButton::Up);    break; // e (joystick up released)
                    case 0x07: button_state.Set(SwitchButton::Right);   break; // d (joystick right pressed)
                    case 0x06: button_state.Clear(SwitchButton::Right); break; // c (joystick right released)
                    case 0x1b: button_state.Set(SwitchButton::Down);    break; // x (joystick down pressed)
                    case 0x1d: button_state.Clear(SwitchButton::Down);  break; // z (joystick down released)
                    case 0x04: button_state.Set(SwitchButton::Left);    break; // a (joystick left pressed)
                    case 0x14: button_state.Clear(SwitchButton::Left);  break; // q (joystick left released)
                    case 0x1c: button_state.Set(SwitchButton::L);       break; // y (button 1 pressed)
                    case 0x17: button_state.Clear(SwitchButton::L);     break; // t (button 1 released)
                    case 0x18: button_state.Set(SwitchButton::X);       break; // u (button 2 pressed)
                    case 0x09: button_state.Clear(SwitchButton::X);     break; // f (button 2 released)
                    case 0x0c: button_state.Set(SwitchButton::A);       break; // i (button 3 pressed)
                    case 0x10: button_state.Clear(SwitchButton::A);     break; // m (button 3 released)
                    case 0x12: button_state.Set(SwitchButton::R);       break; // o (button 4 pressed)
                    case 0x0a: button_state.Clear(SwitchButton::R);     break; // g (button 4 released)
                    case 0x0b: button_state.Set(SwitchButton::ZL);      break; // h (button 5 pressed)
                    case 0x15: button_state.Clear(SwitchButton::ZL);    break; // r (button 5 released)
                    case 0x0d: button_state.Set(SwitchButton::Y);       break; // j (button 6 pressed)
                    case 0x11: button_state.Clear(SwitchButton::Y);     break; // n (button 6 released)
                    case 0x0e: button_state.Set(SwitchButton::B);       break; // k (button 7 pressed)
                    case 0x13: button_state.Clear(SwitchButton::B);     break; // p (button 7 released)
                    case 0x0f: button_state.Set(SwitchButton::ZR);      break; // l (button 8 pressed)
                    case 0x19: button_state.Clear(SwitchButton::ZR);    break; // v (button 8 released)
                    default:
                        break;
                }

                ++i;
            }
        }

        m_buttons = button_state;
    }

    void ICadeController::ApplyButtonCombos(SwitchButtons *buttons) {
        // Combo for minus button
        if (buttons->All(SwitchButton::ZL | SwitchButton::ZR | SwitchButton::L)) {
            buttons->Set(SwitchButton::Minus);
            buttons->Clear(SwitchButton::ZL | SwitchButton::ZR | SwitchButton::L);
        }

        // Combo for plus button
        if (buttons->All(SwitchButton::ZL | SwitchButton::ZR | SwitchButton::R)) {
            buttons->Set(SwitchButton::Plus);
            buttons->Clear(SwitchButton::ZL | SwitchButton::ZR | SwitchButton::R);
        }

        EmulatedSwitchController::ApplyButtonCombos(buttons);
    }

}
