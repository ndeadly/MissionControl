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
        m_buttons.minus = src->input0x01.buttons.back;
        m_buttons.home = src->input0x01.buttons.home;
    }

    void GamestickController::MapInputReport0x03(const GamestickReportData *src) {
        m_left_stick  = PackAnalogStickValues(src->input0x03.left_stick.x,  InvertAnalogStickValue(src->input0x03.left_stick.y));
        m_right_stick = PackAnalogStickValues(src->input0x03.right_stick.x, InvertAnalogStickValue(src->input0x03.right_stick.y));
        
        m_buttons.dpad_down  = (src->input0x03.dpad == GamestickDPad_S)  ||
                               (src->input0x03.dpad == GamestickDPad_SE) ||
                               (src->input0x03.dpad == GamestickDPad_SW);
        m_buttons.dpad_up    = (src->input0x03.dpad == GamestickDPad_N)  ||
                               (src->input0x03.dpad == GamestickDPad_NE) ||
                               (src->input0x03.dpad == GamestickDPad_NW);
        m_buttons.dpad_right = (src->input0x03.dpad == GamestickDPad_E)  ||
                               (src->input0x03.dpad == GamestickDPad_NE) ||
                               (src->input0x03.dpad == GamestickDPad_SE);
        m_buttons.dpad_left  = (src->input0x03.dpad == GamestickDPad_W)  ||
                               (src->input0x03.dpad == GamestickDPad_NW) ||
                               (src->input0x03.dpad == GamestickDPad_SW);
        
        m_buttons.A = src->input0x03.buttons.B;
        m_buttons.B = src->input0x03.buttons.A;
        m_buttons.X = src->input0x03.buttons.Y;
        m_buttons.Y = src->input0x03.buttons.X;

        m_buttons.L = src->input0x03.buttons.L;
        m_buttons.R = src->input0x03.buttons.R;

        // Combos for ZL/ZR
        if (m_buttons.dpad_down) {
            m_buttons.ZL = src->input0x03.buttons.L;
            m_buttons.ZR = src->input0x03.buttons.R;
            m_buttons.dpad_down = !(m_buttons.ZL || m_buttons.ZR);
            m_buttons.L = !m_buttons.ZL;
            m_buttons.R = !m_buttons.ZR;
        }

        m_buttons.plus = src->input0x03.buttons.start;

        m_buttons.lstick_press = src->input0x03.buttons.lstick_press;
        m_buttons.rstick_press = src->input0x03.buttons.rstick_press;
    }

}
