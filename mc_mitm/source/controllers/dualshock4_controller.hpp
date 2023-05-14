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
        u8 x;
        u8 y;
    } PACKED;

    struct Dualshock4ButtonData {
        u8 dpad     : 4;
        u8 square   : 1;
        u8 cross    : 1;
        u8 circle   : 1;
        u8 triangle : 1;
        
        u8 L1       : 1;
        u8 R1       : 1;
        u8 L2       : 1;
        u8 R2       : 1;
        u8 share    : 1;
        u8 options  : 1;
        u8 L3       : 1;
        u8 R3       : 1;
        
        u8 ps       : 1;
        u8 touchpad : 1;
        u8 counter  : 6;
    } PACKED;

    struct Dualshock4RumbleData {
        u8 amp_motor_left;
        u8 amp_motor_right;
    } PACKED;

    struct Dualshock4ImuCalibrationData {
        struct {
            s16 pitch_bias;
            s16 yaw_bias;
            s16 roll_bias;
            s16 pitch_max;
            s16 yaw_max;
            s16 roll_max;
            s16 pitch_min;
            s16 yaw_min;
            s16 roll_min;
            s16 speed_max;
            s16 speed_min;
        } gyro;
        
        struct {
            s16 x_max;
            s16 x_min;
            s16 y_max;
            s16 y_min;
            s16 z_max;
            s16 z_min;
        } acc;
    } PACKED;

    struct Dualshock4TouchpadPoint {
        u8 contact;
        u8 x_lo;
        u8 x_hi : 4;
        u8 y_lo : 4;
        u8 y_hi;
    } PACKED;

    struct Dualshock4TouchReport {
        u8 timestamp;
        Dualshock4TouchpadPoint points[2];
    } PACKED;

    struct Dualshock4VersionInfo {
        char date[48];
    } PACKED;

    struct Dualshock4FeatureReport0x05 {
        Dualshock4ImuCalibrationData calibration;
        u32 crc;
    } PACKED;

    struct Dualshock4FeatureReport0x06 {
        Dualshock4VersionInfo version_info;
        u32 crc;
    } PACKED;

    struct Dualshock4FeatureReport0xa3 {
        Dualshock4VersionInfo version_info;
    } PACKED;

    struct Dualshock4OutputReport0x11 {
        struct {
            u8 data[73];
        };
        u32 crc;
    } PACKED;

    struct Dualshock4InputReport0x01 {
        Dualshock4StickData left_stick;
        Dualshock4StickData right_stick;
        Dualshock4ButtonData buttons;
        u8 left_trigger;
        u8 right_trigger;
    } PACKED;

    struct Dualshock4InputReport0x11 {
        u8 _unk0[2];
        Dualshock4StickData left_stick;
        Dualshock4StickData right_stick;
        Dualshock4ButtonData buttons;
        u8 left_trigger;
        u8 right_trigger;

        u16 timestamp;
        u8 temperature;
        s16 vel_x;
        s16 vel_y;
        s16 vel_z;
        s16 acc_x;
        s16 acc_y;
        s16 acc_z;
        u8 _unk1[5];

        u8 battery_level : 4;
        u8 usb           : 1;
        u8 mic           : 1;
        u8 phone         : 1;
        u8               : 0;
        u8 _unk2[2];

        u8 num_reports;
        Dualshock4TouchReport touch_reports[4];
        u8 _unk3[2];

        u32 crc;
    } PACKED;

    struct Dualshock4ReportData {
        u8 id;
        union {
            Dualshock4FeatureReport0x05 feature0x05;
            Dualshock4FeatureReport0x06 feature0x06;
            Dualshock4FeatureReport0xa3 feature0xa3;
            Dualshock4OutputReport0x11 output0x11;
            Dualshock4InputReport0x01 input0x01;
            Dualshock4InputReport0x11 input0x11;
        };
    } PACKED;

    class Dualshock4Controller final : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = {
                {0x054c, 0x05c4},   // Official Dualshock4 v1
                {0x054c, 0x09cc},   // Official Dualshock4 v2
                {0x0f0d, 0x00f6},   // Hori ONYX
                {0x1532, 0x1009},   // Razer Raiju Ultimate
                {0x1532, 0x100a}    // Razer Raiju Tournament
            };

            Dualshock4Controller(const bluetooth::Address *address, HardwareID id)
            : EmulatedSwitchController(address, id)
            , m_report_rate(Dualshock4ReportRate_125Hz)
            , m_lightbar_colour({0, 0, 0})
            , m_lightbar_brightness(0)
            , m_rumble_state({0, 0}) { }

            Result Initialize();
            Result SetVibration(const SwitchRumbleData *rumble_data);
            Result CancelVibration();
            Result SetPlayerLed(u8 led_mask);
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
            RGBColour m_lightbar_colour;
            u8 m_lightbar_brightness;
            Dualshock4RumbleData m_rumble_state;

            Dualshock4ImuCalibrationData m_motion_calibration;
    };

}
