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
#include "dualsense_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    namespace {

        const constexpr float stick_scale_factor = float(UINT12_MAX) / UINT8_MAX;

    }

    void DualsenseController::UpdateControllerState(const bluetooth::HidReport *report) {
        auto dualsense_report = reinterpret_cast<const DualsenseReportData *>(&report->data);

        switch(dualsense_report->id) {
            case 0x01:
                this->HandleInputReport0x01(dualsense_report);
                break;
            default:
                break;
        }
    }

    void DualsenseController::HandleInputReport0x01(const DualsenseReportData *src) {       
        m_left_stick = this->PackStickData(
            static_cast<uint16_t>(stick_scale_factor * src->input0x01.left_stick.x) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT8_MAX - src->input0x01.left_stick.y)) & 0xfff
        );
        m_right_stick = this->PackStickData(
            static_cast<uint16_t>(stick_scale_factor * src->input0x01.right_stick.x) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT8_MAX - src->input0x01.right_stick.y)) & 0xfff
        );

        this->MapButtons(&src->input0x01.buttons);
    }

    void DualsenseController::MapButtons(const DualsenseButtonData *buttons) {
        m_buttons.dpad_down   = (buttons->dpad == DualsenseDPad_S)  ||
                                (buttons->dpad == DualsenseDPad_SE) ||
                                (buttons->dpad == DualsenseDPad_SW);
        m_buttons.dpad_up     = (buttons->dpad == DualsenseDPad_N)  ||
                                (buttons->dpad == DualsenseDPad_NE) ||
                                (buttons->dpad == DualsenseDPad_NW);
        m_buttons.dpad_right  = (buttons->dpad == DualsenseDPad_E)  ||
                                (buttons->dpad == DualsenseDPad_NE) ||
                                (buttons->dpad == DualsenseDPad_SE);
        m_buttons.dpad_left   = (buttons->dpad == DualsenseDPad_W)  ||
                                (buttons->dpad == DualsenseDPad_NW) ||
                                (buttons->dpad == DualsenseDPad_SW);

        m_buttons.A = buttons->circle;
        m_buttons.B = buttons->cross;
        m_buttons.X = buttons->triangle;
        m_buttons.Y = buttons->square;

        m_buttons.R  = buttons->R1;
        m_buttons.ZR = buttons->R2;
        m_buttons.L  = buttons->L1;
        m_buttons.ZL = buttons->L2;

        m_buttons.minus = buttons->share;
        m_buttons.plus  = buttons->options;

        m_buttons.lstick_press = buttons->L3;
        m_buttons.rstick_press = buttons->R3;

        m_buttons.capture = buttons->tpad;
        m_buttons.home    = buttons->ps;
    }

}
