/*
 * Copyright (c) 2020-2025 ndeadly
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
#include "dualsense_controller.hpp"
#include "../mcmitm_config.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    namespace {

        constexpr u8 TriggerMax = UINT8_MAX;

        constexpr u16 TouchpadWidth = 1920;
        constexpr u16 TouchpadHeight = 1080;

        constinit const u8 PlayerLedFlags[] = {
            // Mimic the Switch's player LEDs
            0x01,
            0x03,
            0x0B,
            0x1B,
            0x11,
            0x09,
            0x19,
            0x0A
        };

        constinit const u8 NewPlayerLedFlags[] = {
            0x04,
            0x02,
            0x05,
            0x03,
            0x07,
            0x07,
            0x07,
            0x07
        };

        constinit const RGBColour PlayerLedBaseColours[] = {
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

        constexpr u8 Step = 4;
        constinit const u8 LedBrightnessMultipliers[] = { 0, 1, 1 * Step, 2 * Step, 3 * Step, 4 * Step, 5 * Step, 6 * Step, 7 * Step, 8 * Step };

        constexpr u32 CrcSeed = 0x8C36CCAE; // CRC32 of {0xa2, 0x31} bytes at beginning of output report

    }

    Result DualsenseController::Initialize() {
        R_TRY(this->PushRumbleLedState());
        R_TRY(EmulatedSwitchController::Initialize());

        // Request controller firmware version info
        R_TRY(this->GetVersionInfo(&m_version_info));

        // Request motion calibration data from DualSense
        R_TRY(this->GetCalibrationData(&m_motion_calibration));

        auto config = mitm::GetGlobalConfig();
        m_lightbar_brightness = config->misc.dualsense_lightbar_brightness;

        R_SUCCEED();
    }

    Result DualsenseController::SetVibration(const SwitchMotorData *motor_data) {
        m_rumble_state.amp_motor_left  = static_cast<u8>(255 * std::max(motor_data->left_motor.low_band_amp, motor_data->right_motor.low_band_amp));
        m_rumble_state.amp_motor_right = static_cast<u8>(255 * std::max(motor_data->left_motor.high_band_amp, motor_data->right_motor.high_band_amp));
        return this->PushRumbleLedState();
    }

    Result DualsenseController::CancelVibration() {
        m_rumble_state.amp_motor_left = 0;
        m_rumble_state.amp_motor_right = 0;
        return this->PushRumbleLedState();
    }

    Result DualsenseController::SetPlayerLed(u8 led_mask) {
        u8 player_number;
        R_TRY(LedsMaskToPlayerNumber(led_mask, &player_number));

        u16 fw_version = *reinterpret_cast<u16 *>(&m_version_info.data[43]);

        auto config = mitm::GetGlobalConfig();
        if (!config->misc.dualsense_enable_player_leds) {
            m_led_flags = 0x00;
        } else if (fw_version < 0x0282) {
            m_led_flags = PlayerLedFlags[player_number];
        } else {
            m_led_flags = NewPlayerLedFlags[player_number];
        }

        // Disable LED fade-in
        m_led_flags |= 0x20;

        RGBColour colour = PlayerLedBaseColours[player_number];
        u8 multiplier = LedBrightnessMultipliers[m_lightbar_brightness];
        colour.r *= multiplier;
        colour.g *= multiplier;
        colour.b *= multiplier;
        R_RETURN(this->SetLightbarColour(colour));
    }

    Result DualsenseController::SetLightbarColour(RGBColour colour) {
        m_lightbar_colour = colour;
        R_RETURN(this->PushRumbleLedState());
    }

    void DualsenseController::ProcessInputData(const bluetooth::HidReport *report) {
        auto dualsense_report = reinterpret_cast<const DualsenseReportData *>(&report->data);

        switch(dualsense_report->id) {
            case 0x01:
                this->MapInputReport0x01(dualsense_report); break;
            case 0x31:
                this->MapInputReport0x31(dualsense_report); break;
            default:
                break;
        }
    }

    void DualsenseController::MapInputReport0x01(const DualsenseReportData *src) {
        m_left_stick  = PackAnalogStickValues(src->input0x01.left_stick.x,  InvertAnalogStickValue(src->input0x01.left_stick.y));
        m_right_stick = PackAnalogStickValues(src->input0x01.right_stick.x, InvertAnalogStickValue(src->input0x01.right_stick.y));

        this->MapButtons(&src->input0x01.buttons);

        m_buttons.ZR = src->input0x01.right_trigger > (m_trigger_threshold * TriggerMax);
        m_buttons.ZL = src->input0x01.left_trigger  > (m_trigger_threshold * TriggerMax);
    }

    void DualsenseController::MapInputReport0x31(const DualsenseReportData *src) {
        m_ext_power = src->input0x31.usb;

        if (!src->input0x31.usb || src->input0x31.full) {
            m_charging = false;
        } else {
            m_charging = true;
        }

        u8 battery_level = src->input0x31.battery_level;
        if (!src->input0x31.usb) {
            battery_level++;
        }
        if (battery_level > 10) {
            battery_level = 10;
        }

        m_battery = static_cast<u8>(8 * (battery_level + 2) / 10) & 0x0e;

        m_left_stick  = PackAnalogStickValues(src->input0x31.left_stick.x,  InvertAnalogStickValue(src->input0x31.left_stick.y));
        m_right_stick = PackAnalogStickValues(src->input0x31.right_stick.x, InvertAnalogStickValue(src->input0x31.right_stick.y));

        this->MapButtons(&src->input0x31.buttons);

        m_buttons.ZR = src->input0x31.right_trigger > (m_trigger_threshold * TriggerMax);
        m_buttons.ZL = src->input0x31.left_trigger  > (m_trigger_threshold * TriggerMax);

        auto config = mitm::GetGlobalConfig();
        if (!config->misc.swap_touchpad_button && src->input0x31.buttons.touchpad) {
            for (int i = 0; i < 2; ++i) {
                const DualsenseTouchpadPoint *point = &src->input0x31.touch_points[i];

                bool active = point->contact & BIT(7) ? false : true;;
                if (active) {
                    u16 x = (point->x_hi << 8) | point->x_lo;

                    if (x < (0.15 * TouchpadWidth)) {
                        m_buttons.minus = 1;
                    } else if (x > (0.85 * TouchpadWidth)) {
                        m_buttons.plus = 1;
                    } else {
                        m_buttons.capture = 1;
                    }
                }
            }
        }

        m_accel.x = -src->input0x31.acc_z / float(m_motion_calibration.acc.z_max);
        m_accel.y = -src->input0x31.acc_x / float(m_motion_calibration.acc.x_max);
        m_accel.z =  src->input0x31.acc_y / float(m_motion_calibration.acc.y_max);

        m_gyro.x = -(float(src->input0x31.vel_z) - m_motion_calibration.gyro.roll_bias)  / ((m_motion_calibration.gyro.roll_max  - m_motion_calibration.gyro.roll_bias)  / m_motion_calibration.gyro.speed_max);
        m_gyro.y = -(float(src->input0x31.vel_x) - m_motion_calibration.gyro.pitch_bias) / ((m_motion_calibration.gyro.pitch_max - m_motion_calibration.gyro.pitch_bias) / m_motion_calibration.gyro.speed_max);
        m_gyro.z =  (float(src->input0x31.vel_y) - m_motion_calibration.gyro.yaw_bias) / ((m_motion_calibration.gyro.yaw_max - m_motion_calibration.gyro.yaw_bias) / m_motion_calibration.gyro.speed_max);
    }

    void DualsenseController::MapButtons(const DualsenseButtonData *buttons) {
        m_buttons.dpad_down  = (buttons->dpad == DualsenseDPad_S)  ||
                               (buttons->dpad == DualsenseDPad_SE) ||
                               (buttons->dpad == DualsenseDPad_SW);
        m_buttons.dpad_up    = (buttons->dpad == DualsenseDPad_N)  ||
                               (buttons->dpad == DualsenseDPad_NE) ||
                               (buttons->dpad == DualsenseDPad_NW);
        m_buttons.dpad_right = (buttons->dpad == DualsenseDPad_E)  ||
                               (buttons->dpad == DualsenseDPad_NE) ||
                               (buttons->dpad == DualsenseDPad_SE);
        m_buttons.dpad_left  = (buttons->dpad == DualsenseDPad_W)  ||
                               (buttons->dpad == DualsenseDPad_NW) ||
                               (buttons->dpad == DualsenseDPad_SW);

        m_buttons.A = buttons->circle;
        m_buttons.B = buttons->cross;
        m_buttons.X = buttons->triangle;
        m_buttons.Y = buttons->square;

        m_buttons.R  = buttons->R1;
        m_buttons.L  = buttons->L1;

        m_buttons.lstick_press = buttons->L3;
        m_buttons.rstick_press = buttons->R3;

        m_buttons.home = buttons->ps;

        auto config = mitm::GetGlobalConfig();
        if (config->misc.swap_touchpad_button) {
            m_buttons.capture = buttons->share;
            m_buttons.plus    = buttons->options;
            m_buttons.minus   = buttons->touchpad;
        } else {
            m_buttons.minus   = buttons->share;
            m_buttons.plus    = buttons->options;
        }

        if (config->misc.enable_dualsense_mute_button) {
            m_buttons.capture = buttons->mute;
        }
    }

    Result DualsenseController::GetVersionInfo(DualsenseVersionInfo *version_info) {
        bluetooth::HidReport output;
        R_TRY(this->GetReport(0x20, BtdrvBluetoothHhReportType_Feature, &output));

        auto response = reinterpret_cast<DualsenseReportData *>(&output.data);
        std::memcpy(version_info, &response->feature0x20.version_info, sizeof(DualsenseVersionInfo));

        R_SUCCEED();
    }

    Result DualsenseController::GetCalibrationData(DualsenseImuCalibrationData *calibration) {
        bluetooth::HidReport output;
        R_TRY(this->GetReport(0x05, BtdrvBluetoothHhReportType_Feature, &output));

        auto response = reinterpret_cast<DualsenseReportData *>(&output.data);
        std::memcpy(calibration, &response->feature0x05.calibration, sizeof(DualsenseImuCalibrationData));

        R_SUCCEED();
    }

    Result DualsenseController::PushRumbleLedState() {
        auto config = mitm::GetGlobalConfig();

        std::scoped_lock lk(m_output_mutex);

        DualsenseReportData report = {};
        report.id = 0x31;
        report.output0x31.data[0] = 0x02;
        report.output0x31.data[1] = 0x03;
        report.output0x31.data[2] = 0x54;
        report.output0x31.data[3] = m_rumble_state.amp_motor_right;
        report.output0x31.data[4] = m_rumble_state.amp_motor_left;
        report.output0x31.data[37] = 0x08 - config->misc.dualsense_vibration_intensity;  // User setting is inverse of how the controller sets intensity
        report.output0x31.data[39] = 0x02 | 0x01;
        report.output0x31.data[42] = 0x02;
        report.output0x31.data[43] = 0x02;
        report.output0x31.data[44] = m_led_flags;
        report.output0x31.data[45] = m_lightbar_colour.r;
        report.output0x31.data[46] = m_lightbar_colour.g;
        report.output0x31.data[47] = m_lightbar_colour.b;
        report.output0x31.crc = crc32CalculateWithSeed(CrcSeed, report.output0x31.data, sizeof(report.output0x31.data));

        m_output_report.size = sizeof(report.output0x31) + sizeof(report.id);
        std::memcpy(m_output_report.data, &report, m_output_report.size);

        R_RETURN(this->WriteDataReport(&m_output_report));
    }

}
