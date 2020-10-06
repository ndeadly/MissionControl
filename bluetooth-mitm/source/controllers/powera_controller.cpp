/*
 * Copyright (C) 2020 ndeadly
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "powera_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    namespace {

        const constexpr float stick_scale_factor = float(UINT12_MAX) / UINT8_MAX;

    }

    void PowerAController::UpdateControllerState(const bluetooth::HidReport *report) {
        auto powera_report = reinterpret_cast<const PowerAReportData *>(&report->data);

        switch(powera_report->id) {
            case 0x03:
                this->HandleInputReport0x03(powera_report);
                break;
            default:
                break;
        }
    }

    void PowerAController::HandleInputReport0x03(const PowerAReportData *src) {
        m_battery = src->input0x03.battery / 52 << 1;

        this->PackStickData(&m_left_stick,
            static_cast<uint16_t>(stick_scale_factor * src->input0x03.left_stick.x) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT8_MAX - src->input0x03.left_stick.y)) & 0xfff
        );
        this->PackStickData(&m_right_stick,
            static_cast<uint16_t>(stick_scale_factor * src->input0x03.right_stick.x) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT8_MAX - src->input0x03.right_stick.y)) & 0xfff
        );
        
        m_buttons.dpad_down   = (src->input0x03.buttons.dpad == PowerADPad_S)  ||
                                (src->input0x03.buttons.dpad == PowerADPad_SE) ||
                                (src->input0x03.buttons.dpad == PowerADPad_SW);
        m_buttons.dpad_up     = (src->input0x03.buttons.dpad == PowerADPad_N)  ||
                                (src->input0x03.buttons.dpad == PowerADPad_NE) ||
                                (src->input0x03.buttons.dpad == PowerADPad_NW);
        m_buttons.dpad_right  = (src->input0x03.buttons.dpad == PowerADPad_E)  ||
                                (src->input0x03.buttons.dpad == PowerADPad_NE) ||
                                (src->input0x03.buttons.dpad == PowerADPad_SE);
        m_buttons.dpad_left   = (src->input0x03.buttons.dpad == PowerADPad_W)  ||
                                (src->input0x03.buttons.dpad == PowerADPad_NW) ||
                                (src->input0x03.buttons.dpad == PowerADPad_SW);

        m_buttons.A = src->input0x03.buttons.B;
        m_buttons.B = src->input0x03.buttons.A;
        m_buttons.X = src->input0x03.buttons.Y;
        m_buttons.Y = src->input0x03.buttons.X;

        m_buttons.R  = src->input0x03.buttons.R1;
        m_buttons.ZR = src->input0x03.R2 > 0;
        m_buttons.L  = src->input0x03.buttons.L1;
        m_buttons.ZL = src->input0x03.L2 > 0; 

        m_buttons.minus = src->input0x03.buttons.select;
        m_buttons.plus  = src->input0x03.buttons.start;

        m_buttons.lstick_press = src->input0x03.buttons.L3;
        m_buttons.rstick_press = src->input0x03.buttons.R3;
    }

}
