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
#include "mocute_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    namespace {

        const constexpr float stick_scale_factor = float(UINT12_MAX) / UINT8_MAX;

    }

    void MocuteController::UpdateControllerState(const bluetooth::HidReport *report) {
        auto mocute_report = reinterpret_cast<const MocuteReportData *>(&report->data);

        switch(mocute_report->id) {
            case 0x04:
                this->HandleInputReport0x04(mocute_report);
                break;
            default:
                break;
        }
    }

    void MocuteController::HandleInputReport0x04(const MocuteReportData *src) {
        this->PackStickData(&m_left_stick,
            static_cast<uint16_t>(stick_scale_factor * src->input0x04.left_stick.x) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT8_MAX - src->input0x04.left_stick.y)) & 0xfff
        );
        this->PackStickData(&m_right_stick,
            static_cast<uint16_t>(stick_scale_factor * src->input0x04.right_stick.x) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT8_MAX - src->input0x04.right_stick.y)) & 0xfff
        );
        
        m_buttons.dpad_down   = (src->input0x04.buttons.dpad == MocuteDPad_S)  ||
                                (src->input0x04.buttons.dpad == MocuteDPad_SE) ||
                                (src->input0x04.buttons.dpad == MocuteDPad_SW);
        m_buttons.dpad_up     = (src->input0x04.buttons.dpad == MocuteDPad_N)  ||
                                (src->input0x04.buttons.dpad == MocuteDPad_NE) ||
                                (src->input0x04.buttons.dpad == MocuteDPad_NW);
        m_buttons.dpad_right  = (src->input0x04.buttons.dpad == MocuteDPad_E)  ||
                                (src->input0x04.buttons.dpad == MocuteDPad_NE) ||
                                (src->input0x04.buttons.dpad == MocuteDPad_SE);
        m_buttons.dpad_left   = (src->input0x04.buttons.dpad == MocuteDPad_W)  ||
                                (src->input0x04.buttons.dpad == MocuteDPad_NW) ||
                                (src->input0x04.buttons.dpad == MocuteDPad_SW);

        m_buttons.A = src->input0x04.buttons.B;
        m_buttons.B = src->input0x04.buttons.A;
        m_buttons.X = src->input0x04.buttons.Y;
        m_buttons.Y = src->input0x04.buttons.X;

        m_buttons.R  = src->input0x04.buttons.R1;
        m_buttons.ZR = src->input0x04.buttons.R2;
        m_buttons.L  = src->input0x04.buttons.L1;
        m_buttons.ZL = src->input0x04.buttons.L2; 

        m_buttons.minus = src->input0x04.buttons.select;
        m_buttons.plus  = src->input0x04.buttons.start;

        m_buttons.lstick_press = src->input0x04.buttons.L3;
        m_buttons.rstick_press = src->input0x04.buttons.R3;
    }

}
