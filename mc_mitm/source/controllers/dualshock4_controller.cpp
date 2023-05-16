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
#include "dualshock4_controller.hpp"
#include "../mcmitm_config.hpp"
#include <switch.h>
#include <stratosphere.hpp>

namespace ams::controller {

    namespace {

        constexpr float stick_scale_factor = float(UINT12_MAX) / UINT8_MAX;

        constexpr float accel_scale_factor = 65535 / 16000.0f * 1000;
        constexpr float gyro_scale_factor = 65535 / (13371 * 360.0f) * 1000;

        constexpr u16 touchpad_width = 1920;
        constexpr u16 touchpad_height = 942;

        const RGBColour player_led_base_colours[] = {
            // Same colours used by PS4
            {0x00, 0x00, 0x04}, // blue
            {0x04, 0x00, 0x00}, // red
            {0x00, 0x04, 0x00}, // green
            {0x02, 0x00, 0x02}, // pink
            // New colours for controllers 5-8
            {0x00, 0x02, 0x02}, // cyan
            {0x03, 0x01, 0x00}, // orange
            {0x02, 0x02, 0x00}, // yellow
            {0x01, 0x00, 0x03}  // purple
        };

        constexpr u8 step = 4;
        const u8 led_brightness_multipliers[] = { 0, 1, 1 * step, 2 * step, 3 * step, 4 * step, 5 * step, 6 * step, 7 * step, 8 * step };

        constexpr u32 crc_seed = 0xB758EC66;  // CRC32 of {0xa2, 0x11} bytes at beginning of output report

    }

    Result Dualshock4Controller::Initialize() {
        auto config = mitm::GetGlobalConfig();
        m_report_rate = static_cast<Dualshock4ReportRate>(config->misc.dualshock4_polling_rate);
        m_lightbar_brightness = config->misc.dualshock4_lightbar_brightness;

        R_TRY(this->PushRumbleLedState());
        R_TRY(EmulatedSwitchController::Initialize());

        // Request motion calibration data from Dualshock4
        if(R_FAILED(this->GetCalibrationData(&m_motion_calibration))) {
            m_enable_motion = false;
        }

        R_SUCCEED();
    }

    Result Dualshock4Controller::SetVibration(const SwitchRumbleData *rumble_data) {
        m_rumble_state.amp_motor_left  = static_cast<u8>(255 * std::max(rumble_data[0].low_band_amp, rumble_data[1].low_band_amp));
        m_rumble_state.amp_motor_right = static_cast<u8>(255 * std::max(rumble_data[0].high_band_amp, rumble_data[1].high_band_amp));
        R_RETURN(this->PushRumbleLedState());
    }

    Result Dualshock4Controller::CancelVibration() {
        m_rumble_state.amp_motor_left = 0;
        m_rumble_state.amp_motor_right = 0;
        R_RETURN(this->PushRumbleLedState());
    }

    Result Dualshock4Controller::SetPlayerLed(u8 led_mask) {
        u8 player_number;
        R_TRY(LedsMaskToPlayerNumber(led_mask, &player_number));
        RGBColour colour = player_led_base_colours[player_number];
        u8 multiplier = led_brightness_multipliers[m_lightbar_brightness];
        colour.r *= multiplier;
        colour.g *= multiplier;
        colour.b *= multiplier;
        R_RETURN(this->SetLightbarColour(colour));
    }

    Result Dualshock4Controller::SetLightbarColour(RGBColour colour) {
        m_lightbar_colour = colour;
        R_RETURN(this->PushRumbleLedState());
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
            static_cast<u16>(stick_scale_factor * src->input0x01.left_stick.x) & UINT12_MAX,
            static_cast<u16>(stick_scale_factor * (UINT8_MAX - src->input0x01.left_stick.y)) & UINT12_MAX
        );
        m_right_stick.SetData(
            static_cast<u16>(stick_scale_factor * src->input0x01.right_stick.x) & UINT12_MAX,
            static_cast<u16>(stick_scale_factor * (UINT8_MAX - src->input0x01.right_stick.y)) & UINT12_MAX
        );

        this->MapButtons(&src->input0x01.buttons);

        m_buttons.ZR = src->input0x01.right_trigger > (m_trigger_threshold * UINT8_MAX);
        m_buttons.ZL = src->input0x01.left_trigger  > (m_trigger_threshold * UINT8_MAX);
    }

    void Dualshock4Controller::MapInputReport0x11(const Dualshock4ReportData *src) {
        m_ext_power = src->input0x11.usb;

        if (!src->input0x11.usb || src->input0x11.battery_level > 10) {
            m_charging = false;
        } else {
            m_charging = true;
        }

        u8 battery_level = src->input0x11.battery_level;
        if (!src->input0x11.usb) {
            battery_level++;
        }
        if (battery_level > 10) {
            battery_level = 10;
        }

        m_battery = static_cast<u8>(8 * (battery_level + 2) / 10) & 0x0e;

        m_left_stick.SetData(
            static_cast<u16>(stick_scale_factor * src->input0x11.left_stick.x) & UINT12_MAX,
            static_cast<u16>(stick_scale_factor * (UINT8_MAX - src->input0x11.left_stick.y)) & UINT12_MAX
        );
        m_right_stick.SetData(
            static_cast<u16>(stick_scale_factor * src->input0x11.right_stick.x) & UINT12_MAX,
            static_cast<u16>(stick_scale_factor * (UINT8_MAX - src->input0x11.right_stick.y)) & UINT12_MAX
        );

        this->MapButtons(&src->input0x11.buttons);

        m_buttons.ZR = src->input0x11.right_trigger > (m_trigger_threshold * UINT8_MAX);
        m_buttons.ZL = src->input0x11.left_trigger  > (m_trigger_threshold * UINT8_MAX);

        if (src->input0x11.buttons.touchpad) {
            for (int i = 0; i < src->input0x11.num_reports; ++i) {
                const Dualshock4TouchReport *touch_report = &src->input0x11.touch_reports[i];
                for (int j = 0; j < 2; ++j) {
                    const Dualshock4TouchpadPoint *point = &touch_report->points[j];

                    bool active = point->contact & BIT(7) ? false : true;
                    if (active) {
                        u16 x = (point->x_hi << 8) | point->x_lo;

                        if (x < (0.15 * touchpad_width)) {
                            m_buttons.minus = 1;
                        } else if (x > (0.85 * touchpad_width)) {
                            m_buttons.plus = 1;
                        } else {
                            m_buttons.capture = 1;
                        }
                    }
                }
            }
        }

        if (m_enable_motion) {
            s16 acc_x = -static_cast<s16>(accel_scale_factor * src->input0x11.acc_z / float(m_motion_calibration.acc.z_max));
            s16 acc_y = -static_cast<s16>(accel_scale_factor * src->input0x11.acc_x / float(m_motion_calibration.acc.x_max));
            s16 acc_z =  static_cast<s16>(accel_scale_factor * src->input0x11.acc_y / float(m_motion_calibration.acc.y_max));

            s16 vel_x = -static_cast<s16>(0.85 * gyro_scale_factor * (src->input0x11.vel_z - m_motion_calibration.gyro.roll_bias)  / ((m_motion_calibration.gyro.roll_max - m_motion_calibration.gyro.roll_bias) / m_motion_calibration.gyro.speed_max));
            s16 vel_y = -static_cast<s16>(0.85 * gyro_scale_factor * (src->input0x11.vel_x - m_motion_calibration.gyro.pitch_bias) / ((m_motion_calibration.gyro.pitch_max - m_motion_calibration.gyro.pitch_bias) / m_motion_calibration.gyro.speed_max));
            s16 vel_z =  static_cast<s16>(0.85 * gyro_scale_factor * (src->input0x11.vel_y - m_motion_calibration.gyro.yaw_bias)   / ((m_motion_calibration.gyro.yaw_max- m_motion_calibration.gyro.yaw_bias) / m_motion_calibration.gyro.speed_max));

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
        } else {
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

        m_buttons.home    = buttons->ps;
    }

    Result Dualshock4Controller::GetVersionInfo(Dualshock4VersionInfo *version_info) {
        bluetooth::HidReport output;
        R_TRY(this->GetReport(0x06, BtdrvBluetoothHhReportType_Feature, &output));

        auto response = reinterpret_cast<Dualshock4ReportData *>(&output.data);
        std::memcpy(version_info, &response->feature0x06.version_info, sizeof(Dualshock4VersionInfo));

        R_SUCCEED();
    }

    Result Dualshock4Controller::GetCalibrationData(Dualshock4ImuCalibrationData *calibration) {
        bluetooth::HidReport output;
        R_TRY(this->GetReport(0x05, BtdrvBluetoothHhReportType_Feature, &output));

        auto response = reinterpret_cast<Dualshock4ReportData *>(&output.data);
        std::memcpy(calibration, &response->feature0x05.calibration, sizeof(Dualshock4ImuCalibrationData));

        R_SUCCEED();
    }

    Result Dualshock4Controller::PushRumbleLedState() {
        std::scoped_lock lk(m_output_mutex);

        Dualshock4ReportData report = {};
        report.id = 0x11;
        report.output0x11.data[0] = static_cast<u8>(0xc0 | (m_report_rate & 0xff));
        report.output0x11.data[1] = 0x20;
        report.output0x11.data[2] = 0xf3;
        report.output0x11.data[3] = 0x04;
        report.output0x11.data[5] = m_rumble_state.amp_motor_right;
        report.output0x11.data[6] = m_rumble_state.amp_motor_left;
        report.output0x11.data[7] = m_lightbar_colour.r;
        report.output0x11.data[8] = m_lightbar_colour.g;
        report.output0x11.data[9] = m_lightbar_colour.b;
        report.output0x11.crc = crc32CalculateWithSeed(crc_seed, report.output0x11.data, sizeof(report.output0x11.data));

        m_output_report.size = sizeof(report.output0x11) + sizeof(report.id);
        std::memcpy(m_output_report.data, &report, m_output_report.size);

        R_RETURN(this->WriteDataReport(&m_output_report));
    }

}
