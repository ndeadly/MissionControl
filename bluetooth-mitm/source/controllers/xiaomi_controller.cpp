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
#include "xiaomi_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    namespace {

        constexpr uint8_t init_packet[] = {0x20, 0x00, 0x00};  // packet to init vibration apparently

        const constexpr float stick_scale_factor = float(UINT12_MAX) / UINT8_MAX;

    }

    Result XiaomiController::Initialize(void) {
        R_TRY(EmulatedSwitchController::Initialize());
        s_output_report.size = sizeof(init_packet);
        std::memcpy(s_output_report.data, init_packet, sizeof(init_packet));
        R_TRY(bluetooth::hid::report::SendHidReport(&m_address, &s_output_report));
        return ams::ResultSuccess();    
    }
    
    void XiaomiController::UpdateControllerState(const bluetooth::HidReport *report) {
        auto xiaomi_report = reinterpret_cast<const XiaomiReportData *>(&report->data);

        switch(xiaomi_report->id) {
            case 0x04:
                this->HandleInputReport0x04(xiaomi_report);
                break;
            default:
                break;
        }
    }

    void XiaomiController::HandleInputReport0x04(const XiaomiReportData *src) {
        m_battery = src->input0x04.battery / 52 << 1;

        this->PackStickData(&m_left_stick,
            static_cast<uint16_t>(stick_scale_factor * src->input0x04.left_stick.x) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT8_MAX - src->input0x04.left_stick.y)) & 0xfff
        );
        this->PackStickData(&m_right_stick,
            static_cast<uint16_t>(stick_scale_factor * src->input0x04.right_stick.x) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT8_MAX - src->input0x04.right_stick.y)) & 0xfff
        );
        
        m_buttons.dpad_down   = (src->input0x04.buttons.dpad == XiaomiDPad_S)  ||
                                (src->input0x04.buttons.dpad == XiaomiDPad_SE) ||
                                (src->input0x04.buttons.dpad == XiaomiDPad_SW);
        m_buttons.dpad_up     = (src->input0x04.buttons.dpad == XiaomiDPad_N)  ||
                                (src->input0x04.buttons.dpad == XiaomiDPad_NE) ||
                                (src->input0x04.buttons.dpad == XiaomiDPad_NW);
        m_buttons.dpad_right  = (src->input0x04.buttons.dpad == XiaomiDPad_E)  ||
                                (src->input0x04.buttons.dpad == XiaomiDPad_NE) ||
                                (src->input0x04.buttons.dpad == XiaomiDPad_SE);
        m_buttons.dpad_left   = (src->input0x04.buttons.dpad == XiaomiDPad_W)  ||
                                (src->input0x04.buttons.dpad == XiaomiDPad_NW) ||
                                (src->input0x04.buttons.dpad == XiaomiDPad_SW);

        m_buttons.A = src->input0x04.buttons.B;
        m_buttons.B = src->input0x04.buttons.A;
        m_buttons.X = src->input0x04.buttons.Y;
        m_buttons.Y = src->input0x04.buttons.X;

        m_buttons.R  = src->input0x04.buttons.R1;
        m_buttons.ZR = src->input0x04.buttons.R2;
        m_buttons.L  = src->input0x04.buttons.L1;
        m_buttons.ZL = src->input0x04.buttons.L2; 

        m_buttons.minus = src->input0x04.buttons.back;
        m_buttons.plus  = src->input0x04.buttons.menu;

        m_buttons.lstick_press = src->input0x04.buttons.lstick_press;
        m_buttons.rstick_press = src->input0x04.buttons.rstick_press;    

        m_buttons.home     = src->input0x04.home;
    }

}
