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
#include "steelseries_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    namespace {

        const constexpr float stick_scale_factor = float(UINT12_MAX) / UINT8_MAX;

    }

    void SteelseriesController::ProcessInputData(const bluetooth::HidReport *report) {
        auto steelseries_report = reinterpret_cast<const SteelseriesReportData *>(&report->data);

        switch(steelseries_report->id) {
            case 0x01:
                if (report->size == sizeof(SteelseriesInputReport0x01_v2) + 1) {
                    this->MapInputReport0x01_v2(steelseries_report);
                } else {
                    this->MapInputReport0x01(steelseries_report);
                }
                break;
            case 0x02:
                this->MapInputReport0x02(steelseries_report); break;
            case 0x12:
                this->MapInputReport0x12(steelseries_report); break;
            case 0xc4:
                this->MapInputReport0xc4(steelseries_report); break;
            default:
                // Todo: handle this properly
                this->MapMfiInputReport(steelseries_report);
                break;
        }
    }

    void SteelseriesController::MapInputReport0x01(const SteelseriesReportData *src) {
        m_left_stick.SetData(
            static_cast<u16>(stick_scale_factor * -static_cast<s8>(~src->input0x01.left_stick.x + 1) + 0x7ff) & UINT12_MAX,
            static_cast<u16>(stick_scale_factor * (UINT8_MAX + static_cast<s8>(~src->input0x01.left_stick.y + 1)) + 0x7ff) & UINT12_MAX
        );
        m_right_stick.SetData(
            static_cast<u16>(stick_scale_factor * -static_cast<s8>(~src->input0x01.right_stick.x + 1) + 0x7ff) & UINT12_MAX,
            static_cast<u16>(stick_scale_factor * (UINT8_MAX + static_cast<s8>(~src->input0x01.right_stick.y + 1)) + 0x7ff) & UINT12_MAX
        );

        m_buttons.dpad_down  = (src->input0x01.dpad == SteelseriesDPad_S)  ||
                               (src->input0x01.dpad == SteelseriesDPad_SE) ||
                               (src->input0x01.dpad == SteelseriesDPad_SW);
        m_buttons.dpad_up    = (src->input0x01.dpad == SteelseriesDPad_N)  ||
                               (src->input0x01.dpad == SteelseriesDPad_NE) ||
                               (src->input0x01.dpad == SteelseriesDPad_NW);
        m_buttons.dpad_right = (src->input0x01.dpad == SteelseriesDPad_E)  ||
                               (src->input0x01.dpad == SteelseriesDPad_NE) ||
                               (src->input0x01.dpad == SteelseriesDPad_SE);
        m_buttons.dpad_left  = (src->input0x01.dpad == SteelseriesDPad_W)  ||
                               (src->input0x01.dpad == SteelseriesDPad_NW) ||
                               (src->input0x01.dpad == SteelseriesDPad_SW);

        m_buttons.A = src->input0x01.buttons.B;
        m_buttons.B = src->input0x01.buttons.A;
        m_buttons.X = src->input0x01.buttons.Y;
        m_buttons.Y = src->input0x01.buttons.X;

        m_buttons.R = src->input0x01.buttons.R1;
        m_buttons.L = src->input0x01.buttons.L1;

        m_buttons.minus = src->input0x01.buttons.select;
        m_buttons.plus  = src->input0x01.buttons.start;
    }

    void SteelseriesController::MapInputReport0x01_v2(const SteelseriesReportData *src) {
        m_left_stick.SetData(
            static_cast<u16>( src->input0x01_v2.left_stick.x + 0x7ff) & UINT12_MAX,
            static_cast<u16>(-src->input0x01_v2.left_stick.y + 0x7ff) & UINT12_MAX
        );
        m_right_stick.SetData(
            static_cast<u16>( src->input0x01_v2.right_stick.x + 0x7ff) & UINT12_MAX,
            static_cast<u16>(-src->input0x01_v2.right_stick.y + 0x7ff) & UINT12_MAX
        );

        m_buttons.dpad_down  = (src->input0x01_v2.dpad == SteelseriesDPad_S)  ||
                               (src->input0x01_v2.dpad == SteelseriesDPad_SE) ||
                               (src->input0x01_v2.dpad == SteelseriesDPad_SW);
        m_buttons.dpad_up    = (src->input0x01_v2.dpad == SteelseriesDPad_N)  ||
                               (src->input0x01_v2.dpad == SteelseriesDPad_NE) ||
                               (src->input0x01_v2.dpad == SteelseriesDPad_NW);
        m_buttons.dpad_right = (src->input0x01_v2.dpad == SteelseriesDPad_E)  ||
                               (src->input0x01_v2.dpad == SteelseriesDPad_NE) ||
                               (src->input0x01_v2.dpad == SteelseriesDPad_SE);
        m_buttons.dpad_left  = (src->input0x01_v2.dpad == SteelseriesDPad_W)  ||
                               (src->input0x01_v2.dpad == SteelseriesDPad_NW) ||
                               (src->input0x01_v2.dpad == SteelseriesDPad_SW);

        m_buttons.A = src->input0x01_v2.buttons.B;
        m_buttons.B = src->input0x01_v2.buttons.A;
        m_buttons.X = src->input0x01_v2.buttons.Y;
        m_buttons.Y = src->input0x01_v2.buttons.X;

        m_buttons.R  = src->input0x01_v2.buttons.R1;
        m_buttons.ZR = src->input0x01_v2.right_trigger > 0x7ff;
        m_buttons.L  = src->input0x01_v2.buttons.L1;
        m_buttons.ZL = src->input0x01_v2.left_trigger  > 0x7ff;

        m_buttons.rstick_press = src->input0x01_v2.buttons.R3;
        m_buttons.lstick_press = src->input0x01_v2.buttons.L3;

        m_buttons.plus = src->input0x01_v2.buttons.start;
    }

    void SteelseriesController::MapInputReport0x02(const SteelseriesReportData *src) {
        m_buttons.minus = src->input0x02.select;
        m_buttons.home = src->input0x02.home;
    }

    void SteelseriesController::MapInputReport0x12(const SteelseriesReportData *src) {
        m_buttons.home = src->input0x12.home;
    }

    void SteelseriesController::MapInputReport0xc4(const SteelseriesReportData *src) {
        m_left_stick.SetData(
            static_cast<u16>(stick_scale_factor * src->input0xc4.left_stick.x) & UINT12_MAX,
            static_cast<u16>(stick_scale_factor * (UINT8_MAX - src->input0xc4.left_stick.y)) & UINT12_MAX
        );
        m_right_stick.SetData(
            static_cast<u16>(stick_scale_factor * src->input0xc4.right_stick.x) & UINT12_MAX,
            static_cast<u16>(stick_scale_factor * (UINT8_MAX - src->input0xc4.right_stick.y)) & UINT12_MAX
        );

        m_buttons.dpad_down  = (src->input0xc4.dpad == SteelseriesDPad2_S)  ||
                               (src->input0xc4.dpad == SteelseriesDPad2_SE) ||
                               (src->input0xc4.dpad == SteelseriesDPad2_SW);
        m_buttons.dpad_up    = (src->input0xc4.dpad == SteelseriesDPad2_N)  ||
                               (src->input0xc4.dpad == SteelseriesDPad2_NE) ||
                               (src->input0xc4.dpad == SteelseriesDPad2_NW);
        m_buttons.dpad_right = (src->input0xc4.dpad == SteelseriesDPad2_E)  ||
                               (src->input0xc4.dpad == SteelseriesDPad2_NE) ||
                               (src->input0xc4.dpad == SteelseriesDPad2_SE);
        m_buttons.dpad_left  = (src->input0xc4.dpad == SteelseriesDPad2_W)  ||
                               (src->input0xc4.dpad == SteelseriesDPad2_NW) ||
                               (src->input0xc4.dpad == SteelseriesDPad2_SW);

        m_buttons.A = src->input0xc4.buttons.B;
        m_buttons.B = src->input0xc4.buttons.A;
        m_buttons.X = src->input0xc4.buttons.Y;
        m_buttons.Y = src->input0xc4.buttons.X;

        m_buttons.R  = src->input0xc4.buttons.R1;
        m_buttons.ZR = src->input0xc4.right_trigger > (m_trigger_threshold * UINT8_MAX);
        m_buttons.L  = src->input0xc4.buttons.L1;
        m_buttons.ZL = src->input0xc4.left_trigger  > (m_trigger_threshold * UINT8_MAX);

        m_buttons.lstick_press = src->input0xc4.buttons.L3;
        m_buttons.rstick_press = src->input0xc4.buttons.R3;   

        m_buttons.minus = src->input0xc4.buttons.select;
        m_buttons.plus  = src->input0xc4.buttons.start;
    }

    void SteelseriesController::MapMfiInputReport(const SteelseriesReportData *src) {
        m_left_stick.SetData(
            static_cast<u16>(stick_scale_factor * -static_cast<s8>(~src->input_mfi.left_stick.x + 1) + 0x7ff) & UINT12_MAX,
            static_cast<u16>(stick_scale_factor * (-static_cast<s8>(~src->input_mfi.left_stick.y + 1)) + 0x7ff) & UINT12_MAX
        );
        m_right_stick.SetData(
            static_cast<u16>(stick_scale_factor * -static_cast<s8>(~src->input_mfi.right_stick.x + 1) + 0x7ff) & UINT12_MAX,
            static_cast<u16>(stick_scale_factor * (-static_cast<s8>(~src->input_mfi.right_stick.y + 1)) + 0x7ff) & UINT12_MAX
        );

        m_buttons.dpad_up    = src->input_mfi.buttons.dpad_up > 0;
        m_buttons.dpad_right = src->input_mfi.buttons.dpad_right > 0;
        m_buttons.dpad_down  = src->input_mfi.buttons.dpad_down > 0;
        m_buttons.dpad_left  = src->input_mfi.buttons.dpad_left > 0;

        m_buttons.A = src->input_mfi.buttons.A > 0;
        m_buttons.B = src->input_mfi.buttons.B > 0;
        m_buttons.X = src->input_mfi.buttons.X > 0;
        m_buttons.Y = src->input_mfi.buttons.Y > 0;

        m_buttons.R  = src->input_mfi.buttons.R1 > 0;
        m_buttons.L  = src->input_mfi.buttons.L1 > 0;
        m_buttons.ZR = src->input_mfi.buttons.R2 > 0;
        m_buttons.ZL = src->input_mfi.buttons.L2 > 0;

        m_buttons.home = src->input_mfi.buttons.menu;
    }

}
