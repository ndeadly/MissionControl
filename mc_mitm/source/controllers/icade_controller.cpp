/*
 * Copyright (c) 2020-2025 ndeadly
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

    void ICadeController::ProcessInputData(const bluetooth::HidReport *report) {
        auto icade_report = reinterpret_cast<const ICadeReportData *>(&report->data);

        if (icade_report->id == 0x01) {

            for (unsigned int i = 0; i < sizeof(icade_report->input0x01.keys); ++i) {
                
                switch (icade_report->input0x01.keys[i]) {
                    case 0x1a: m_buttons.dpad_up    = 1; break; // w (joystick up pressed)
                    case 0x08: m_buttons.dpad_up    = 0; break; // e (joystick up released)
                    case 0x07: m_buttons.dpad_right = 1; break; // d (joystick right pressed)
                    case 0x06: m_buttons.dpad_right = 0; break; // c (joystick right released)
                    case 0x1b: m_buttons.dpad_down  = 1; break; // x (joystick down pressed)
                    case 0x1d: m_buttons.dpad_down  = 0; break; // z (joystick down released)
                    case 0x04: m_buttons.dpad_left  = 1; break; // a (joystick left pressed)
                    case 0x14: m_buttons.dpad_left  = 0; break; // q (joystick left released)
                    case 0x1c: m_buttons.L          = 1; break; // y (button 1 pressed)
                    case 0x17: m_buttons.L          = 0; break; // t (button 1 released)
                    case 0x18: m_buttons.X          = 1; break; // u (button 2 pressed)
                    case 0x09: m_buttons.X          = 0; break; // f (button 2 released)
                    case 0x0c: m_buttons.A          = 1; break; // i (button 3 pressed)
                    case 0x10: m_buttons.A          = 0; break; // m (button 3 released)
                    case 0x12: m_buttons.R          = 1; break; // o (button 4 pressed)
                    case 0x0a: m_buttons.R          = 0; break; // g (button 4 released)
                    case 0x0b: m_buttons.ZL         = 1; break; // h (button 5 pressed)
                    case 0x15: m_buttons.ZL         = 0; break; // r (button 5 released)
                    case 0x0d: m_buttons.Y          = 1; break; // j (button 6 pressed)
                    case 0x11: m_buttons.Y          = 0; break; // n (button 6 released)
                    case 0x0e: m_buttons.B          = 1; break; // k (button 7 pressed)
                    case 0x13: m_buttons.B          = 0; break; // p (button 7 released)
                    case 0x0f: m_buttons.ZR         = 1; break; // l (button 8 pressed)
                    case 0x19: m_buttons.ZR         = 0; break; // v (button 8 released)
                    default:
                        break;
                }

                ++i;
            }

        }

    }

    void ICadeController::ApplyButtonCombos(SwitchButtonData *buttons) {
        // Combo for minus button
        if (buttons->ZL && buttons->ZR && buttons->L) {
            buttons->minus = 1;
            buttons->ZL = 0;
            buttons->ZR = 0;
            buttons->L = 0;
        }

        // Combo for plus button
        if (buttons->ZL && buttons->ZR && buttons->R) {
            buttons->plus = 1;
            buttons->ZL = 0;
            buttons->ZR = 0;
            buttons->R = 0;
        }

        EmulatedSwitchController::ApplyButtonCombos(buttons);
    }

}
