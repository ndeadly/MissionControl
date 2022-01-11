/*
 * Copyright (c) 2020-2021 ndeadly
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
#include <cstring>

namespace ams::controller {

    namespace {

        const constexpr float stick_scale_factor = float(UINT12_MAX) / UINT8_MAX;

        constexpr auto GYRO_RES_PER_DEG_S = 1024;
        constexpr auto ACC_RES_PER_G = 8192;

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

        void ConvertToSwitchCalibration(const Dualshock4ImuCalibrationData *calib_data, Switch6AxisCalibrationData *switch_calib) {
            /* Gyro */
            //float numerator = (calib_data->gyro.speed_min + calib_data->gyro.speed_max) * GYRO_RES_PER_DEG_S;
            switch_calib->gyro_bias.pitch = calib_data->gyro.pitch_bias;
            switch_calib->gyro_sensitivity.pitch = (calib_data->gyro.pitch_max - calib_data->gyro.pitch_min) / 2; //numerator / (calib_data->gyro.pitch_max - calib_data->gyro.pitch_min);

            switch_calib->gyro_bias.yaw = calib_data->gyro.yaw_bias;
            switch_calib->gyro_sensitivity.yaw = (calib_data->gyro.pitch_max - calib_data->gyro.pitch_min) / 2; //numerator / (calib_data->gyro.yaw_max - calib_data->gyro.yaw_min);

            switch_calib->gyro_bias.roll = calib_data->gyro.roll_bias;
            switch_calib->gyro_sensitivity.roll = (calib_data->gyro.pitch_max - calib_data->gyro.pitch_min) / 2; //numerator / (calib_data->gyro.roll_max - calib_data->gyro.roll_min);

            /* Accelerometer */
            int16_t acc_range_2g;
            acc_range_2g = calib_data->acc.x_max - calib_data->acc.x_min;
            switch_calib->acc_bias.x = calib_data->acc.x_max - acc_range_2g / 2;
            switch_calib->acc_sensitivity.x = 2 * ACC_RES_PER_G / float(acc_range_2g);

            acc_range_2g = calib_data->acc.y_max - calib_data->acc.y_min;
            switch_calib->acc_bias.y = calib_data->acc.y_max - acc_range_2g / 2;
            switch_calib->acc_sensitivity.y = 2 * ACC_RES_PER_G / float(acc_range_2g);
            
            acc_range_2g = calib_data->acc.z_max - calib_data->acc.z_min;
            switch_calib->acc_bias.z = calib_data->acc.z_max - acc_range_2g / 2;
            switch_calib->acc_sensitivity.z = 2 * ACC_RES_PER_G / float(acc_range_2g);
        }

    }

    Result Dualshock4Controller::Initialize(void) {

        R_TRY(this->PushRumbleLedState());
        R_TRY(EmulatedSwitchController::Initialize());
		
        // Check if factory calibration data has been inserted into virtual SPI flash
        uint8_t calib[sizeof(Switch6AxisCalibrationData)];
        R_TRY(this->VirtualSpiFlashRead(0x6020, &calib, sizeof(calib)));
        bool has_motion_calib = true;
        for (unsigned int i = 0; i < sizeof(calib); ++i) {
            if (calib[i] != 0xff) {
                has_motion_calib = true;
                break;
            }
        }

        // Request calbration from the controller if it's not present
        if (!has_motion_calib) {
            //Todo: make this command spawn a worker and return the value
            R_TRY(this->RequestCalibrationData());

            // Simulate having received the calibration data from a real controller
            uint8_t raw_calib_data[] = {0xfc, 0xff, 0x06, 0x00, 0x07, 0x00, 0x4d, 0x22, 
                                        0xd0, 0x22, 0x2a, 0x23, 0x00, 0xde, 0x3e, 0xdd, 
                                        0x93, 0xdc, 0x1c, 0x02, 0x1c, 0x02, 0xfd, 0x1f, 
                                        0xc6, 0xe0, 0x65, 0x20, 0x66, 0xe0, 0x18, 0x20, 
                                        0x37, 0xdf, 0x0a, 0x00, 0xb6, 0xd3, 0x01, 0x5c};

            auto calib_data = reinterpret_cast<Dualshock4ImuCalibrationData *>(raw_calib_data);

            // Todo: Check CRC of calibration data

            ConvertToSwitchCalibration(calib_data, &m_motion_calibration);

            // Todo: Write converted calibration to virtual SPI file for future use
            //R_TRY(this->VirtualSpiFlashWrite(0x6020, &m_motion_calibration, sizeof(m_motion_calibration)));
        }
        
        return ams::ResultSuccess();
    }

    Result Dualshock4Controller::SetVibration(const SwitchRumbleData *rumble_data) {
        m_rumble_state.amp_motor_left  = static_cast<uint8_t>(255 * std::max(rumble_data[0].low_band_amp, rumble_data[1].low_band_amp));
        m_rumble_state.amp_motor_right = static_cast<uint8_t>(255 * std::max(rumble_data[0].high_band_amp, rumble_data[1].high_band_amp));
        return this->PushRumbleLedState();
    }

    Result Dualshock4Controller::CancelVibration(void) {
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
        m_led_colour = config->misc.disable_sony_leds ? led_disable : colour;
        return this->PushRumbleLedState();
    }

    void Dualshock4Controller::UpdateControllerState(const bluetooth::HidReport *report) {
        auto ds4_report = reinterpret_cast<const Dualshock4ReportData *>(&report->data);

        switch(ds4_report->id) {
            case 0x01:
                this->HandleInputReport0x01(ds4_report);
                break;
            case 0x11:
                this->HandleInputReport0x11(ds4_report);
                break;
            default:
                break;
        }
    }

    void Dualshock4Controller::HandleInputReport0x01(const Dualshock4ReportData *src) {       
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

    void Dualshock4Controller::HandleInputReport0x11(const Dualshock4ReportData *src) {
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

        m_battery = static_cast<uint8_t>(8 * (battery_level + 1) / 10) & 0x0e;

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
            m_motion_data[0].gyro_1 = -src->input0x11.vel_z;
            m_motion_data[0].gyro_2 = -src->input0x11.vel_x;
            m_motion_data[0].gyro_3 = src->input0x11.vel_y;
            m_motion_data[0].accel_x = -src->input0x11.acc_z;
            m_motion_data[0].accel_y = -src->input0x11.acc_x;
            m_motion_data[0].accel_z = src->input0x11.acc_y;

            m_motion_data[1].gyro_1 = -src->input0x11.vel_z;
            m_motion_data[1].gyro_2 = -src->input0x11.vel_x;
            m_motion_data[1].gyro_3 = src->input0x11.vel_y;
            m_motion_data[1].accel_x = -src->input0x11.acc_z;
            m_motion_data[1].accel_y = -src->input0x11.acc_x;
            m_motion_data[1].accel_z = src->input0x11.acc_y;

            m_motion_data[2].gyro_1 = -src->input0x11.vel_z;
            m_motion_data[2].gyro_2 = -src->input0x11.vel_x;
            m_motion_data[2].gyro_3 = src->input0x11.vel_y;
            m_motion_data[2].accel_x = -src->input0x11.acc_z;
            m_motion_data[2].accel_y = -src->input0x11.acc_x;
            m_motion_data[2].accel_z = src->input0x11.acc_y;
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

    Result Dualshock4Controller::RequestVersionInfo(void) {
        return btdrvGetHidReport(m_address, 0xa3, BtdrvBluetoothHhReportType_Feature);
    }

    Result Dualshock4Controller::RequestCalibrationData(void) {
        return btdrvGetHidReport(m_address, 0x05, BtdrvBluetoothHhReportType_Feature);
    }

    Result Dualshock4Controller::PushRumbleLedState(void) {
        Dualshock4OutputReport0x11 report = {0xa2, 0x11, static_cast<uint8_t>(0xc0 | (m_report_rate & 0xff)), 0x20, 0xf3, 0x04, 0x00,
            m_rumble_state.amp_motor_right, m_rumble_state.amp_motor_left,
            m_led_colour.r, m_led_colour.g, m_led_colour.b
        };
        report.crc = crc32Calculate(report.data, sizeof(report.data));

        m_output_report.size = sizeof(report) - 1;
        std::memcpy(m_output_report.data, &report.data[1], m_output_report.size);

        return bluetooth::hid::report::SendHidReport(&m_address, &m_output_report);
    }

    Result Dualshock4Controller::HandleGetReport(const bluetooth::HidReport *report) {
        AMS_UNUSED(report);

        return ams::ResultSuccess();
    }

}
