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
#include "xbox_one_controller.hpp"
#include <stratosphere.hpp>
#include <cstring>

namespace ams::controller {

    namespace {

        constexpr uint8_t init_packet[] = {0x05, 0x20, 0x00, 0x01, 0x00};
        
        constexpr float stick_scale_factor = float(UINT12_MAX) / UINT16_MAX;

    }

    Result XboxOneController::Initialize(void) {
        R_TRY(EmulatedSwitchController::Initialize());
        s_output_report.size = sizeof(init_packet);
        std::memcpy(s_output_report.data, init_packet, sizeof(init_packet));
        R_TRY(bluetooth::hid::report::SendHidReport(&m_address, &s_output_report));
        return ams::ResultSuccess();
    }

    void XboxOneController::ConvertReportFormat(const bluetooth::HidReport *in_report, bluetooth::HidReport *out_report) {
        auto xbox_report = reinterpret_cast<const XboxOneReportData *>(&in_report->data);
        auto switch_report = reinterpret_cast<SwitchReportData *>(&out_report->data);

        switch(xbox_report->id) {
            case 0x01:
                this->HandleInputReport0x01(xbox_report, switch_report);
                break;
            case 0x04:
                this->HandleInputReport0x04(xbox_report, switch_report);
                break;
            default:
                break;
        }

        out_report->size = sizeof(SwitchInputReport0x30) + 1;
        switch_report->id = 0x30;
        switch_report->input0x30.conn_info = 0x0;
        switch_report->input0x30.battery = m_battery | m_charging;
        std::memset(switch_report->input0x30.motion, 0, sizeof(switch_report->input0x30.motion));
        switch_report->input0x30.timer = os::ConvertToTimeSpan(os::GetSystemTick()).GetMilliSeconds() & 0xff;
    }

    void XboxOneController::HandleInputReport0x01(const XboxOneReportData *src, SwitchReportData *dst) {
        this->PackStickData(&dst->input0x30.left_stick,
            static_cast<uint16_t>(stick_scale_factor * src->input0x01.left_stick.x) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT16_MAX - src->input0x01.left_stick.y)) & 0xfff
        );
        this->PackStickData(&dst->input0x30.right_stick,
            static_cast<uint16_t>(stick_scale_factor * src->input0x01.right_stick.x) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT16_MAX - src->input0x01.right_stick.y)) & 0xfff
        );

        dst->input0x30.buttons.dpad_down   = (src->input0x01.buttons.dpad == XboxOneDPad_S)  ||
                                             (src->input0x01.buttons.dpad == XboxOneDPad_SE) ||
                                             (src->input0x01.buttons.dpad == XboxOneDPad_SW);
        dst->input0x30.buttons.dpad_up     = (src->input0x01.buttons.dpad == XboxOneDPad_N)  ||
                                             (src->input0x01.buttons.dpad == XboxOneDPad_NE) ||
                                             (src->input0x01.buttons.dpad == XboxOneDPad_NW);
        dst->input0x30.buttons.dpad_right  = (src->input0x01.buttons.dpad == XboxOneDPad_E)  ||
                                             (src->input0x01.buttons.dpad == XboxOneDPad_NE) ||
                                             (src->input0x01.buttons.dpad == XboxOneDPad_SE);
        dst->input0x30.buttons.dpad_left   = (src->input0x01.buttons.dpad == XboxOneDPad_W)  ||
                                             (src->input0x01.buttons.dpad == XboxOneDPad_NW) ||
                                             (src->input0x01.buttons.dpad == XboxOneDPad_SW);

        dst->input0x30.buttons.A = src->input0x01.buttons.B;
        dst->input0x30.buttons.B = src->input0x01.buttons.A;
        dst->input0x30.buttons.X = src->input0x01.buttons.Y;
        dst->input0x30.buttons.Y = src->input0x01.buttons.X;

        dst->input0x30.buttons.R  = src->input0x01.buttons.RB;
        dst->input0x30.buttons.ZR = src->input0x01.right_trigger > 0;
        dst->input0x30.buttons.L  = src->input0x01.buttons.LB;
        dst->input0x30.buttons.ZL = src->input0x01.left_trigger > 0;

        dst->input0x30.buttons.minus = src->input0x01.buttons.view;
        dst->input0x30.buttons.plus  = src->input0x01.buttons.menu;

        dst->input0x30.buttons.lstick_press = src->input0x01.buttons.lstick_press;
        dst->input0x30.buttons.rstick_press = src->input0x01.buttons.rstick_press;

        dst->input0x30.buttons.capture  = 0;
        dst->input0x30.buttons.home     = src->input0x01.buttons.guide;
    }

    void XboxOneController::HandleInputReport0x04(const XboxOneReportData *src, SwitchReportData *dst) {
        m_battery = src->input0x04.capacity << 1;
        m_charging = src->input0x04.charging;

        this->PackStickData(&dst->input0x30.left_stick, STICK_ZERO, STICK_ZERO);
        this->PackStickData(&dst->input0x30.right_stick, STICK_ZERO, STICK_ZERO);
    }

}
