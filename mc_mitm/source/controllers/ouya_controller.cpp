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
#include "ouya_controller.hpp"
#include "controller_utils.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    namespace {

        constexpr float stick_scale_factor = float(UINT12_MAX) / UINT16_MAX;

    }

    void OuyaController::ProcessInputData(const bluetooth::HidReport *report) {
        auto ouya_report = reinterpret_cast<const OuyaReportData *>(&report->data);

        switch(ouya_report->id) {
            case 0x03:
                this->MapInputReport0x03(ouya_report); break;
            case 0x07:
                this->MapInputReport0x07(ouya_report); break;
            default:
                break;
        }
    }

    void OuyaController::MapInputReport0x03(const OuyaReportData *src) {
        m_battery = convert_battery_255(src->input0x03.battery);
    }
    
    void OuyaController::MapInputReport0x07(const OuyaReportData *src) {
        m_left_stick.SetData(
            static_cast<u16>(stick_scale_factor * src->input0x07.left_stick.x) & UINT12_MAX,
            static_cast<u16>(stick_scale_factor * (UINT16_MAX - src->input0x07.left_stick.y)) & UINT12_MAX
        );
        m_right_stick.SetData(
            static_cast<u16>(stick_scale_factor * src->input0x07.right_stick.x) & UINT12_MAX,
            static_cast<u16>(stick_scale_factor * (UINT16_MAX - src->input0x07.right_stick.y)) & UINT12_MAX
        );
        
        m_buttons.dpad_down  = src->input0x07.buttons.dpad_down;
        m_buttons.dpad_up    = src->input0x07.buttons.dpad_up;
        m_buttons.dpad_right = src->input0x07.buttons.dpad_right;
        m_buttons.dpad_left  = src->input0x07.buttons.dpad_left;
        
        m_buttons.A = src->input0x07.buttons.A;
        m_buttons.B = src->input0x07.buttons.O;
        m_buttons.X = src->input0x07.buttons.Y;
        m_buttons.Y = src->input0x07.buttons.U;

        m_buttons.R  = src->input0x07.buttons.RB;
        m_buttons.ZR = src->input0x07.right_trigger > (m_trigger_threshold * UINT16_MAX);
        m_buttons.L  = src->input0x07.buttons.LB;
        m_buttons.ZL = src->input0x07.left_trigger  > (m_trigger_threshold * UINT16_MAX);

        m_buttons.minus = 0;
        m_buttons.plus  = 0;

        m_buttons.lstick_press = src->input0x07.buttons.LS;
        m_buttons.rstick_press = src->input0x07.buttons.RS;

        m_buttons.home = src->input0x07.buttons.center_hold;
    }

}
