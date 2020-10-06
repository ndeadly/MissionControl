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
#include "gamesir_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    namespace {

        const constexpr float stick_scale_factor = float(UINT12_MAX) / UINT8_MAX;

    }

    void GamesirController::UpdateControllerState(const bluetooth::HidReport *report) {
        auto gamesir_report = reinterpret_cast<const GamesirReportData *>(&report->data);

        switch(gamesir_report->id) {
            case 0x12:
                this->HandleInputReport0x12(gamesir_report);
                break;
            case 0xc4:
                this->HandleInputReport0xc4(gamesir_report);
                break;
            default:
                break;
        }
    }

    void GamesirController::HandleInputReport0x12(const GamesirReportData *src) {
        m_buttons.home = src->input0x12.home;
    }

    void GamesirController::HandleInputReport0xc4(const GamesirReportData *src) {
        this->PackStickData(&m_left_stick,
            static_cast<uint16_t>(stick_scale_factor * src->input0xc4.left_stick.x) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT8_MAX - src->input0xc4.left_stick.y)) & 0xfff
        );
        this->PackStickData(&m_right_stick,
            static_cast<uint16_t>(stick_scale_factor * src->input0xc4.right_stick.x) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT8_MAX - src->input0xc4.right_stick.y)) & 0xfff
        );

        m_buttons.dpad_down   = (src->input0xc4.buttons.dpad == GamesirDpad_S)  ||
                                (src->input0xc4.buttons.dpad == GamesirDpad_SE) ||
                                (src->input0xc4.buttons.dpad == GamesirDpad_SW);
        m_buttons.dpad_up     = (src->input0xc4.buttons.dpad == GamesirDpad_N)  ||
                                (src->input0xc4.buttons.dpad == GamesirDpad_NE) ||
                                (src->input0xc4.buttons.dpad == GamesirDpad_NW);
        m_buttons.dpad_right  = (src->input0xc4.buttons.dpad == GamesirDpad_E)  ||
                                (src->input0xc4.buttons.dpad == GamesirDpad_NE) ||
                                (src->input0xc4.buttons.dpad == GamesirDpad_SE);
        m_buttons.dpad_left   = (src->input0xc4.buttons.dpad == GamesirDpad_W)  ||
                                (src->input0xc4.buttons.dpad == GamesirDpad_NW) ||
                                (src->input0xc4.buttons.dpad == GamesirDpad_SW);

        m_buttons.A = src->input0xc4.buttons.B;
        m_buttons.B = src->input0xc4.buttons.A;
        m_buttons.X = src->input0xc4.buttons.Y;
        m_buttons.Y = src->input0xc4.buttons.X;

        m_buttons.R  = src->input0xc4.buttons.RB;
        m_buttons.ZR = src->input0xc4.buttons.RT;
        m_buttons.L  = src->input0xc4.buttons.LB;
        m_buttons.ZL = src->input0xc4.buttons.LT; 

        m_buttons.minus = src->input0xc4.buttons.select;
        m_buttons.plus  = src->input0xc4.buttons.start;

        m_buttons.lstick_press = src->input0xc4.buttons.L3;
        m_buttons.rstick_press = src->input0xc4.buttons.R3;  
    }

}
