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
#pragma once
#include "emulated_switch_controller.hpp"

namespace ams::controller {

    enum WiiControllerLEDs {
        WiiControllerLEDs_P1 = 0x10,
        WiiControllerLEDs_P2 = 0x20,
        WiiControllerLEDs_P3 = 0x40,
        WiiControllerLEDs_P4 = 0x80,
    };

    enum WiiControllerOrientation {
        WiiControllerOrientation_Horizontal,
        WiiControllerOrientation_Vertical
    };

    enum WiiExtensionController {
        WiiExtensionController_None,
        WiiExtensionController_Nunchuck,
        WiiExtensionController_ClassicPro,
        WiiExtensionController_WiiUPro,
        WiiExtensionController_MotionPlus,
        WiiExtensionController_MotionPlusNunchuckPassthrough,
        WiiExtensionController_MotionPlusClassicControllerPassthrough,
        WiiExtensionController_TaTaCon,
        WiiExtensionController_BalanceBoard,
        WiiExtensionController_Unrecognised,
    };

    enum MotionPlusStatus {
        MotionPlusStatus_None,
        MotionPlusStatus_Uninitialised,
        MotionPlusStatus_Inactive,
        MotionPlusStatus_Active,
    };

    struct WiiButtonData {
        union {
            u8 raw[2];

            struct {
                u8 dpad_left  : 1;
                u8 dpad_right : 1;
                u8 dpad_down  : 1;
                u8 dpad_up    : 1;
                u8 plus       : 1;
                u8            : 0;
                
                u8 two        : 1;
                u8 one        : 1;
                u8 B          : 1;
                u8 A          : 1;
                u8 minus      : 1;
                u8            : 2;
                u8 home       : 1;
            };
        };
    } PACKED;

    struct WiiAccelerometerData {
        u8 x;
        u8 y;
        u8 z;
    } PACKED;

    struct WiiAccelerometerCalibrationData {
        u16 acc_x_0g;
        u16 acc_y_0g;
        u16 acc_z_0g;

        u16 acc_x_1g;
        u16 acc_y_1g;
        u16 acc_z_1g;
    } PACKED;

    struct WiiClassicControllerExtensionData {
        struct {
            u8 left_stick_x     : 6;
            u8 right_stick_x_43 : 2;

            u8 left_stick_y     : 6;
            u8 right_stick_x_21 : 2;

            u8 right_stick_y    : 5;
            u8 left_trigger_43  : 2;
            u8 right_stick_x_0  : 1;

            u8 right_trigger    : 5;
            u8 left_trigger_20  : 3;
        };

        struct {
            u8            : 1;
            u8 R          : 1;
            u8 plus       : 1;
            u8 home       : 1;
            u8 minus      : 1;
            u8 L          : 1;
            u8 dpad_down  : 1;
            u8 dpad_right : 1;

            u8 dpad_up    : 1;
            u8 dpad_left  : 1;
            u8 ZR         : 1;
            u8 X          : 1; 
            u8 A          : 1;
            u8 Y          : 1;
            u8 B          : 1;
            u8 ZL         : 1;
        } buttons;
    } PACKED;

    struct WiiClassicControllerPassthroughExtensionData {
        union {
            struct {
                u8                     : 1;
                u8 left_stick_x_51     : 5;
                u8 right_stick_x_43    : 2;

                u8                     : 1;
                u8 left_stick_y_51     : 5;
                u8 right_stick_x_21    : 2;

                u8 right_stick_y       : 5;
                u8 left_trigger_43     : 2;
                u8 right_stick_x_0     : 1;

                u8 right_trigger       : 5;
                u8 left_trigger_20     : 3;

                u8 extension_connected : 1;
                u8                     : 0;

                u8                     : 1;
                u8 motionplus_report   : 1;
                u8                     : 0;
            };

            struct {
                u8 dpad_up    : 1;
                u8            : 0;
 
                u8 dpad_left  : 1;
                u8            : 0;

                u8 pad[2];

                u8            : 1;
                u8 R          : 1;
                u8 plus       : 1;
                u8 home       : 1;
                u8 minus      : 1;
                u8 L          : 1;
                u8 dpad_down  : 1;
                u8 dpad_right : 1;

                u8            : 2;
                u8 ZR         : 1;
                u8 X          : 1; 
                u8 A          : 1;
                u8 Y          : 1;
                u8 B          : 1;
                u8 ZL         : 1;
            } buttons;
        };
    } PACKED;

    struct WiiNunchuckExtensionData {
        u8 stick_x;
        u8 stick_y;
        u8 accel_x_92;
        u8 accel_y_92;
        u8 accel_z_92;

        u8 Z          : 1;
        u8 C          : 1;
        u8 accel_x_10 : 2;
        u8 accel_y_10 : 2;
        u8 accel_z_10 : 2;
    } PACKED;

    struct WiiNunchuckPassthroughExtensionData {
        u8 stick_x;
        u8 stick_y;
        u8 accel_x_92;
        u8 accel_y_92;
        u8 extension_connected : 1;
        u8 accel_z_93          : 7;

        u8                     : 1;
        u8 motionplus_report   : 1;
        u8 Z                   : 1;
        u8 C                   : 1;
        u8 accel_x_1           : 1;
        u8 accel_y_1           : 1;
        u8 accel_z_21          : 2; 

    } PACKED;

    struct WiiUProButtonData {
        u8            : 1;
        u8 R          : 1;
        u8 plus       : 1;
        u8 home       : 1;
        u8 minus      : 1;
        u8 L          : 1;
        u8 dpad_down  : 1;
        u8 dpad_right : 1;

        u8 dpad_up    : 1;
        u8 dpad_left  : 1;
        u8 ZR         : 1;
        u8 X          : 1; 
        u8 A          : 1;
        u8 Y          : 1;
        u8 B          : 1;
        u8 ZL         : 1;

        u8 rstick_press  : 1;
        u8 lstick_press  : 1;
        u8 charging      : 1;
        u8 usb_connected : 1;
        u8 battery       : 3;
        u8               : 1;
    } PACKED;

    struct WiiUProExtensionData {
        u16 left_stick_x;
        u16 right_stick_x;
        u16 left_stick_y;
        u16 right_stick_y;
        WiiUProButtonData buttons;
    } PACKED;

    struct MotionPlusExtensionData {
        u8 yaw_speed_lo;
        u8 roll_speed_lo;
        u8 pitch_speed_lo;

        u8 pitch_slow_mode : 1;
        u8 yaw_slow_mode   : 1;
        u8 yaw_speed_hi    : 6;

        u8 extension_connected : 1;
        u8 roll_slow_mode      : 1;
        u8 roll_speed_hi       : 6;

        u8                   : 1;
        u8 motionplus_report : 1;
        u8 pitch_speed_hi    : 6;
    } PACKED;

    struct MotionPlusCalibration {
        u16 yaw_zero;
        u16 roll_zero;
        u16 pitch_zero;
        u16 yaw_scale;
        u16 roll_scale;
        u16 pitch_scale;
        u8 degrees_div_6;
    } PACKED;

    struct MotionPlusCalibrationData {
        MotionPlusCalibration fast;
        MotionPlusCalibration slow;
    } PACKED;

    struct TaTaConExtensionData {
        u8 _unk0[5];

        u8          : 3;
        u8 R_rim    : 1;
        u8 R_center : 1;
        u8 L_rim    : 1;
        u8 L_center : 1;
        u8          : 0;
    } PACKED;

    struct BalanceBoardExtensionData {
        u16 top_right;
        u16 bottom_right;
        u16 top_left;
        u16 bottom_left;
        u8 temperature;
        u8 _pad;
        u8 battery;
    } PACKED;

    struct BalanceBoardCalibrationData {
        u16 top_right_0kg;
        u16 bottom_right_0kg;
        u16 top_left_0kg;
        u16 bottom_left_0kg;

        u16 top_right_17kg;
        u16 bottom_right_17kg;
        u16 top_left_17kg;
        u16 bottom_left_17kg;

        u16 top_right_34kg;
        u16 bottom_right_34kg;
        u16 top_left_34kg;
        u16 bottom_left_34kg;
    } PACKED;

    struct WiiOutputReport0x10 {
        u8 rumble : 1;
        u8        : 0;
    } PACKED;

    struct WiiOutputReport0x11 {
        u8 rumble : 1;
        u8        : 3;
        u8 leds   : 4;
    } PACKED;

    struct WiiOutputReport0x12 {
        u8 rumble : 1;
        u8        : 0;
        u8 report_mode;
    } PACKED;

    struct WiiOutputReport0x14 {
        u8                : 5;
        u8 speaker_enable : 1;
        u8                : 0;
    } PACKED;

    struct WiiOutputReport0x15 {
        u8 rumble  : 1;
        u8         : 0;
    } PACKED;

    struct WiiOutputReport0x16 {
        u32 address;
        u8 size;
        u8 data[16];
    } PACKED;

    struct WiiOutputReport0x17 {
        u32 address;
        u16 size;
    } PACKED;

    struct WiiOutputReport0x18 {
        u8 size;
        u8 speaker_data[20];
    } PACKED;

    struct WiiOutputReport0x19 {
        u8              : 5;
        u8 speaker_mute : 1;
        u8              : 0;
    } PACKED;

    struct WiiInputReport0x20 {
        WiiButtonData buttons;
        u8 battery_critical    : 1;
        u8 extension_connected : 1;
        u8 speaker_enabled     : 1;
        u8 ir_enabled          : 1;
        u8 led_state           : 4;
        u8 _pad[2];
        u8 battery;
    } PACKED;

    struct WiiInputReport0x21 {
        WiiButtonData buttons;
        u8 error : 4;
        u8 size  : 4;
        u16 address;
        u8 data[16];
    } PACKED;

    struct WiiInputReport0x22 {
        WiiButtonData buttons;
        u8 report_id;
        u8 error;
    } PACKED;

    struct WiiInputReport0x30 {
        WiiButtonData buttons;
    } PACKED;

    struct WiiInputReport0x31 {
        WiiButtonData buttons;
        WiiAccelerometerData accel;
    } PACKED;

    struct WiiInputReport0x32 {
        WiiButtonData buttons;
        u8 extension[8];
    } PACKED;

    struct WiiInputReport0x33 {
        WiiButtonData buttons;
        WiiAccelerometerData accel;
        u8 ir[12];
    } PACKED;

    struct WiiInputReport0x34 {
        WiiButtonData buttons;
        u8 extension[19];
    } PACKED;

    struct WiiInputReport0x35 {
        WiiButtonData buttons;
        WiiAccelerometerData accel;
        u8 extension[16];
    } PACKED;

    struct WiiInputReport0x36 {
        WiiButtonData buttons;
        u8 ir[10];
        u8 extension[9];
    } PACKED;

    struct WiiInputReport0x37 {
        WiiButtonData buttons;
        WiiAccelerometerData accel;
        u8 ir[10];
        u8 extension[6];
    } PACKED;

    struct WiiInputReport0x3d {
        u8 extension[21];
    } PACKED;

    struct WiiInputReport0x3e {
        WiiButtonData buttons;
        u8 accel;
        u8 ir[18];
    } PACKED;

    struct WiiInputReport0x3f {
        WiiButtonData buttons;
        u8 accel;
        u8 ir[18];
    } PACKED;

    struct WiiReportData {
        u8 id;
        union {
            WiiOutputReport0x10 output0x10;
            WiiOutputReport0x11 output0x11;
            WiiOutputReport0x12 output0x12;
            WiiOutputReport0x14 output0x14;
            WiiOutputReport0x15 output0x15;
            WiiOutputReport0x16 output0x16;
            WiiOutputReport0x17 output0x17;
            WiiOutputReport0x18 output0x18;
            WiiOutputReport0x19 output0x19;
            WiiInputReport0x20 input0x20;
            WiiInputReport0x21 input0x21;
            WiiInputReport0x22 input0x22;
            WiiInputReport0x30 input0x30;
            WiiInputReport0x31 input0x31;
            WiiInputReport0x32 input0x32;
            WiiInputReport0x33 input0x33;
            WiiInputReport0x34 input0x34;
            WiiInputReport0x35 input0x35;
            WiiInputReport0x36 input0x36;
            WiiInputReport0x37 input0x37;
            WiiInputReport0x3d input0x3d;
            WiiInputReport0x3e input0x3e;
            WiiInputReport0x3f input0x3f;
        };
    } PACKED;

    class WiiController final : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = {
                {0x057e, 0x0306},  // Official Wiimote/Balance Board
                {0x057e, 0x0330},  // Official Wii U Pro Controller
            };

            WiiController(const bluetooth::Address *address, HardwareID id)
            : EmulatedSwitchController(address, id)
            , m_orientation(WiiControllerOrientation_Horizontal)
            , m_extension(WiiExtensionController_None)
            , m_rumble_state(0)
            , m_mp_extension_flag(false)
            , m_mp_state_changing(false) { }

            Result Initialize();
            Result SetVibration(const SwitchMotorData *motor_data);
            Result CancelVibration();
            Result SetPlayerLed(u8 led_mask);
            void ProcessInputData(const bluetooth::HidReport *report) override;

        protected:
            void MapInputReport0x20(const WiiReportData *src);
            void MapInputReport0x21(const WiiReportData *src);
            void MapInputReport0x22(const WiiReportData *src);
            void MapInputReport0x30(const WiiReportData *src);
            void MapInputReport0x31(const WiiReportData *src);
            void MapInputReport0x32(const WiiReportData *src);
            void MapInputReport0x34(const WiiReportData *src);
            void MapInputReport0x35(const WiiReportData *src);
            void MapInputReport0x3d(const WiiReportData *src);

            void MapCoreButtons(const WiiButtonData *buttons);
            void MapAccelerometerData(const WiiAccelerometerData *accel, const WiiButtonData *buttons);
            void MapExtensionBytes(const u8 ext[]);
            void MapNunchuckExtension(const u8 ext[]);
            void MapClassicControllerExtension(const u8 ext[]);
            void MapWiiUProControllerExtension(const u8 ext[]);
            void MapTaTaConExtension(const u8 ext[]);
            void MapBalanceBoardExtension(const u8 ext[]);
            void MapMotionPlusExtension(const u8 ext[]);
            void MapNunchuckExtensionPassthroughMode(const u8 ext[]);
            void MapClassicControllerExtensionPassthroughMode(const u8 ext[]);

            void HandleStatusReport(const WiiReportData *wii_report);

            WiiExtensionController GetExtensionControllerType();
            MotionPlusStatus GetMotionPlusStatus();

            Result GetAccelerometerCalibration(WiiAccelerometerCalibrationData *calibration);
            Result GetMotionPlusCalibration(MotionPlusCalibrationData *calibration);
            Result GetBalanceBoardCalibration(BalanceBoardCalibrationData *calibration);

            Result SetReportMode(u8 mode);
            Result QueryStatus();

            Result InitializeStandardExtension();
            Result InitializeMotionPlus();

            Result ActivateMotionPlus();
            Result ActivateMotionPlusNunchuckPassthrough();
            Result ActivateMotionPlusClassicPassthrough();
            Result DeactivateMotionPlus();
            Result UpdateMotionPlusExtensionStatus(bool extension_connected);

            Result WriteMemory(u32 write_addr, const void *data, u8 size);
            Result ReadMemory(u32 read_addr, u16 size, void *out_data);

            WiiControllerOrientation m_orientation;
            WiiExtensionController m_extension;
            bool m_rumble_state;

            bool m_mp_extension_flag;
            bool m_mp_state_changing;

            WiiAccelerometerCalibrationData m_accel_calibration;

            union {
                MotionPlusCalibrationData motion_plus;
                BalanceBoardCalibrationData balance_board;
            } m_ext_calibration;
    };

}
