/*
 * Copyright (c) 2020-2022 ndeadly
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

        const constexpr float stick_scale_factor = float(UINT12_MAX) / UINT8_MAX;

    }

    void AtGamesController::ProcessInputData(const bluetooth::HidReport *report) {
        auto atgames_report = reinterpret_cast<const AtGamesReportData *>(&report->data);

        switch(atgames_report->id) {
            case 0x01:
                this->MapInputReport0x01(atgames_report); break;
            default:
                break;
        }
    }

    void AtGamesController::MapInputReport0x01(const AtGamesReportData *src) {
        m_left_stick.SetData(
            STICK_ZERO + 0x7ff * (src->input0x01.nudge_left - src->input0x01.nudge_right),
            STICK_ZERO
        );
        m_right_stick.SetData(
            STICK_ZERO,
            static_cast<uint16_t>(stick_scale_factor * (UINT8_MAX - src->input0x01.right_stick.x)) & 0xfff
        );
        
        m_buttons.dpad_down   = (src->input0x01.dpad == AtGamesDPad_S)  ||
                                (src->input0x01.dpad == AtGamesDPad_SE) ||
                                (src->input0x01.dpad == AtGamesDPad_SW);
        m_buttons.dpad_up     = (src->input0x01.dpad == AtGamesDPad_N)  ||
                                (src->input0x01.dpad == AtGamesDPad_NE) ||
                                (src->input0x01.dpad == AtGamesDPad_NW);
        m_buttons.dpad_right  = (src->input0x01.dpad == AtGamesDPad_E)  ||
                                (src->input0x01.dpad == AtGamesDPad_NE) ||
                                (src->input0x01.dpad == AtGamesDPad_SE);
        m_buttons.dpad_left   = (src->input0x01.dpad == AtGamesDPad_W)  ||
                                (src->input0x01.dpad == AtGamesDPad_NW) ||
                                (src->input0x01.dpad == AtGamesDPad_SW);

        m_buttons.A = src->input0x01.play;
        m_buttons.B = src->input0x01.rewind;
        m_buttons.Y = src->input0x01.nudge_front;

        m_buttons.R  = src->input0x01.flipper_right;
        m_buttons.ZR = src->input0x01.flipper_right;
        m_buttons.L  = src->input0x01.flipper_left;
        m_buttons.ZL = src->input0x01.flipper_left; 

        m_buttons.plus  = src->input0x01.home_twirl;
    }

}
