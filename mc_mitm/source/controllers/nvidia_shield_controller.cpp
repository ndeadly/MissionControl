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
#include "nvidia_shield_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    namespace {

        constexpr float stick_scale_factor = float(UINT12_MAX) / UINT16_MAX;

    }

    void NvidiaShieldController::ProcessInputData(const bluetooth::HidReport *report) {
        auto nvidia_report = reinterpret_cast<const NvidiaShieldReportData *>(&report->data);

        switch(nvidia_report->id) {
            case 0x01:
                this->MapInputReport0x01(nvidia_report); break;
            case 0x03:
                this->MapInputReport0x03(nvidia_report); break;
            default:
                break;
        }
    }

    void NvidiaShieldController::MapInputReport0x01(const NvidiaShieldReportData *src) {
        m_left_stick.SetData(
            static_cast<u16>(stick_scale_factor * src->input0x01.left_stick.x) & UINT12_MAX,
            static_cast<u16>(stick_scale_factor * (UINT16_MAX - src->input0x01.left_stick.y)) & UINT12_MAX
        );
        m_right_stick.SetData(
            static_cast<u16>(stick_scale_factor * src->input0x01.right_stick.x) & UINT12_MAX,
            static_cast<u16>(stick_scale_factor * (UINT16_MAX - src->input0x01.right_stick.y)) & UINT12_MAX
        );

        m_buttons.dpad_down  = (src->input0x01.dpad == NvidiaShieldDPad_S)  ||
                               (src->input0x01.dpad == NvidiaShieldDPad_SE) ||
                               (src->input0x01.dpad == NvidiaShieldDPad_SW);
        m_buttons.dpad_up    = (src->input0x01.dpad == NvidiaShieldDPad_N)  ||
                               (src->input0x01.dpad == NvidiaShieldDPad_NE) ||
                               (src->input0x01.dpad == NvidiaShieldDPad_NW);
        m_buttons.dpad_right = (src->input0x01.dpad == NvidiaShieldDPad_E)  ||
                               (src->input0x01.dpad == NvidiaShieldDPad_NE) ||
                               (src->input0x01.dpad == NvidiaShieldDPad_SE);
        m_buttons.dpad_left  = (src->input0x01.dpad == NvidiaShieldDPad_W)  ||
                               (src->input0x01.dpad == NvidiaShieldDPad_NW) ||
                               (src->input0x01.dpad == NvidiaShieldDPad_SW);

        m_buttons.A = src->input0x01.buttons.B;
        m_buttons.B = src->input0x01.buttons.A;
        m_buttons.X = src->input0x01.buttons.Y;
        m_buttons.Y = src->input0x01.buttons.X;

        m_buttons.R  = src->input0x01.buttons.RB;
        m_buttons.ZR = src->input0x01.right_trigger > (m_trigger_threshold * UINT16_MAX);
        m_buttons.L  = src->input0x01.buttons.LB;
        m_buttons.ZL = src->input0x01.left_trigger  > (m_trigger_threshold * UINT16_MAX);

        m_buttons.minus = src->input0x01.back;
        m_buttons.plus  = src->input0x01.buttons.start;

        m_buttons.lstick_press = src->input0x01.buttons.L3;
        m_buttons.rstick_press = src->input0x01.buttons.R3;

        m_buttons.home = src->input0x01.home;
    }

    void NvidiaShieldController::MapInputReport0x03(const NvidiaShieldReportData *src) {
        AMS_UNUSED(src);
    }

}
