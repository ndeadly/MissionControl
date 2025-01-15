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
#include "atari_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    namespace {

        constexpr u16 TriggerMax = 0x3ff;

    }

    void AtariController::ProcessInputData(const bluetooth::HidReport *report) {
        auto atari_report = reinterpret_cast<const AtariReportData *>(&report->data);

        switch(atari_report->id) {
            case 0x01:
                this->MapInputReport0x01(atari_report); break;
            case 0x02:
                this->MapInputReport0x02(atari_report); break;    
            default:
                break;
        }
    }

    void AtariController::MapInputReport0x01(const AtariReportData *src) {
        m_left_stick  = PackAnalogStickValues(src->input0x01.left_stick.x,  InvertAnalogStickValue(src->input0x01.left_stick.y));
        m_right_stick = PackAnalogStickValues(src->input0x01.right_stick.x, InvertAnalogStickValue(src->input0x01.right_stick.y));
        
        m_buttons.dpad_down  = (src->input0x01.buttons.dpad == AtariDPad_S)  ||
                               (src->input0x01.buttons.dpad == AtariDPad_SE) ||
                               (src->input0x01.buttons.dpad == AtariDPad_SW);
        m_buttons.dpad_up    = (src->input0x01.buttons.dpad == AtariDPad_N)  ||
                               (src->input0x01.buttons.dpad == AtariDPad_NE) ||
                               (src->input0x01.buttons.dpad == AtariDPad_NW);
        m_buttons.dpad_right = (src->input0x01.buttons.dpad == AtariDPad_E)  ||
                               (src->input0x01.buttons.dpad == AtariDPad_NE) ||
                               (src->input0x01.buttons.dpad == AtariDPad_SE);
        m_buttons.dpad_left  = (src->input0x01.buttons.dpad == AtariDPad_W)  ||
                               (src->input0x01.buttons.dpad == AtariDPad_NW) ||
                               (src->input0x01.buttons.dpad == AtariDPad_SW);

        m_buttons.A = src->input0x01.buttons.B;
        m_buttons.B = src->input0x01.buttons.A;
        m_buttons.X = src->input0x01.buttons.Y;
        m_buttons.Y = src->input0x01.buttons.X;

        m_buttons.R  = src->input0x01.buttons.RB;
        m_buttons.L  = src->input0x01.buttons.LB;
        m_buttons.ZR = src->input0x01.right_trigger > (m_trigger_threshold * TriggerMax);
        m_buttons.ZL = src->input0x01.left_trigger  > (m_trigger_threshold * TriggerMax);

        m_buttons.lstick_press = src->input0x01.buttons.L3;
        m_buttons.rstick_press = src->input0x01.buttons.R3;

        m_buttons.minus = src->input0x01.buttons.back;
        m_buttons.plus  = src->input0x01.buttons.menu;

        m_buttons.home = src->input0x01.buttons.home;
    }

    void AtariController::MapInputReport0x02(const AtariReportData *src) {
        AMS_UNUSED(src);
    }

}
