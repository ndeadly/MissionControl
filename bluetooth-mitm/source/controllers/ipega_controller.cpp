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
#include "ipega_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    namespace {

        const constexpr float stick_scale_factor = float(UINT12_MAX) / UINT8_MAX;

    }

    void IpegaController::UpdateControllerState(const bluetooth::HidReport *report) {
        auto ipega_report = reinterpret_cast<const IpegaReportData *>(&report->data);

        switch(ipega_report->id) {
            case 0x02:
                this->HandleInputReport0x02(ipega_report);
                break;
            case 0x07:
                this->HandleInputReport0x07(ipega_report);
                break;
            default:
                break;
        }
    }

    void IpegaController::HandleInputReport0x02(const IpegaReportData *src) {
        m_buttons.home = src->input0x02.home;
    }

    void IpegaController::HandleInputReport0x07(const IpegaReportData *src) {
        this->PackStickData(&m_left_stick,
            static_cast<uint16_t>(stick_scale_factor * src->input0x07.left_stick.x) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT8_MAX - src->input0x07.left_stick.y)) & 0xfff
        );
        this->PackStickData(&m_right_stick,
            static_cast<uint16_t>(stick_scale_factor * src->input0x07.right_stick.x) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT8_MAX - src->input0x07.right_stick.y)) & 0xfff
        );

        m_buttons.dpad_down   = (src->input0x07.buttons.dpad == IpegaDPad_S)  ||
                                (src->input0x07.buttons.dpad == IpegaDPad_SE) ||
                                (src->input0x07.buttons.dpad == IpegaDPad_SW);
        m_buttons.dpad_up     = (src->input0x07.buttons.dpad == IpegaDPad_N)  ||
                                (src->input0x07.buttons.dpad == IpegaDPad_NE) ||
                                (src->input0x07.buttons.dpad == IpegaDPad_NW);
        m_buttons.dpad_right  = (src->input0x07.buttons.dpad == IpegaDPad_E)  ||
                                (src->input0x07.buttons.dpad == IpegaDPad_NE) ||
                                (src->input0x07.buttons.dpad == IpegaDPad_SE);
        m_buttons.dpad_left   = (src->input0x07.buttons.dpad == IpegaDPad_W)  ||
                                (src->input0x07.buttons.dpad == IpegaDPad_NW) ||
                                (src->input0x07.buttons.dpad == IpegaDPad_SW);

        m_buttons.A = src->input0x07.buttons.B;
        m_buttons.B = src->input0x07.buttons.A;
        m_buttons.X = src->input0x07.buttons.Y;
        m_buttons.Y = src->input0x07.buttons.X;

        m_buttons.R  = src->input0x07.buttons.RB;
        m_buttons.ZR = src->input0x07.buttons.RT;
        m_buttons.L  = src->input0x07.buttons.LB;
        m_buttons.ZL = src->input0x07.buttons.LT;

        m_buttons.minus = src->input0x07.buttons.view;
        m_buttons.plus  = src->input0x07.buttons.menu;

        m_buttons.lstick_press = src->input0x07.buttons.lstick_press;
        m_buttons.rstick_press = src->input0x07.buttons.rstick_press;
    }

}
