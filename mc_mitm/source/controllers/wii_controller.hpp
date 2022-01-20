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
            uint8_t raw[2];

            struct {
                uint8_t dpad_left   : 1;
                uint8_t dpad_right  : 1;
                uint8_t dpad_down   : 1;
                uint8_t dpad_up     : 1;
                uint8_t plus        : 1;
                uint8_t             : 0;
                
                uint8_t two         : 1;
                uint8_t one         : 1;
                uint8_t B           : 1;
                uint8_t A           : 1;
                uint8_t minus       : 1;
                uint8_t             : 2;
                uint8_t home        : 1;
            };
        };
    } __attribute__ ((__packed__));

    struct WiiAccelerometerData {
        uint8_t x;
        uint8_t y;
        uint8_t z;
    } __attribute__ ((__packed__));

    struct WiiAccelerometerCalibrationData {
        uint16_t acc_x_0g;
        uint16_t acc_y_0g;
        uint16_t acc_z_0g;

        uint16_t acc_x_1g;
        uint16_t acc_y_1g;
        uint16_t acc_z_1g;
    } __attribute__ ((__packed__));

    struct WiiClassicControllerExtensionData {
        struct {
            uint8_t left_stick_x     : 6;
            uint8_t right_stick_x_43 : 2;

            uint8_t left_stick_y     : 6;
            uint8_t right_stick_x_21 : 2;

            uint8_t right_stick_y    : 5;
            uint8_t left_trigger_43  : 2;
            uint8_t right_stick_x_0  : 1;

            uint8_t right_trigger    : 5;
            uint8_t left_trigger_20  : 3;
        };

        struct {
            uint8_t             : 1;
            uint8_t R           : 1;
            uint8_t plus        : 1;
            uint8_t home        : 1;
            uint8_t minus       : 1;
            uint8_t L           : 1;
            uint8_t dpad_down   : 1;
            uint8_t dpad_right  : 1;

            uint8_t dpad_up     : 1;
            uint8_t dpad_left   : 1;
            uint8_t ZR          : 1;
            uint8_t X           : 1; 
            uint8_t A           : 1;
            uint8_t Y           : 1;
            uint8_t B           : 1;
            uint8_t ZL          : 1;
        } buttons;
    } __attribute__ ((__packed__));

    struct WiiClassicControllerPassthroughExtensionData {
        union {
            struct {
                uint8_t                  : 1;
                uint8_t left_stick_x_51  : 5;
                uint8_t right_stick_x_43 : 2;

                uint8_t                  : 1;
                uint8_t left_stick_y_51  : 5;
                uint8_t right_stick_x_21 : 2;

                uint8_t right_stick_y    : 5;
                uint8_t left_trigger_43  : 2;
                uint8_t right_stick_x_0  : 1;

                uint8_t right_trigger    : 5;
                uint8_t left_trigger_20  : 3;

                uint8_t extension_connected : 1;
                uint8_t                     : 0;

                uint8_t                   : 1;
                uint8_t motionplus_report : 1;
                uint8_t                   : 0;
            };

            struct {
                uint8_t dpad_up    : 1;
                uint8_t            : 0;
 
                uint8_t dpad_left  : 1;
                uint8_t            : 0;

                uint8_t pad[2];

                uint8_t            : 1;
                uint8_t R          : 1;
                uint8_t plus       : 1;
                uint8_t home       : 1;
                uint8_t minus      : 1;
                uint8_t L          : 1;
                uint8_t dpad_down  : 1;
                uint8_t dpad_right : 1;

                uint8_t            : 2;
                uint8_t ZR         : 1;
                uint8_t X          : 1; 
                uint8_t A          : 1;
                uint8_t Y          : 1;
                uint8_t B          : 1;
                uint8_t ZL         : 1;
            } buttons;
        };
    } __attribute__ ((__packed__));

    struct WiiNunchuckExtensionData {
        uint8_t stick_x;
        uint8_t stick_y;
        uint8_t accel_x_92;
        uint8_t accel_y_92;
        uint8_t accel_z_92;

        uint8_t Z : 1;
        uint8_t C : 1;
        uint8_t accel_x_10 : 2;
        uint8_t accel_y_10 : 2;
        uint8_t accel_z_10 : 2; 
    } __attribute__ ((__packed__));

    struct WiiNunchuckPassthroughExtensionData {
        uint8_t stick_x;
        uint8_t stick_y;
        uint8_t accel_x_92;
        uint8_t accel_y_92;
        uint8_t extension_connected : 1;
        uint8_t accel_z_93          : 7;

        uint8_t                   : 1;
        uint8_t motionplus_report : 1;
        uint8_t Z                 : 1;
        uint8_t C                 : 1;
        uint8_t accel_x_1         : 1;
        uint8_t accel_y_1         : 1;
        uint8_t accel_z_21        : 2; 

    } __attribute__ ((__packed__));

    struct WiiUProButtonData {
        uint8_t             : 1;
        uint8_t R           : 1;
        uint8_t plus        : 1;
        uint8_t home        : 1;
        uint8_t minus       : 1;
        uint8_t L           : 1;
        uint8_t dpad_down   : 1;
        uint8_t dpad_right  : 1;

        uint8_t dpad_up     : 1;
        uint8_t dpad_left   : 1;
        uint8_t ZR          : 1;
        uint8_t X           : 1; 
        uint8_t A           : 1;
        uint8_t Y           : 1;
        uint8_t B           : 1;
        uint8_t ZL          : 1;

        uint8_t rstick_press  : 1;
        uint8_t lstick_press  : 1;
        uint8_t charging      : 1;
        uint8_t usb_connected : 1;
        uint8_t battery       : 3;
        uint8_t               : 1;
    } __attribute__ ((__packed__));

    struct WiiUProExtensionData {
        uint16_t left_stick_x;
        uint16_t right_stick_x;
        uint16_t left_stick_y;
        uint16_t right_stick_y;
        WiiUProButtonData buttons;
    } __attribute__ ((__packed__));

    struct MotionPlusExtensionData {
        uint8_t yaw_speed_lo;
        uint8_t roll_speed_lo;
        uint8_t pitch_speed_lo;

        uint8_t pitch_slow_mode : 1;
        uint8_t yaw_slow_mode   : 1;
        uint8_t yaw_speed_hi    : 6;

        uint8_t extension_connected : 1;
        uint8_t roll_slow_mode      : 1;
        uint8_t roll_speed_hi       : 6;

        uint8_t                   : 1;  
        uint8_t motionplus_report : 1;
        uint8_t pitch_speed_hi    : 6;
    } __attribute__ ((__packed__));

    struct MotionPlusCalibration {
        uint16_t yaw_zero;
        uint16_t roll_zero;
        uint16_t pitch_zero;
        uint16_t yaw_scale;
        uint16_t roll_scale;
        uint16_t pitch_scale;
        uint8_t degrees_div_6;
    } __attribute__ ((__packed__));

    struct MotionPlusCalibrationData {
        MotionPlusCalibration fast;
        MotionPlusCalibration slow;
    } __attribute__ ((__packed__));

    struct TaTaConExtensionData {
        uint8_t _unk0[5];

        uint8_t             : 3;
        uint8_t R_rim       : 1;
        uint8_t R_center    : 1;
        uint8_t L_rim       : 1;
        uint8_t L_center    : 1;
        uint8_t             : 0;
    } __attribute__ ((__packed__));

    struct BalanceBoardExtensionData {
        uint16_t top_right;
        uint16_t bottom_right;
        uint16_t top_left;
        uint16_t bottom_left;
        uint8_t temperature;
        uint8_t _pad;
        uint8_t battery;
    } __attribute__ ((__packed__));

    struct BalanceBoardCalibrationData {
        uint16_t top_right_0kg;
        uint16_t bottom_right_0kg;
        uint16_t top_left_0kg;
        uint16_t bottom_left_0kg;

        uint16_t top_right_17kg;
        uint16_t bottom_right_17kg;
        uint16_t top_left_17kg;
        uint16_t bottom_left_17kg;

        uint16_t top_right_34kg;
        uint16_t bottom_right_34kg;
        uint16_t top_left_34kg;
        uint16_t bottom_left_34kg;
    } __attribute__ ((__packed__));

    struct WiiOutputReport0x10 {
        uint8_t rumble  : 1;
        uint8_t         : 0;
    } __attribute__ ((__packed__));

    struct WiiOutputReport0x11 {
        uint8_t rumble  : 1;
        uint8_t         : 3;
        uint8_t leds    : 4;
    } __attribute__ ((__packed__));

    struct WiiOutputReport0x12 {
        uint8_t rumble  : 1;
        uint8_t         : 0;
        uint8_t report_mode;
    } __attribute__ ((__packed__));

    struct WiiOutputReport0x14 {
        uint8_t                : 5;
        uint8_t speaker_enable : 1;
        uint8_t                : 0;
    } __attribute__ ((__packed__));

    struct WiiOutputReport0x15 {
        uint8_t rumble  : 1;
        uint8_t         : 0;
    } __attribute__ ((__packed__));

    struct WiiOutputReport0x16 {
        uint32_t address;
        uint8_t  size;
        uint8_t  data[16];
    } __attribute__ ((__packed__));

    struct WiiOutputReport0x17 {
        uint32_t address;
        uint16_t size;
    } __attribute__ ((__packed__));

    struct WiiOutputReport0x18 {
        uint8_t size;
        uint8_t speaker_data[20];
    } __attribute__ ((__packed__));

    struct WiiOutputReport0x19 {
        uint8_t : 5;
        uint8_t speaker_mute : 1;
        uint8_t : 0;
    } __attribute__ ((__packed__));

    struct WiiInputReport0x20 {
        WiiButtonData buttons;
        uint8_t battery_critical    : 1;
        uint8_t extension_connected : 1;
        uint8_t speaker_enabled     : 1;
        uint8_t ir_enabled          : 1;
        uint8_t led_state           : 4;
        uint8_t _pad[2];
        uint8_t battery;
    } __attribute__ ((__packed__));

    struct WiiInputReport0x21 {
        WiiButtonData buttons;
        uint8_t error : 4;
        uint8_t size  : 4;
        uint16_t address;
        uint8_t data[16];
    } __attribute__ ((__packed__));

    struct WiiInputReport0x22 {
        WiiButtonData buttons;
        uint8_t report_id;
        uint8_t error;
    } __attribute__ ((__packed__));

    struct WiiInputReport0x30 {
        WiiButtonData buttons;
    } __attribute__ ((__packed__));

    struct WiiInputReport0x31 {
        WiiButtonData buttons;
        WiiAccelerometerData accel;
    } __attribute__ ((__packed__));

    struct WiiInputReport0x32 {
        WiiButtonData buttons;
        uint8_t extension[8];
    } __attribute__ ((__packed__));

    struct WiiInputReport0x33 {
        WiiButtonData buttons;
        WiiAccelerometerData accel;
        uint8_t ir[12];
    } __attribute__ ((__packed__));

    struct WiiInputReport0x34 {
        WiiButtonData buttons;
        uint8_t extension[19];
    } __attribute__ ((__packed__));

    struct WiiInputReport0x35 {
        WiiButtonData buttons;
        WiiAccelerometerData accel;
        uint8_t extension[16];
    } __attribute__ ((__packed__));

    struct WiiInputReport0x36 {
        WiiButtonData buttons;
        uint8_t ir[10];
        uint8_t extension[9];
    } __attribute__ ((__packed__));

    struct WiiInputReport0x37 {
        WiiButtonData buttons;
        WiiAccelerometerData accel;
        uint8_t ir[10];
        uint8_t extension[6];
    } __attribute__ ((__packed__));

    struct WiiInputReport0x3d {
        uint8_t extension[21];
    } __attribute__ ((__packed__));

    struct WiiInputReport0x3e {
        WiiButtonData buttons;
        uint8_t accel;
        uint8_t ir[18];
    } __attribute__ ((__packed__));

    struct WiiInputReport0x3f {
        WiiButtonData buttons;
        uint8_t accel;
        uint8_t ir[18];
    } __attribute__ ((__packed__));

    struct WiiReportData {
        uint8_t id;
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
    } __attribute__ ((__packed__));

    class WiiController : public EmulatedSwitchController {

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
            Result SetVibration(const SwitchRumbleData *rumble_data);
            Result CancelVibration();
            Result SetPlayerLed(uint8_t led_mask);
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
            void MapExtensionBytes(const uint8_t ext[]);
            void MapNunchuckExtension(const uint8_t ext[]);
            void MapClassicControllerExtension(const uint8_t ext[]);
            void MapWiiUProControllerExtension(const uint8_t ext[]);
            void MapTaTaConExtension(const uint8_t ext[]);
			void MapBalanceBoardExtension(const uint8_t ext[]);
            void MapMotionPlusExtension(const uint8_t ext[]);
            void MapNunchuckExtensionPassthroughMode(const uint8_t ext[]);
            void MapClassicControllerExtensionPassthroughMode(const uint8_t ext[]);

            void HandleStatusReport(const WiiReportData *wii_report);

            WiiExtensionController GetExtensionControllerType();
            MotionPlusStatus GetMotionPlusStatus();

            Result GetAccelerometerCalibration(WiiAccelerometerCalibrationData *calibration);
            Result GetMotionPlusCalibration(MotionPlusCalibrationData *calibration);
            Result GetBalanceBoardCalibration(BalanceBoardCalibrationData *calibration);

            Result SetReportMode(uint8_t mode);
            Result QueryStatus();

            Result InitializeStandardExtension();
            Result InitializeMotionPlus();

            Result ActivateMotionPlus();
            Result ActivateMotionPlusNunchuckPassthrough();
            Result ActivateMotionPlusClassicPassthrough();
            Result DeactivateMotionPlus();
            Result UpdateMotionPlusExtensionStatus(bool extension_connected);

            Result WriteMemory(uint32_t write_addr, const void *data, uint8_t size);
            Result ReadMemory(uint32_t read_addr, uint16_t size, void *out_data);

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
