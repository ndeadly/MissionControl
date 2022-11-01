/*
 * Copyright (c) 2020-2022 ndeadly
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
#include "dualshock4_controller.hpp"
#include "../mcmitm_config.hpp"
#include <switch.h>
#include <stratosphere.hpp>

namespace ams::controller {

    namespace {

        const constexpr float stick_scale_factor = float(UINT12_MAX) / UINT8_MAX;

        constexpr float accel_scale_factor = 65535 / 16000.0f * 1000;
        constexpr float gyro_scale_factor = 65535 / (13371 * 360.0f) * 1000;

        const constexpr RGBColour led_disable = {0x00, 0x00, 0x00};

        const RGBColour player_led_colours[] = {
            // Same colours used by PS4
            {0x00, 0x00, 0x40}, // blue
            {0x40, 0x00, 0x00}, // red
            {0x00, 0x40, 0x00}, // green
            {0x20, 0x00, 0x20}, // pink
            // New colours for controllers 5-8
            {0x00, 0x20, 0x20}, // cyan
            {0x30, 0x10, 0x00}, // orange
            {0x20, 0x20, 0x00}, // yellow
            {0x10, 0x00, 0x30}  // purple
        };

        constexpr uint32_t crc_seed = 0xB758EC66;  // CRC32 of {0xa2, 0x11} bytes at beginning of output report

    }

    Result Dualshock4Controller::Initialize() {
        R_TRY(this->PushRumbleLedState());
        R_TRY(EmulatedSwitchController::Initialize());

        // Request motion calibration data from Dualshock4
        if(R_FAILED(this->GetCalibrationData(&m_motion_calibration))) {
            m_enable_motion = false;
        }

        return ams::ResultSuccess();
    }

    Result Dualshock4Controller::SetVibration(const SwitchRumbleData *rumble_data) {
        m_rumble_state.amp_motor_left  = static_cast<uint8_t>(255 * std::max(rumble_data[0].low_band_amp, rumble_data[1].low_band_amp));
        m_rumble_state.amp_motor_right = static_cast<uint8_t>(255 * std::max(rumble_data[0].high_band_amp, rumble_data[1].high_band_amp));
        return this->PushRumbleLedState();
    }

    Result Dualshock4Controller::CancelVibration() {
        m_rumble_state.amp_motor_left = 0;
        m_rumble_state.amp_motor_right = 0;
        return this->PushRumbleLedState();
    }

    Result Dualshock4Controller::SetPlayerLed(uint8_t led_mask) {
        uint8_t player_number;
        R_TRY(LedsMaskToPlayerNumber(led_mask, &player_number));
        RGBColour colour = player_led_colours[player_number];
        return this->SetLightbarColour(colour);
    }

    Result Dualshock4Controller::SetLightbarColour(RGBColour colour) {
        auto config = mitm::GetGlobalConfig();
        m_led_colour = config->misc.enable_dualshock4_lightbar ? colour : led_disable;
        return this->PushRumbleLedState();
    }

    void Dualshock4Controller::ProcessInputData(const bluetooth::HidReport *report) {
        auto ds4_report = reinterpret_cast<const Dualshock4ReportData *>(&report->data);

        switch(ds4_report->id) {
            case 0x01:
                this->MapInputReport0x01(ds4_report); break;
            case 0x11:
                this->MapInputReport0x11(ds4_report); break;
            default:
                break;
        }
    }

    void Dualshock4Controller::MapInputReport0x01(const Dualshock4ReportData *src) {       
        m_left_stick.SetData(
            static_cast<uint16_t>(stick_scale_factor * src->input0x01.left_stick.x) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT8_MAX - src->input0x01.left_stick.y)) & 0xfff
        );
        m_right_stick.SetData(
            static_cast<uint16_t>(stick_scale_factor * src->input0x01.right_stick.x) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT8_MAX - src->input0x01.right_stick.y)) & 0xfff
        );

        this->MapButtons(&src->input0x01.buttons);
    }

    void Dualshock4Controller::MapInputReport0x11(const Dualshock4ReportData *src) {
        m_ext_power = src->input0x11.usb;

        if (!src->input0x11.usb || src->input0x11.battery_level > 10)
            m_charging = false;
        else
            m_charging = true;

        uint8_t battery_level = src->input0x11.battery_level;
        if (!src->input0x11.usb)
            battery_level++;
        if (battery_level > 10)
            battery_level = 10;

        m_battery = static_cast<uint8_t>(8 * (battery_level + 2) / 10) & 0x0e;

        m_left_stick.SetData(
            static_cast<uint16_t>(stick_scale_factor * src->input0x11.left_stick.x) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT8_MAX - src->input0x11.left_stick.y)) & 0xfff
        );
        m_right_stick.SetData(
            static_cast<uint16_t>(stick_scale_factor * src->input0x11.right_stick.x) & 0xfff,
            static_cast<uint16_t>(stick_scale_factor * (UINT8_MAX - src->input0x11.right_stick.y)) & 0xfff
        );

        this->MapButtons(&src->input0x11.buttons);

        if (m_enable_motion) {
            int16_t acc_x = -static_cast<int16_t>(accel_scale_factor * src->input0x11.acc_z / float(m_motion_calibration.acc.z_max));
            int16_t acc_y = -static_cast<int16_t>(accel_scale_factor * src->input0x11.acc_x / float(m_motion_calibration.acc.x_max));
            int16_t acc_z =  static_cast<int16_t>(accel_scale_factor * src->input0x11.acc_y / float(m_motion_calibration.acc.y_max));

            int16_t vel_x = -static_cast<int16_t>(0.85 * gyro_scale_factor * (src->input0x11.vel_z - m_motion_calibration.gyro.roll_bias)  / ((m_motion_calibration.gyro.roll_max - m_motion_calibration.gyro.roll_bias) / m_motion_calibration.gyro.speed_max));
            int16_t vel_y = -static_cast<int16_t>(0.85 * gyro_scale_factor * (src->input0x11.vel_x - m_motion_calibration.gyro.pitch_bias) / ((m_motion_calibration.gyro.pitch_max - m_motion_calibration.gyro.pitch_bias) / m_motion_calibration.gyro.speed_max));
            int16_t vel_z =  static_cast<int16_t>(0.85 * gyro_scale_factor * (src->input0x11.vel_y - m_motion_calibration.gyro.yaw_bias)   / ((m_motion_calibration.gyro.yaw_max- m_motion_calibration.gyro.yaw_bias) / m_motion_calibration.gyro.speed_max));

            m_motion_data[0].gyro_1  = vel_x;
            m_motion_data[0].gyro_2  = vel_y;
            m_motion_data[0].gyro_3  = vel_z;
            m_motion_data[0].accel_x = acc_x;
            m_motion_data[0].accel_y = acc_y;
            m_motion_data[0].accel_z = acc_z;

            m_motion_data[1].gyro_1  = vel_x;
            m_motion_data[1].gyro_2  = vel_y;
            m_motion_data[1].gyro_3  = vel_z;
            m_motion_data[1].accel_x = acc_x;
            m_motion_data[1].accel_y = acc_y;
            m_motion_data[1].accel_z = acc_z;

            m_motion_data[2].gyro_1  = vel_x;
            m_motion_data[2].gyro_2  = vel_y;
            m_motion_data[2].gyro_3  = vel_z;
            m_motion_data[2].accel_x = acc_x;
            m_motion_data[2].accel_y = acc_y;
            m_motion_data[2].accel_z = acc_z;
        }
        else {
            std::memset(&m_motion_data, 0, sizeof(m_motion_data));
        }
    }

    void Dualshock4Controller::MapButtons(const Dualshock4ButtonData *buttons) {
        m_buttons.dpad_down  = (buttons->dpad == Dualshock4DPad_S)  ||
                               (buttons->dpad == Dualshock4DPad_SE) ||
                               (buttons->dpad == Dualshock4DPad_SW);
        m_buttons.dpad_up    = (buttons->dpad == Dualshock4DPad_N)  ||
                               (buttons->dpad == Dualshock4DPad_NE) ||
                               (buttons->dpad == Dualshock4DPad_NW);
        m_buttons.dpad_right = (buttons->dpad == Dualshock4DPad_E)  ||
                               (buttons->dpad == Dualshock4DPad_NE) ||
                               (buttons->dpad == Dualshock4DPad_SE);
        m_buttons.dpad_left  = (buttons->dpad == Dualshock4DPad_W)  ||
                               (buttons->dpad == Dualshock4DPad_NW) ||
                               (buttons->dpad == Dualshock4DPad_SW);

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

    Result Dualshock4Controller::GetVersionInfo(Dualshock4VersionInfo *version_info) {
        bluetooth::HidReport output;
        R_TRY(this->GetFeatureReport(0x06, &output));

        auto response = reinterpret_cast<Dualshock4ReportData *>(&output.data);
        std::memcpy(version_info, &response->feature0x06.version_info, sizeof(Dualshock4VersionInfo));

        return ams::ResultSuccess();
    }

    Result Dualshock4Controller::GetCalibrationData(Dualshock4ImuCalibrationData *calibration) {
        bluetooth::HidReport output;
        R_TRY(this->GetFeatureReport(0x05, &output));

        auto response = reinterpret_cast<Dualshock4ReportData *>(&output.data);
        std::memcpy(calibration, &response->feature0x05.calibration, sizeof(Dualshock4ImuCalibrationData));

        return ams::ResultSuccess();
    }

    Result Dualshock4Controller::PushRumbleLedState() {
        std::scoped_lock lk(m_output_mutex);

        Dualshock4ReportData report = {};
        report.id = 0x11;
        report.output0x11.data[0] = static_cast<uint8_t>(0xc0 | (m_report_rate & 0xff));
        report.output0x11.data[1] = 0x20;
        report.output0x11.data[2] = 0xf3;
        report.output0x11.data[3] = 0x04;
        report.output0x11.data[5] = m_rumble_state.amp_motor_right;
        report.output0x11.data[6] = m_rumble_state.amp_motor_left;
        report.output0x11.data[7] = m_led_colour.r;
        report.output0x11.data[8] = m_led_colour.g;
        report.output0x11.data[9] = m_led_colour.b;
        report.output0x11.crc = crc32CalculateWithSeed(crc_seed, report.output0x11.data, sizeof(report.output0x11.data));

        m_output_report.size = sizeof(report.output0x11) + sizeof(report.id);
        std::memcpy(m_output_report.data, &report, m_output_report.size);

        return this->WriteDataReport(&m_output_report);
    }

}
