/*
 * Copyright (c) 2020-2023 ndeadly
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
#include "betop_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    namespace {

        const constexpr float stick_scale_factor = float(UINT12_MAX) / UINT8_MAX;

    }

    void BetopController::ProcessInputData(const bluetooth::HidReport *report) {
        auto betop_report = reinterpret_cast<const BetopReportData *>(&report->data);

        switch(betop_report->id) {
            case 0x03:
                this->MapInputReport0x03(betop_report); break;
            default:
                break;
        }
    }

    void BetopController::MapInputReport0x03(const BetopReportData *src) {
        m_left_stick.SetData(
            static_cast<u16>(stick_scale_factor * src->input0x03.left_stick.x) & UINT12_MAX,
            static_cast<u16>(stick_scale_factor * (UINT8_MAX - src->input0x03.left_stick.y)) & UINT12_MAX
        );
        m_right_stick.SetData(
            static_cast<u16>(stick_scale_factor * src->input0x03.right_stick.x) & UINT12_MAX,
            static_cast<u16>(stick_scale_factor * (UINT8_MAX - src->input0x03.right_stick.y)) & UINT12_MAX
        );
        
        m_buttons.dpad_down  = (src->input0x03.buttons.dpad == BetopDPad_S)  ||
                               (src->input0x03.buttons.dpad == BetopDPad_SE) ||
                               (src->input0x03.buttons.dpad == BetopDPad_SW);
        m_buttons.dpad_up    = (src->input0x03.buttons.dpad == BetopDPad_N)  ||
                               (src->input0x03.buttons.dpad == BetopDPad_NE) ||
                               (src->input0x03.buttons.dpad == BetopDPad_NW);
        m_buttons.dpad_right = (src->input0x03.buttons.dpad == BetopDPad_E)  ||
                               (src->input0x03.buttons.dpad == BetopDPad_NE) ||
                               (src->input0x03.buttons.dpad == BetopDPad_SE);
        m_buttons.dpad_left  = (src->input0x03.buttons.dpad == BetopDPad_W)  ||
                               (src->input0x03.buttons.dpad == BetopDPad_NW) ||
                               (src->input0x03.buttons.dpad == BetopDPad_SW);

        m_buttons.A = src->input0x03.buttons.B;
        m_buttons.B = src->input0x03.buttons.A;
        m_buttons.X = src->input0x03.buttons.Y;
        m_buttons.Y = src->input0x03.buttons.X;

        m_buttons.R  = src->input0x03.buttons.R1;
        m_buttons.ZR = src->input0x03.buttons.R2;
        m_buttons.L  = src->input0x03.buttons.L1;
        m_buttons.ZL = src->input0x03.buttons.L2;

        m_buttons.lstick_press = src->input0x03.buttons.L3;
        m_buttons.rstick_press = src->input0x03.buttons.R3;

        m_buttons.minus = src->input0x03.buttons.select;
        m_buttons.plus  = src->input0x03.buttons.start;

        m_buttons.home = src->input0x03.buttons.home;
    }

}
