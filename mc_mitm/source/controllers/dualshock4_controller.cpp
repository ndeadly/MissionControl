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

        // Every Switch button a Dualshock 4 input can be mapped to. These are cleared and rebuilt from the button map on each report.
        constexpr u32 RemappableButtons =
            static_cast<u32>(SwitchButton::Y)       | static_cast<u32>(SwitchButton::X)      |
            static_cast<u32>(SwitchButton::B)       | static_cast<u32>(SwitchButton::A)      |
            static_cast<u32>(SwitchButton::R)       | static_cast<u32>(SwitchButton::ZR)     |
            static_cast<u32>(SwitchButton::Minus)   | static_cast<u32>(SwitchButton::Plus)   |
            static_cast<u32>(SwitchButton::StickR)  | static_cast<u32>(SwitchButton::StickL) |
            static_cast<u32>(SwitchButton::Home)    | static_cast<u32>(SwitchButton::Capture)|
            static_cast<u32>(SwitchButton::Down)    | static_cast<u32>(SwitchButton::Up)     |
            static_cast<u32>(SwitchButton::Right)   | static_cast<u32>(SwitchButton::Left)   |
            static_cast<u32>(SwitchButton::L)       | static_cast<u32>(SwitchButton::ZL);

        struct TouchpadZones {
            bool left;
            bool centre;
            bool right;
        };

        // Translates the pressed Dualshock 4 inputs into Switch buttons using the user's button map (see dualshock4_map_* in the ini)
        SwitchButtons MapDualshock4Buttons(const Dualshock4ButtonData &buttons, bool l2, bool r2, const TouchpadZones &touch) {
            const auto &map = mitm::GetGlobalConfig()->misc.dualshock4_button_map;
            const auto dpad = DirectionalPad(static_cast<DirectionalPadType<0, 8>>(buttons.dpad));

            SwitchButtons state;
            auto press = [&](mitm::Dualshock4Input input, bool pressed) {
                state.SetIf(SwitchButtons(map[input]), pressed);
            };

            press(mitm::Dualshock4Input_Cross,          buttons.cross);
            press(mitm::Dualshock4Input_Circle,         buttons.circle);
            press(mitm::Dualshock4Input_Square,         buttons.square);
            press(mitm::Dualshock4Input_Triangle,       buttons.triangle);
            press(mitm::Dualshock4Input_L1,             buttons.L1);
            press(mitm::Dualshock4Input_R1,             buttons.R1);
            press(mitm::Dualshock4Input_L2,             l2);
            press(mitm::Dualshock4Input_R2,             r2);
            press(mitm::Dualshock4Input_L3,             buttons.L3);
            press(mitm::Dualshock4Input_R3,             buttons.R3);
            press(mitm::Dualshock4Input_Share,          buttons.share);
            press(mitm::Dualshock4Input_Options,        buttons.options);
            press(mitm::Dualshock4Input_Ps,             buttons.ps);
            press(mitm::Dualshock4Input_DpadUp,         dpad.IsUp());
            press(mitm::Dualshock4Input_DpadDown,       dpad.IsDown());
            press(mitm::Dualshock4Input_DpadLeft,       dpad.IsLeft());
            press(mitm::Dualshock4Input_DpadRight,      dpad.IsRight());
            press(mitm::Dualshock4Input_TouchpadLeft,   touch.left);
            press(mitm::Dualshock4Input_TouchpadCentre, touch.centre);
            press(mitm::Dualshock4Input_TouchpadRight,  touch.right);

            return state;
        }

    }

    Result Dualshock4Controller::Initialize() {
        // Read the generation first: if the configuration is reloaded in between we simply apply it again on the next report
        m_config_generation = mitm::GetConfigurationGeneration();

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

        auto config = mitm::GetGlobalConfig();

        // Brightness is read here rather than cached so that edits to the ini file take effect straight away
        m_lightbar_brightness = config->misc.dualshock4_lightbar_brightness;
        u8 multiplier = LedBrightnessMultipliers[m_lightbar_brightness];

        // Look up a user-defined colour: per-player override first, then the global one
        int custom = config->misc.dualshock4_lightbar_colour[player_index];
        if (custom < 0) {
            custom = config->misc.dualshock4_lightbar_colour[0];
        }

        RGBColour colour;
        if ((player_index != 0) && (custom >= 0)) {
            // Custom colours use the full 0-255 range per channel. Scale by brightness so that the max setting (9) is 100%
            constexpr u32 MaxMultiplier = 8 * Step;
            colour.r = static_cast<u8>(((custom >> 16) & 0xff) * multiplier / MaxMultiplier);
            colour.g = static_cast<u8>(((custom >>  8) & 0xff) * multiplier / MaxMultiplier);
            colour.b = static_cast<u8>(( custom        & 0xff) * multiplier / MaxMultiplier);
        } else {
            colour = PlayerLedBaseColours[player_index];
            colour.r *= multiplier;
            colour.g *= multiplier;
            colour.b *= multiplier;
        }

        R_RETURN(this->SetLightbarColour(colour));
    }

    Result Dualshock4Controller::SetLightbarColour(RGBColour colour) {
        m_lightbar_colour = colour;
        R_RETURN(this->PushRumbleLedState());
    }

    void Dualshock4Controller::ApplyConfiguration() {
        auto config = mitm::GetGlobalConfig();
        m_report_rate = static_cast<Dualshock4ReportRate>(config->misc.dualshock4_polling_rate);

        // Re-send the lightbar colour (which also pushes the new report rate). Leave the lightbar alone until the
        // system has assigned this controller a player number.
        auto player_number = m_player_indicator.GetPlayerNumber();
        if (player_number != SwitchPlayerNumber::Invalid) {
            R_DISCARD(this->SetPlayerLed(player_number));
        }
    }

    void Dualshock4Controller::ParseInputReport(const u8 *report_buffer, size_t size) {
        AMS_UNUSED(size);

        // The ini file was reloaded since we last looked: push the new lightbar colour/brightness to the controller
        const u32 generation = mitm::GetConfigurationGeneration();
        if (generation != m_config_generation) {
            m_config_generation = generation;
            this->ApplyConfiguration();
        }

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
        m_left_stick.SetValuesFrom(
            src->input0x01.left_stick.GetX(),
            src->input0x01.left_stick.GetYInverted()
        );

        m_right_stick.SetValuesFrom(
            src->input0x01.right_stick.GetX(),
            src->input0x01.right_stick.GetYInverted()
        );

        // This report carries no touchpad position, so the touchpad can't be used here
        SwitchButtons button_state = m_buttons;
        button_state.Clear(SwitchButtons(RemappableButtons));
        button_state.Set(MapDualshock4Buttons(
            src->input0x01.buttons,
            src->input0x01.left_trigger  > (m_trigger_threshold * TriggerMax),
            src->input0x01.right_trigger > (m_trigger_threshold * TriggerMax),
            TouchpadZones{}
        ));
        m_buttons = button_state;
    }

    void Dualshock4Controller::MapInputReport0x11(const Dualshock4ReportData *src) {
        m_power_info.SetPowered(src->input0x11.powered);
        m_power_info.SetCharging(src->input0x11.powered && !(src->input0x11.battery_level > 10));
        m_power_info.SetBatteryLevel(BatteryLookup[std::clamp<u8>(src->input0x11.battery_level, 0, sizeof(BatteryLookup) - 1)]);

        m_left_stick.SetValuesFrom(
            src->input0x11.left_stick.GetX(),
            src->input0x11.left_stick.GetYInverted()
        );

        m_right_stick.SetValuesFrom(
            src->input0x11.right_stick.GetX(),
            src->input0x11.right_stick.GetYInverted()
        );

        // Work out which third of the touchpad is being pressed (the touchpad acts as three buttons)
        TouchpadZones touch = {};
        if (src->input0x11.buttons.touchpad) {
            for (int i = 0; i < src->input0x11.num_reports; ++i) {
                const Dualshock4TouchReport *touch_report = &src->input0x11.touch_reports[i];
                for (int j = 0; j < 2; ++j) {
                    const Dualshock4TouchpadPoint *point = &touch_report->points[j];

                    bool active = point->contact & BIT(7) ? false : true;
                    if (active) {
                        u16 x = (point->x_hi << 8) | point->x_lo;

                        if (x < (0.15 * TouchpadWidth)) {
                            touch.left = true;
                        } else if (x > (0.85 * TouchpadWidth)) {
                            touch.right = true;
                        } else {
                            touch.centre = true;
                        }
                    }
                }
            }
        }

        SwitchButtons button_state = m_buttons;
        button_state.Clear(SwitchButtons(RemappableButtons));
        button_state.Set(MapDualshock4Buttons(
            src->input0x11.buttons,
            src->input0x11.left_trigger  > (m_trigger_threshold * TriggerMax),
            src->input0x11.right_trigger > (m_trigger_threshold * TriggerMax),
            touch
        ));
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
