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
#include "xiaomi_controller.hpp"
#include "controller_utils.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    namespace {

        constexpr u8 init_packet[] = {0x20, 0x00, 0x00};  // packet to init vibration apparently

        const constexpr float stick_scale_factor = float(UINT12_MAX) / UINT8_MAX;

    }

    Result XiaomiController::Initialize() {
        R_TRY(EmulatedSwitchController::Initialize());

        std::scoped_lock lk(m_output_mutex);

        m_output_report.size = sizeof(init_packet);
        std::memcpy(m_output_report.data, init_packet, sizeof(init_packet));
        R_TRY(this->WriteDataReport(&m_output_report));

        R_SUCCEED();
    }

    void XiaomiController::ProcessInputData(const bluetooth::HidReport *report) {
        auto xiaomi_report = reinterpret_cast<const XiaomiReportData *>(&report->data);

        switch(xiaomi_report->id) {
            case 0x04:
                this->MapInputReport0x04(xiaomi_report); break;
            default:
                break;
        }
    }

    void XiaomiController::MapInputReport0x04(const XiaomiReportData *src) {
        m_battery = convert_battery_100(src->input0x04.battery);

        m_left_stick.SetData(
            static_cast<u16>(stick_scale_factor * src->input0x04.left_stick.x) & UINT12_MAX,
            static_cast<u16>(stick_scale_factor * (UINT8_MAX - src->input0x04.left_stick.y)) & UINT12_MAX
        );
        m_right_stick.SetData(
            static_cast<u16>(stick_scale_factor * src->input0x04.right_stick.x) & UINT12_MAX,
            static_cast<u16>(stick_scale_factor * (UINT8_MAX - src->input0x04.right_stick.y)) & UINT12_MAX
        );

        m_buttons.dpad_down  = (src->input0x04.buttons.dpad == XiaomiDPad_S)  ||
                               (src->input0x04.buttons.dpad == XiaomiDPad_SE) ||
                               (src->input0x04.buttons.dpad == XiaomiDPad_SW);
        m_buttons.dpad_up    = (src->input0x04.buttons.dpad == XiaomiDPad_N)  ||
                               (src->input0x04.buttons.dpad == XiaomiDPad_NE) ||
                               (src->input0x04.buttons.dpad == XiaomiDPad_NW);
        m_buttons.dpad_right = (src->input0x04.buttons.dpad == XiaomiDPad_E)  ||
                               (src->input0x04.buttons.dpad == XiaomiDPad_NE) ||
                               (src->input0x04.buttons.dpad == XiaomiDPad_SE);
        m_buttons.dpad_left  = (src->input0x04.buttons.dpad == XiaomiDPad_W)  ||
                               (src->input0x04.buttons.dpad == XiaomiDPad_NW) ||
                               (src->input0x04.buttons.dpad == XiaomiDPad_SW);

        m_buttons.A = src->input0x04.buttons.B;
        m_buttons.B = src->input0x04.buttons.A;
        m_buttons.X = src->input0x04.buttons.Y;
        m_buttons.Y = src->input0x04.buttons.X;

        m_buttons.R  = src->input0x04.buttons.R1;
        m_buttons.ZR = src->input0x04.right_trigger > (m_trigger_threshold * UINT8_MAX);
        m_buttons.L  = src->input0x04.buttons.L1;
        m_buttons.ZL = src->input0x04.left_trigger  > (m_trigger_threshold * UINT8_MAX);

        m_buttons.minus = src->input0x04.buttons.back;
        m_buttons.plus  = src->input0x04.buttons.menu;

        m_buttons.lstick_press = src->input0x04.buttons.lstick_press;
        m_buttons.rstick_press = src->input0x04.buttons.rstick_press;

        m_buttons.home = src->input0x04.home;
    }

}
