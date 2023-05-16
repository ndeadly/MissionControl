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
#include "gembox_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    namespace {

        const constexpr float stick_scale_factor = float(UINT12_MAX) / UINT8_MAX;

    }

    void GemboxController::ProcessInputData(const bluetooth::HidReport *report) {
        auto gembox_report = reinterpret_cast<const GemboxReportData *>(&report->data);

        switch(gembox_report->id) {
            case 0x02:
                this->MapInputReport0x02(gembox_report); break;
            case 0x07:
                this->MapInputReport0x07(gembox_report); break;
            default:
                break;
        }
    }

    void GemboxController::MapInputReport0x02(const GemboxReportData *src) {
        m_buttons.minus = src->input0x02.back;
        //m_buttons.home = src->input0x02.buttons == 0;
    }

    void GemboxController::MapInputReport0x07(const GemboxReportData *src) {
        m_left_stick.SetData(
            static_cast<u16>(stick_scale_factor * -static_cast<s8>(~src->input0x07.left_stick.x + 1) + 0x7ff) & UINT12_MAX,
            static_cast<u16>(stick_scale_factor * (UINT8_MAX + static_cast<s8>(~src->input0x07.left_stick.y + 1)) + 0x7ff) & UINT12_MAX
        );
        m_right_stick.SetData(
            static_cast<u16>(stick_scale_factor * -static_cast<s8>(~src->input0x07.right_stick.x + 1) + 0x7ff) & UINT12_MAX,
            static_cast<u16>(stick_scale_factor * (UINT8_MAX + static_cast<s8>(~src->input0x07.right_stick.y + 1)) + 0x7ff) & UINT12_MAX
        );

        m_buttons.dpad_down  = (src->input0x07.dpad == GemboxDPad_S)  ||
                               (src->input0x07.dpad == GemboxDPad_SE) ||
                               (src->input0x07.dpad == GemboxDPad_SW);
        m_buttons.dpad_up    = (src->input0x07.dpad == GemboxDPad_N)  ||
                               (src->input0x07.dpad == GemboxDPad_NE) ||
                               (src->input0x07.dpad == GemboxDPad_NW);
        m_buttons.dpad_right = (src->input0x07.dpad == GemboxDPad_E)  ||
                               (src->input0x07.dpad == GemboxDPad_NE) ||
                               (src->input0x07.dpad == GemboxDPad_SE);
        m_buttons.dpad_left  = (src->input0x07.dpad == GemboxDPad_W)  ||
                               (src->input0x07.dpad == GemboxDPad_NW) ||
                               (src->input0x07.dpad == GemboxDPad_SW);

        m_buttons.A = src->input0x07.buttons.B;
        m_buttons.B = src->input0x07.buttons.A;
        m_buttons.X = src->input0x07.buttons.Y;
        m_buttons.Y = src->input0x07.buttons.X;

        m_buttons.R  = src->input0x07.buttons.RB;
        m_buttons.ZR = src->input0x07.right_trigger > (m_trigger_threshold * UINT8_MAX);
        m_buttons.L  = src->input0x07.buttons.LB;
        m_buttons.ZL = src->input0x07.left_trigger  > (m_trigger_threshold * UINT8_MAX);

        m_buttons.plus = src->input0x07.buttons.start;

        m_buttons.lstick_press = src->input0x07.buttons.L3;
        m_buttons.rstick_press = src->input0x07.buttons.R3;
    }

}
