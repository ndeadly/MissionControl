/*
 * Copyright (c) 2020 ndeadly
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

    void XboxOneController::UpdateControllerState(const bluetooth::HidReport *report) {
        auto xbox_report = reinterpret_cast<const XboxOneReportData *>(&report->data);

        switch(xbox_report->id) {
            case 0x01:
                this->HandleInputReport0x01(xbox_report);
                break;
            case 0x04:
                this->HandleInputReport0x04(xbox_report);
                break;
            default:
                break;
        }
    }

    void XboxOneController::HandleInputReport0x01(const XboxOneReportData *src) {
        this->PackStickData(&m_left_stick,
            static_cast<uint16_t>(stick_scale_factor * src->input0x01.left_stick.x) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT16_MAX - src->input0x01.left_stick.y)) & 0xfff
        );
        this->PackStickData(&m_right_stick,
            static_cast<uint16_t>(stick_scale_factor * src->input0x01.right_stick.x) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT16_MAX - src->input0x01.right_stick.y)) & 0xfff
        );

        m_buttons.dpad_down   = (src->input0x01.buttons.dpad == XboxOneDPad_S)  ||
                                (src->input0x01.buttons.dpad == XboxOneDPad_SE) ||
                                (src->input0x01.buttons.dpad == XboxOneDPad_SW);
        m_buttons.dpad_up     = (src->input0x01.buttons.dpad == XboxOneDPad_N)  ||
                                (src->input0x01.buttons.dpad == XboxOneDPad_NE) ||
                                (src->input0x01.buttons.dpad == XboxOneDPad_NW);
        m_buttons.dpad_right  = (src->input0x01.buttons.dpad == XboxOneDPad_E)  ||
                                (src->input0x01.buttons.dpad == XboxOneDPad_NE) ||
                                (src->input0x01.buttons.dpad == XboxOneDPad_SE);
        m_buttons.dpad_left   = (src->input0x01.buttons.dpad == XboxOneDPad_W)  ||
                                (src->input0x01.buttons.dpad == XboxOneDPad_NW) ||
                                (src->input0x01.buttons.dpad == XboxOneDPad_SW);

        m_buttons.A = src->input0x01.buttons.B;
        m_buttons.B = src->input0x01.buttons.A;
        m_buttons.X = src->input0x01.buttons.Y;
        m_buttons.Y = src->input0x01.buttons.X;

        m_buttons.R  = src->input0x01.buttons.RB;
        m_buttons.ZR = src->input0x01.right_trigger > 0;
        m_buttons.L  = src->input0x01.buttons.LB;
        m_buttons.ZL = src->input0x01.left_trigger > 0;

        m_buttons.minus = src->input0x01.buttons.view;
        m_buttons.plus  = src->input0x01.buttons.menu;

        m_buttons.lstick_press = src->input0x01.buttons.lstick_press;
        m_buttons.rstick_press = src->input0x01.buttons.rstick_press;

        m_buttons.home     = src->input0x01.buttons.guide;
    }

    void XboxOneController::HandleInputReport0x04(const XboxOneReportData *src) {
        m_battery = src->input0x04.capacity << 1;
        m_charging = src->input0x04.charging;
    }

}
