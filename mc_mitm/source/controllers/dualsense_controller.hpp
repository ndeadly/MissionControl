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

    enum DualsenseDPadDirection {
        DualsenseDPad_N,
        DualsenseDPad_NE,
        DualsenseDPad_E,
        DualsenseDPad_SE,
        DualsenseDPad_S,
        DualsenseDPad_SW,
        DualsenseDPad_W,
        DualsenseDPad_NW,
        DualsenseDPad_Released
    };

    struct DualsenseStickData {
        u8 x;
        u8 y;
    } PACKED;

    struct DualsenseButtonData {
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
        u8 mute     : 1;
        u8          : 0;

        u8 unk;
    } PACKED;

    struct DualsenseRumbleData {
        u8 amp_motor_left;
        u8 amp_motor_right;
    } PACKED;

    struct DualsenseImuCalibrationData {
        struct {
            s16 pitch_bias;
            s16 yaw_bias;
            s16 roll_bias;
            s16 pitch_max;
            s16 pitch_min;
            s16 yaw_max;
            s16 yaw_min;
            s16 roll_max;
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

    struct DualsenseTouchpadPoint {
        u8 contact;
        u8 x_lo;
        u8 x_hi : 4;
        u8 y_lo : 4;
        u8 y_hi;
    } PACKED;

    struct DualsenseVersionInfo {
        char data[64];
    } PACKED;

    struct DualsenseFeatureReport0x05 {
        DualsenseImuCalibrationData calibration;
        u32 crc;
    } PACKED;

    struct DualsenseFeatureReport0x20 {
        DualsenseVersionInfo version_info;
    } PACKED;

    struct DualsenseOutputReport0x31 {
        struct {
            u8 data[73];
        };
        u32 crc;
    } PACKED;

    struct DualsenseInputReport0x01 {
        DualsenseStickData left_stick;
        DualsenseStickData right_stick;
        DualsenseButtonData buttons;
        u8 left_trigger;
        u8 right_trigger;
    } PACKED;

    struct DualsenseInputReport0x31 {
        u8 _unk0;
        DualsenseStickData left_stick;
        DualsenseStickData right_stick;
        u8 left_trigger;
        u8 right_trigger;
        u8 counter;
        DualsenseButtonData buttons;
        u8 _unk1[4];
        s16 vel_x;
        s16 vel_y;
        s16 vel_z;
        s16 acc_x;
        s16 acc_y;
        s16 acc_z;
        s32 timestamp;
        u8 _unk2;
        DualsenseTouchpadPoint touch_points[2];
        u8 _unk3[12];

        u8 battery_level : 4;
        u8 usb           : 1;
        u8 full          : 1;
        u8               : 0;
    } PACKED;

    struct DualsenseReportData {
        u8 id;
        union {
            DualsenseFeatureReport0x05 feature0x05;
            DualsenseFeatureReport0x20 feature0x20;
            DualsenseOutputReport0x31 output0x31;
            DualsenseInputReport0x01 input0x01;
            DualsenseInputReport0x31 input0x31;
        };
    } PACKED;

    class DualsenseController final : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = {
                {0x054c, 0x0ce6},   // Sony Dualsense Controller
                {0x054c, 0x0df2}    // Sony Dualsense Edge Controller
            };

            DualsenseController(const bluetooth::Address *address, HardwareID id)
            : EmulatedSwitchController(address, id)
            , m_led_flags(0)
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
            void MapInputReport0x01(const DualsenseReportData *src);
            void MapInputReport0x31(const DualsenseReportData *src);

            void MapButtons(const DualsenseButtonData *buttons);

            Result GetVersionInfo(DualsenseVersionInfo *version_info);
            Result GetCalibrationData(DualsenseImuCalibrationData *calibration);
            Result PushRumbleLedState();

            u8 m_led_flags;
            RGBColour m_lightbar_colour;
            u8 m_lightbar_brightness;
            DualsenseRumbleData m_rumble_state;

            DualsenseVersionInfo m_version_info;
            DualsenseImuCalibrationData m_motion_calibration;
    };

}
