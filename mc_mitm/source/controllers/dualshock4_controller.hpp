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
#pragma once
#include "emulated_switch_controller.hpp"

namespace ams::controller {

    enum Dualshock4ReportRate {
        Dualshock4ReportRate_Max    = 0,
        Dualshock4ReportRate_1000Hz = 1,
        Dualshock4ReportRate_500Hz  = 2,
        Dualshock4ReportRate_333Hz  = 3,
        Dualshock4ReportRate_250Hz  = 4,
        Dualshock4ReportRate_200Hz  = 5,
        Dualshock4ReportRate_166Hz  = 6,
        Dualshock4ReportRate_142Hz  = 7,
        Dualshock4ReportRate_125Hz  = 8,
        Dualshock4ReportRate_111Hz  = 9,
        Dualshock4ReportRate_100Hz  = 10,
        Dualshock4ReportRate_90Hz   = 11,
        Dualshock4ReportRate_83Hz   = 12,
        Dualshock4ReportRate_76Hz   = 13,
        Dualshock4ReportRate_71Hz   = 14,
        Dualshock4ReportRate_66Hz   = 15,
        Dualshock4ReportRate_62Hz   = 16
    };

    enum Dualshock4ControllerVariant {
        Dualshock4ControllerVariant_V1,
        Dualshock4ControllerVariant_V2,
        Dualshock4ControllerVariant_Unknown
    };

    enum Dualshock4DPadDirection {
        Dualshock4DPad_N,
        Dualshock4DPad_NE,
        Dualshock4DPad_E,
        Dualshock4DPad_SE,
        Dualshock4DPad_S,
        Dualshock4DPad_SW,
        Dualshock4DPad_W,
        Dualshock4DPad_NW,
        Dualshock4DPad_Released
    };

    struct Dualshock4StickData {
        uint8_t x;
        uint8_t y;
    } __attribute__((packed));

    struct Dualshock4ButtonData {
        uint8_t dpad       : 4;
        uint8_t square     : 1;
        uint8_t cross      : 1;
        uint8_t circle     : 1;
        uint8_t triangle   : 1;
        
        uint8_t L1         : 1;
        uint8_t R1         : 1;
        uint8_t L2         : 1;
        uint8_t R2         : 1;
        uint8_t share      : 1;
        uint8_t options    : 1;
        uint8_t L3         : 1;
        uint8_t R3         : 1;
        
        uint8_t ps         : 1;
        uint8_t tpad       : 1;
        uint8_t counter    : 6;
    } __attribute__((packed));

    struct Dualshock4RumbleData {
        uint8_t amp_motor_left;
        uint8_t amp_motor_right;
    } __attribute__((packed));

    struct Dualshock4ImuCalibrationData {
        struct {
            int16_t pitch_bias;
            int16_t yaw_bias;
            int16_t roll_bias;
            int16_t pitch_max;
            int16_t yaw_max;
            int16_t roll_max;
            int16_t pitch_min;
            int16_t yaw_min;
            int16_t roll_min;
            int16_t speed_max;
            int16_t speed_min;
        } gyro;
        
        struct {
            int16_t x_max;
            int16_t x_min;
            int16_t y_max;
            int16_t y_min;
            int16_t z_max;
            int16_t z_min;
        } acc;
    } __attribute__((packed));

    struct Dualshock4VersionInfo {
        char date[48];
    } __attribute__((packed));

    struct Dualshock4FeatureReport0x05 {
        Dualshock4ImuCalibrationData calibration;
        uint32_t crc;
    } __attribute__((packed));

    struct Dualshock4FeatureReport0x06 {
        Dualshock4VersionInfo version_info;
        uint32_t crc;
    } __attribute__((packed));

    struct Dualshock4FeatureReport0xa3 {
        Dualshock4VersionInfo version_info;
    } __attribute__((packed));

    struct Dualshock4OutputReport0x11 {
        struct {
            uint8_t data[73];
        };
        uint32_t crc;
    } __attribute__((packed));

    struct Dualshock4InputReport0x01 {
        Dualshock4StickData left_stick;
        Dualshock4StickData right_stick;
        Dualshock4ButtonData buttons;
        uint8_t left_trigger;
        uint8_t right_trigger;
    } __attribute__((packed));

    struct Dualshock4InputReport0x11 {
        uint8_t _unk0[2];
        Dualshock4StickData left_stick;
        Dualshock4StickData right_stick;
        Dualshock4ButtonData buttons;
        uint8_t left_trigger;
        uint8_t right_trigger;
        uint16_t timestamp;
        uint8_t battery;
        int16_t vel_x;
        int16_t vel_y;
        int16_t vel_z;
        int16_t acc_x;
        int16_t acc_y;
        int16_t acc_z;
        uint8_t _unk1[5];

        uint8_t battery_level    : 4;
        uint8_t usb              : 1;
        uint8_t mic              : 1;
        uint8_t phone            : 1;
        uint8_t                  : 0;

        uint16_t _unk2;
        uint8_t tpad_packets;
        uint8_t packet_counter;
    } __attribute__((packed));

    struct Dualshock4ReportData {
        uint8_t id;
        union {
            Dualshock4FeatureReport0x05 feature0x05;
            Dualshock4FeatureReport0x06 feature0x06;
            Dualshock4FeatureReport0xa3 feature0xa3;
            Dualshock4OutputReport0x11  output0x11;
            Dualshock4InputReport0x01   input0x01;
            Dualshock4InputReport0x11   input0x11;
        };
    } __attribute__((packed));

    class Dualshock4Controller : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = {
                {0x054c, 0x05c4},   // Official Dualshock4 v1
                {0x054c, 0x09cc},   // Official Dualshock4 v2
                {0x0f0d, 0x00f6},   // Hori ONYX
                {0x1532, 0x100a}    // Razer Raiju Tournament
            };

            Dualshock4Controller(const bluetooth::Address *address, HardwareID id)
            : EmulatedSwitchController(address, id)
            , m_report_rate(Dualshock4ReportRate_125Hz)
            , m_led_colour({0, 0, 0})
            , m_rumble_state({0, 0}) { }

            Result Initialize();
            Result SetVibration(const SwitchRumbleData *rumble_data);
            Result CancelVibration();
            Result SetPlayerLed(uint8_t led_mask);
            Result SetLightbarColour(RGBColour colour);

            void ProcessInputData(const bluetooth::HidReport *report) override;

        private:
            void MapInputReport0x01(const Dualshock4ReportData *src);
            void MapInputReport0x11(const Dualshock4ReportData *src);

            void MapButtons(const Dualshock4ButtonData *buttons);
            
            Result GetVersionInfo(Dualshock4VersionInfo *version_info);
            Result GetCalibrationData(Dualshock4ImuCalibrationData *calibration);
            Result PushRumbleLedState();

            Dualshock4ReportRate m_report_rate;
            RGBColour m_led_colour; 
            Dualshock4RumbleData m_rumble_state;

            Dualshock4ImuCalibrationData m_motion_calibration;
    };

}
