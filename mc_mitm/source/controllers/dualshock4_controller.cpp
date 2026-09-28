/*
 * Copyright (c) 2020-2026 ndeadly
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

        constexpr u8 TriggerMax = UINT8_MAX;

        constexpr u16 TouchpadWidth = 1920;
        constexpr u16 TouchpadHeight = 942;

        constexpr RGBColour PlayerLedBaseColours[] = {
            {0x00, 0x00, 0x00}, // off
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

        constexpr SwitchBatteryLevel BatteryLookup[] = {
            SwitchBatteryLevel::Critical,
            SwitchBatteryLevel::Low,
            SwitchBatteryLevel::Low,
            SwitchBatteryLevel::Low,
            SwitchBatteryLevel::Medium,
            SwitchBatteryLevel::Medium,
            SwitchBatteryLevel::Medium,
            SwitchBatteryLevel::Full,
            SwitchBatteryLevel::Full,
            SwitchBatteryLevel::Full,
            SwitchBatteryLevel::Full,
        };

        constexpr u8 Step = 4;
        constexpr u8 LedBrightnessMultipliers[] = { 0, 1, 1 * Step, 2 * Step, 3 * Step, 4 * Step, 5 * Step, 6 * Step, 7 * Step, 8 * Step };

        constexpr u32 CrcSeed = 0xB758EC66;  // CRC32 of {0xa2, 0x11} bytes at beginning of output report

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

    Result Dualshock4Controller::SetVibration(const SwitchMotorData *motor_data) {
        m_rumble_state.amp_motor_left  = static_cast<u8>(255 * std::max(motor_data->left_motor.low_band_amp,  motor_data->right_motor.low_band_amp));
        m_rumble_state.amp_motor_right = static_cast<u8>(255 * std::max(motor_data->left_motor.high_band_amp, motor_data->right_motor.high_band_amp));
        R_RETURN(this->PushRumbleLedState());
    }

    Result Dualshock4Controller::CancelVibration() {
        m_rumble_state.amp_motor_left = 0;
        m_rumble_state.amp_motor_right = 0;
        R_RETURN(this->PushRumbleLedState());
    }

    Result Dualshock4Controller::SetPlayerLed(SwitchPlayerNumber player_number) {
        u8 player_index = static_cast<u8>(player_number);

        RGBColour colour = PlayerLedBaseColours[player_index];
        u8 multiplier = LedBrightnessMultipliers[m_lightbar_brightness];
        colour.r *= multiplier;
        colour.g *= multiplier;
        colour.b *= multiplier;

        R_RETURN(this->SetLightbarColour(colour));
    }

    Result Dualshock4Controller::SetLightbarColour(RGBColour colour) {
        m_lightbar_colour = colour;
        R_RETURN(this->PushRumbleLedState());
    }

    void Dualshock4Controller::ParseInputReport(const u8 *report_buffer, size_t size) {
        AMS_UNUSED(size);
        auto report = reinterpret_cast<const Dualshock4ReportData *>(report_buffer);

        switch(report->id) {
            case 0x01:
                this->MapInputReport0x01(report); break;
            case 0x11:
                this->MapInputReport0x11(report); break;
            default:
                break;
        }
    }

    void Dualshock4Controller::MapInputReport0x01(const Dualshock4ReportData *src) {
        auto dpad = DirectionalPad(static_cast<DirectionalPadType<0, 8>>(src->input0x01.buttons.dpad));

        m_left_stick.SetValuesFrom(
            src->input0x01.left_stick.GetX(),
            src->input0x01.left_stick.GetYInverted()
        );

        m_right_stick.SetValuesFrom(
            src->input0x01.right_stick.GetX(),
            src->input0x01.right_stick.GetYInverted()
        );

        SwitchButtons button_state = m_buttons;
        button_state.Assign(SwitchButton::Down,   dpad.IsDown());
        button_state.Assign(SwitchButton::Up,     dpad.IsUp());
        button_state.Assign(SwitchButton::Right,  dpad.IsRight());
        button_state.Assign(SwitchButton::Left,   dpad.IsLeft());
        button_state.Assign(SwitchButton::A,      src->input0x01.buttons.circle);
        button_state.Assign(SwitchButton::B,      src->input0x01.buttons.cross);
        button_state.Assign(SwitchButton::X,      src->input0x01.buttons.triangle);
        button_state.Assign(SwitchButton::Y,      src->input0x01.buttons.square);
        button_state.Assign(SwitchButton::R,      src->input0x01.buttons.R1);
        button_state.Assign(SwitchButton::L,      src->input0x01.buttons.L1);
        button_state.Assign(SwitchButton::ZR,     src->input0x01.right_trigger > (m_trigger_threshold * TriggerMax));
        button_state.Assign(SwitchButton::ZL,     src->input0x01.left_trigger  > (m_trigger_threshold * TriggerMax));
        button_state.Assign(SwitchButton::Minus,  src->input0x01.buttons.share);
        button_state.Assign(SwitchButton::Plus,   src->input0x01.buttons.options);
        button_state.Assign(SwitchButton::StickL, src->input0x01.buttons.L3);
        button_state.Assign(SwitchButton::StickR, src->input0x01.buttons.R3);
        button_state.Assign(SwitchButton::Home,   src->input0x01.buttons.ps);
        m_buttons = button_state;
    }

    void Dualshock4Controller::MapInputReport0x11(const Dualshock4ReportData *src) {
        m_power_info.SetPowered(src->input0x11.powered);
        m_power_info.SetCharging(src->input0x11.powered && !(src->input0x11.battery_level > 10));
        m_power_info.SetBatteryLevel(BatteryLookup[std::clamp<u8>(src->input0x11.battery_level, 0, sizeof(BatteryLookup) - 1)]);

        auto dpad = DirectionalPad(static_cast<DirectionalPadType<0, 8>>(src->input0x11.buttons.dpad));

        m_left_stick.SetValuesFrom(
            src->input0x11.left_stick.GetX(),
            src->input0x11.left_stick.GetYInverted()
        );

        m_right_stick.SetValuesFrom(
            src->input0x11.right_stick.GetX(),
            src->input0x11.right_stick.GetYInverted()
        );

        SwitchButtons button_state = m_buttons;
        button_state.Assign(SwitchButton::Down,   dpad.IsDown());
        button_state.Assign(SwitchButton::Up,     dpad.IsUp());
        button_state.Assign(SwitchButton::Right,  dpad.IsRight());
        button_state.Assign(SwitchButton::Left,   dpad.IsLeft());
        button_state.Assign(SwitchButton::A,      src->input0x11.buttons.circle);
        button_state.Assign(SwitchButton::B,      src->input0x11.buttons.cross);
        button_state.Assign(SwitchButton::X,      src->input0x11.buttons.triangle);
        button_state.Assign(SwitchButton::Y,      src->input0x11.buttons.square);
        button_state.Assign(SwitchButton::R,      src->input0x11.buttons.R1);
        button_state.Assign(SwitchButton::L,      src->input0x11.buttons.L1);
        button_state.Assign(SwitchButton::ZR,     src->input0x11.right_trigger > (m_trigger_threshold * TriggerMax));
        button_state.Assign(SwitchButton::ZL,     src->input0x11.left_trigger  > (m_trigger_threshold * TriggerMax));
        button_state.Assign(SwitchButton::Minus,  src->input0x11.buttons.share);
        button_state.Assign(SwitchButton::Plus,   src->input0x11.buttons.options);
        button_state.Assign(SwitchButton::StickL, src->input0x11.buttons.L3);
        button_state.Assign(SwitchButton::StickR, src->input0x11.buttons.R3);
        button_state.Assign(SwitchButton::Home,   src->input0x11.buttons.ps);
        if (src->input0x11.buttons.touchpad) {
            for (int i = 0; i < src->input0x11.num_reports; ++i) {
                const Dualshock4TouchReport *touch_report = &src->input0x11.touch_reports[i];
                for (int j = 0; j < 2; ++j) {
                    const Dualshock4TouchpadPoint *point = &touch_report->points[j];

                    bool active = point->contact & BIT(7) ? false : true;
                    if (active) {
                        u16 x = (point->x_hi << 8) | point->x_lo;

                        if (x < (0.15 * TouchpadWidth)) {
                            button_state.Set(SwitchButton::Minus);
                        } else if (x > (0.85 * TouchpadWidth)) {
                            button_state.Set(SwitchButton::Plus);
                        } else {
                            button_state.Set(SwitchButton::Capture);
                        }
                    }
                }
            }
        } else {
            button_state.Clear(SwitchButton::Capture);
        }
        m_buttons = button_state;

        utils::Vec3d<float> accel = {
            .x = -src->input0x11.acc_z / float(m_motion_calibration.acc.z_max),
            .y = -src->input0x11.acc_x / float(m_motion_calibration.acc.x_max),
            .z =  src->input0x11.acc_y / float(m_motion_calibration.acc.y_max)
        };

        utils::Vec3d<float> gyro = {
            .x = -(src->input0x11.vel_z - m_motion_calibration.gyro.roll_bias)  / (float(m_motion_calibration.gyro.roll_max  - m_motion_calibration.gyro.roll_bias)  / m_motion_calibration.gyro.speed_max),
            .y = -(src->input0x11.vel_x - m_motion_calibration.gyro.pitch_bias) / (float(m_motion_calibration.gyro.pitch_max - m_motion_calibration.gyro.pitch_bias) / m_motion_calibration.gyro.speed_max),
            .z =  (src->input0x11.vel_y - m_motion_calibration.gyro.yaw_bias)   / (float(m_motion_calibration.gyro.yaw_max   - m_motion_calibration.gyro.yaw_bias)   / m_motion_calibration.gyro.speed_max)
        };

        m_sixaxis_processor.Update(accel, gyro);
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
        report.output0x11.crc = crc32CalculateWithSeed(CrcSeed, report.output0x11.data, sizeof(report.output0x11.data));

        R_RETURN(this->WriteDataReport(&report, sizeof(report.output0x11) + sizeof(report.id)));
    }

}
