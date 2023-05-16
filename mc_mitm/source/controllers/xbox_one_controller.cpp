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
#include "xbox_one_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    namespace {

        constexpr float stick_scale_factor = float(UINT12_MAX) / UINT16_MAX;

    }

    Result XboxOneController::SetVibration(const SwitchRumbleData *rumble_data) {
        auto report = reinterpret_cast<XboxOneReportData *>(m_output_report.data);
        m_output_report.size = sizeof(XboxOneOutputReport0x03) + 1;
        report->id = 0x03;
        report->output0x03.enable             = 0x3;
        report->output0x03.magnitude_strong   = static_cast<u8>(100 * std::max(rumble_data[0].low_band_amp, rumble_data[1].low_band_amp));
        report->output0x03.magnitude_weak     = static_cast<u8>(100 * std::max(rumble_data[0].high_band_amp, rumble_data[1].high_band_amp));
        report->output0x03.pulse_sustain_10ms = 1;
        report->output0x03.pulse_release_10ms = 0;
        report->output0x03.loop_count         = 0;

        return this->WriteDataReport(&m_output_report);
    }

    void XboxOneController::ProcessInputData(const bluetooth::HidReport *report) {
        auto xbox_report = reinterpret_cast<const XboxOneReportData *>(&report->data);

        switch(xbox_report->id) {
            case 0x01:
                this->MapInputReport0x01(xbox_report, report->size >= sizeof(XboxOneInputReport0x01) + 1); break;
            case 0x02:
                this->MapInputReport0x02(xbox_report); break;
            case 0x04:
                this->MapInputReport0x04(xbox_report); break;
            default:
                break;
        }
    }

    void XboxOneController::MapInputReport0x01(const XboxOneReportData *src, bool new_format) {
        m_left_stick.SetData(
            static_cast<u16>(stick_scale_factor * src->input0x01.left_stick.x) & UINT12_MAX,
            static_cast<u16>(stick_scale_factor * (UINT16_MAX - src->input0x01.left_stick.y)) & UINT12_MAX
        );
        m_right_stick.SetData(
            static_cast<u16>(stick_scale_factor * src->input0x01.right_stick.x) & UINT12_MAX,
            static_cast<u16>(stick_scale_factor * (UINT16_MAX - src->input0x01.right_stick.y)) & UINT12_MAX
        );

        m_buttons.ZR = src->input0x01.right_trigger > (m_trigger_threshold * 0x3ff);
        m_buttons.ZL = src->input0x01.left_trigger  > (m_trigger_threshold * 0x3ff);

        if (new_format) {
            m_buttons.dpad_down  = (src->input0x01.buttons.dpad == XboxOneDPad_S)  ||
                                   (src->input0x01.buttons.dpad == XboxOneDPad_SE) ||
                                   (src->input0x01.buttons.dpad == XboxOneDPad_SW);
            m_buttons.dpad_up    = (src->input0x01.buttons.dpad == XboxOneDPad_N)  ||
                                   (src->input0x01.buttons.dpad == XboxOneDPad_NE) ||
                                   (src->input0x01.buttons.dpad == XboxOneDPad_NW);
            m_buttons.dpad_right = (src->input0x01.buttons.dpad == XboxOneDPad_E)  ||
                                   (src->input0x01.buttons.dpad == XboxOneDPad_NE) ||
                                   (src->input0x01.buttons.dpad == XboxOneDPad_SE);
            m_buttons.dpad_left  = (src->input0x01.buttons.dpad == XboxOneDPad_W)  ||
                                   (src->input0x01.buttons.dpad == XboxOneDPad_NW) ||
                                   (src->input0x01.buttons.dpad == XboxOneDPad_SW);

            m_buttons.A = src->input0x01.buttons.B;
            m_buttons.B = src->input0x01.buttons.A;
            m_buttons.X = src->input0x01.buttons.Y;
            m_buttons.Y = src->input0x01.buttons.X;

            m_buttons.R = src->input0x01.buttons.RB;
            m_buttons.L = src->input0x01.buttons.LB;

            m_buttons.minus = src->input0x01.buttons.view;
            m_buttons.plus  = src->input0x01.buttons.menu;

            m_buttons.lstick_press = src->input0x01.buttons.lstick_press;
            m_buttons.rstick_press = src->input0x01.buttons.rstick_press;

            m_buttons.home = src->input0x01.buttons.guide;
        } else {
            m_buttons.dpad_down  = (src->input0x01.old.buttons.dpad == XboxOneDPad_S)  ||
                                   (src->input0x01.old.buttons.dpad == XboxOneDPad_SE) ||
                                   (src->input0x01.old.buttons.dpad == XboxOneDPad_SW);
            m_buttons.dpad_up    = (src->input0x01.old.buttons.dpad == XboxOneDPad_N)  ||
                                   (src->input0x01.old.buttons.dpad == XboxOneDPad_NE) ||
                                   (src->input0x01.old.buttons.dpad == XboxOneDPad_NW);
            m_buttons.dpad_right = (src->input0x01.old.buttons.dpad == XboxOneDPad_E)  ||
                                   (src->input0x01.old.buttons.dpad == XboxOneDPad_NE) ||
                                   (src->input0x01.old.buttons.dpad == XboxOneDPad_SE);
            m_buttons.dpad_left  = (src->input0x01.old.buttons.dpad == XboxOneDPad_W)  ||
                                   (src->input0x01.old.buttons.dpad == XboxOneDPad_NW) ||
                                   (src->input0x01.old.buttons.dpad == XboxOneDPad_SW);

            m_buttons.A = src->input0x01.old.buttons.B;
            m_buttons.B = src->input0x01.old.buttons.A;
            m_buttons.X = src->input0x01.old.buttons.Y;
            m_buttons.Y = src->input0x01.old.buttons.X;

            m_buttons.R = src->input0x01.old.buttons.RB;
            m_buttons.L = src->input0x01.old.buttons.LB;

            m_buttons.minus = src->input0x01.old.buttons.view;
            m_buttons.plus  = src->input0x01.old.buttons.menu;

            m_buttons.lstick_press = src->input0x01.old.buttons.lstick_press;
            m_buttons.rstick_press = src->input0x01.old.buttons.rstick_press;
        }
    }

    void XboxOneController::MapInputReport0x02(const XboxOneReportData *src) {
        m_buttons.home = src->input0x02.guide;
    }

    void XboxOneController::MapInputReport0x04(const XboxOneReportData *src) {
        m_ext_power = src->input0x04.mode != XboxOnePowerMode_Battery;
        m_battery = (src->input0x04.mode == XboxOnePowerMode_USB) ? BATTERY_MAX : src->input0x04.capacity << 1;
        m_charging = src->input0x04.charging;
    }

}
