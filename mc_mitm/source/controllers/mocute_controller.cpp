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
#include "mocute_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    namespace {

        const constexpr float stick_scale_factor = float(UINT12_MAX) / UINT8_MAX;

    }

    MocuteController::MocuteController(const bluetooth::Address *address, HardwareID id)
    : EmulatedSwitchController(address, id) {
        if (id.vid == 0x0000 && id.pid == 0x0000) {
            m_variant = MocuteControllerVariant_053;
        } else {
            m_variant = MocuteControllerVariant_050;
        }
    }

    void MocuteController::ProcessInputData(const bluetooth::HidReport *report) {
        auto mocute_report = reinterpret_cast<const MocuteReportData *>(&report->data);

        if (m_variant == MocuteControllerVariant_050) {
            switch(mocute_report->id) {
                case 0x01:
                case 0x04:
                case 0x06:
                    this->MapInputReport0x01(mocute_report); break;
                default:
                    break;
            }
        } else {
            switch(mocute_report->id) {
                case 0x04:
                    this->MapInputReport0x04(mocute_report); break;
                default:
                    break;
            }
        }

    }

    void MocuteController::MapInputReport0x01(const MocuteReportData *src) {
        this->MapAnalogSticks(&src->input0x01.left_stick, &src->input0x01.right_stick);
        this->MapButtons(&src->input0x01.buttons, src->id == 0x01);

        m_buttons.ZR = src->input0x01.right_trigger > (m_trigger_threshold * UINT8_MAX);
        m_buttons.ZL = src->input0x01.left_trigger  > (m_trigger_threshold * UINT8_MAX);
    }

    void MocuteController::MapInputReport0x04(const MocuteReportData *src) {
        this->MapAnalogSticks(&src->input0x04.left_stick, &src->input0x04.right_stick);
        this->MapButtons(&src->input0x04.buttons, 1);

        m_buttons.ZR = src->input0x04.right_trigger > (m_trigger_threshold * UINT8_MAX);
        m_buttons.ZL = src->input0x04.left_trigger  > (m_trigger_threshold * UINT8_MAX);
    }

    void MocuteController::MapAnalogSticks(const MocuteStickData *left_stick, const MocuteStickData *right_stick) {
        m_left_stick.SetData(
            static_cast<u16>(stick_scale_factor * left_stick->x) & UINT12_MAX,
            static_cast<u16>(stick_scale_factor * (UINT8_MAX - left_stick->y)) & UINT12_MAX
        );
        m_right_stick.SetData(
            static_cast<u16>(stick_scale_factor * right_stick->x) & UINT12_MAX,
            static_cast<u16>(stick_scale_factor * (UINT8_MAX - right_stick->y)) & UINT12_MAX
        );
    }

    void MocuteController::MapButtons(const MocuteButtonData *buttons, u8 dpad_format) {
        // Convert dpad to always use the same format
        u8 dpad = buttons->dpad;
        if (dpad_format == 1) {
            dpad = (dpad == 0) ? MocuteDPad_Released : dpad - 1;
        }

        m_buttons.dpad_down  = (dpad == MocuteDPad_S)  ||
                               (dpad == MocuteDPad_SE) ||
                               (dpad == MocuteDPad_SW);
        m_buttons.dpad_up    = (dpad == MocuteDPad_N)  ||
                               (dpad == MocuteDPad_NE) ||
                               (dpad == MocuteDPad_NW);
        m_buttons.dpad_right = (dpad == MocuteDPad_E)  ||
                               (dpad == MocuteDPad_NE) ||
                               (dpad == MocuteDPad_SE);
        m_buttons.dpad_left  = (dpad == MocuteDPad_W)  ||
                               (dpad == MocuteDPad_NW) ||
                               (dpad == MocuteDPad_SW);

        m_buttons.A = buttons->B;
        m_buttons.B = buttons->A;
        m_buttons.X = buttons->Y;
        m_buttons.Y = buttons->X;

        m_buttons.R  = buttons->R1;
        m_buttons.L  = buttons->L1;

        m_buttons.minus = buttons->select;
        m_buttons.plus  = buttons->start;

        m_buttons.lstick_press = buttons->L3;
        m_buttons.rstick_press = buttons->R3;
    }

}
