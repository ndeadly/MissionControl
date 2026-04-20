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
#pragma once
#include <stratosphere.hpp>
#include "switch_power_info.hpp"
#include "switch_player_indicator.hpp"
#include "switch_analog_stick.hpp"
#include "switch_rumble_handler.hpp"
#include "switch_motion_packing.hpp"
#include "switch_mcu_command_processor.hpp"

namespace ams::controller {

     enum SwitchPlayerNumber : u8 {
        SwitchPlayerNumber_One,
        SwitchPlayerNumber_Two,
        SwitchPlayerNumber_Three,
        SwitchPlayerNumber_Four,
        SwitchPlayerNumber_Five,
        SwitchPlayerNumber_Six,
        SwitchPlayerNumber_Seven,
        SwitchPlayerNumber_Eight,
        SwitchPlayerNumber_Unknown = 0xf
    };

    struct HardwareID {
        u16 vid;
        u16 pid;
    };

    struct RGBColour {
        u8 r;
        u8 g;
        u8 b;
    } PACKED;

    struct SwitchControllerColours {
        RGBColour body;
        RGBColour buttons;
        RGBColour left_grip;
        RGBColour right_grip;
    } PACKED;

    struct SwitchButtonData {
        u8 Y            : 1;
        u8 X            : 1;
        u8 B            : 1;
        u8 A            : 1;
        u8 SR_R         : 1;
        u8 SL_R         : 1;
        u8 R            : 1;
        u8 ZR           : 1;

        u8 minus        : 1;
        u8 plus         : 1;
        u8 rstick_press : 1;
        u8 lstick_press : 1;
        u8 home         : 1;
        u8 capture      : 1;
        u8              : 0;

        u8 dpad_down    : 1;
        u8 dpad_up      : 1;
        u8 dpad_right   : 1;
        u8 dpad_left    : 1;
        u8 SR_L         : 1;
        u8 SL_L         : 1;
        u8 L            : 1;
        u8 ZL           : 1;
    } PACKED;

    struct Switch6AxisCalibrationData {
        struct {
            s16 x;
            s16 y;
            s16 z;
        } acc_bias;

        struct {
            s16 x;
            s16 y;
            s16 z;
        } acc_sensitivity;

        struct {
            s16 roll;
            s16 pitch;
            s16 yaw;
        } gyro_bias;

        struct {
            s16 roll;
            s16 pitch;
            s16 yaw;
        } gyro_sensitivity;
    } PACKED;

    struct Switch6AxisHorizontalOffset {
        s16 x;
        s16 y;
        s16 z;
    } PACKED;

    enum class SwitchHidReportId : u8 {
        CommandOutputReport    = 0x01,
        McuUpdateOutputReport  = 0x03,
        BasicOutputReport      = 0x10,
        McuOutputReport        = 0x11,
        AttachmentOutputReport = 0x12,
        CommandInputReport     = 0x21,
        McuUpdateInputReport   = 0x23,
        BasicInputReport       = 0x30,
        McuInputReport         = 0x31,
        AttachmentInputReport  = 0x32,
        Report0x33             = 0x33,
        GenericInputReport     = 0x3F,
        OtaEnableFwuReport     = 0x70,
        OtaSetupReadReport     = 0x71,
        OtaReadReport          = 0x72,
        OtaWriteReport         = 0x73,
        OtaEraseReport         = 0x74,
        OtaLaunchReport        = 0x75,
        ExtGripOutputReport    = 0x80,
        ExtGripInputReport     = 0x81,
        Report0x82             = 0x82,
    };

    enum class SwitchHidCommandId : u8 {
        PairingOut                       = 0x01,
        GetDeviceInfo                    = 0x02,
        SetDataFormat                    = 0x03,
        LRButtonDetection                = 0x04,
        Page                             = 0x05,
        Reset                            = 0x06,
        ClearPairingInfo                 = 0x07,
        Shipment                         = 0x08,
        SerialFlashRead                  = 0x10,
        SerialFlashWrite                 = 0x11,
        SerialFlashSectorErase           = 0x12,
        McuReset                         = 0x20,
        McuWrite                         = 0x21,
        McuResume                        = 0x22,
        McuPollingEnable                 = 0x24,
        McuPollingDisable                = 0x25,
        AttachmentWrite                  = 0x28,
        AttachmentRead                   = 0x29,
        AttachmentEnable                 = 0x2A,
        GetAttachmentInfo                = 0x2B,
        SetIndicatorLed                  = 0x30,
        GetIndicatorLed                  = 0x31,
        SetNotificationLed               = 0x38,
        SensorSleep                      = 0x40,
        SensorConfig                     = 0x41,
        SensorWrite                      = 0x42,
        SensorRead                       = 0x43,
        MotorEnable                      = 0x48,
        GetBatteryVoltage                = 0x50,
        WriteChargeSetting               = 0x51,
        ReadChargeSetting                = 0x52,
        ExtDevWrite                      = 0x58,
        GetExtDevInfo                    = 0x59,
        ExtDevPollingEnable              = 0x5A,
        ExtDevPollingDisable             = 0x5B,
        ExtDevInFormatConfig             = 0x5C,
        Command0x5D                      = 0x5D,
        InternalAttachmentWrite          = 0x60,
        Command0x61                      = 0x61,
        InternalAttachmentEnable         = 0x62,
        InternalAttachmentPollingEnable  = 0x63,
        InternalAttachmentPollingDisable = 0x64,
        AddButtonPlayData                = 0x7A,
        StartStopButtonPlayData          = 0x7B,
    };

    enum class SwitchHidCommandAck : u8 {
        Default                = 0x80,
        PairingIn              = 0x81,
        DeviceInfo             = 0x82,
        LRButtonElapsedTime    = 0x83,
        SerialFlashData        = 0x90,
        McuData                = 0xA0,
        AttachmentData         = 0xA8,
        AttachmentInfo         = 0xA9,
        InternalAttachmentRead = 0xAA,
        IndicatorLed           = 0xB0,
        SensorData             = 0xC0,
        BatteryVoltage         = 0xD0,
        ExtDevRead             = 0xD8,
        ExtDevInfo             = 0xD9,
    };

    struct SwitchHidCommand {
        SwitchHidCommandId id;
        union {
            u8 data[0x26];

            struct {
                SwitchHidReportId id;
            } set_data_format;

            struct {
                u32 address;
                u8 size;
            } serial_flash_read;

            struct {
                u32 address;
                u8 size;
                u8 data[];
            } serial_flash_write;

            struct {
                u32 address;
            } serial_flash_sector_erase;

            struct {
                SwitchPlayerIndicatorType led_state;
            } set_indicator_led;

            struct {
                SwitchImuSensorSleepMode mode;
            } sensor_sleep;

            struct {
                GyroSensitivity gyro_sensitivity;
                AccelSensitivity accel_sensitivity;
                u8 gyro_perf_rate;
                u8 acc_aa_bandwidth;
            } sensor_config;

            struct {
                bool enabled;
            } motor_enable;

            struct {
                SwitchMcuCommandId command;
                union {
                    u8 raw[0x25];
                    struct {
                        u8 pad;
                        SwitchMcuMode mode;
                    } configure_mcu;
                } data;
            } mcu_write;
            
            struct {
                bool enabled;
            } mcu_resume;
        };
    } PACKED;

    struct SwitchHidCommandResponse {
        SwitchHidCommandAck ack;
        SwitchHidCommandId id;
        union {
            u8 raw[0x23];

            struct {
                struct {
                    u8 major;
                    u8 minor;
                } fw_ver;
                u8 type;
                u8 _unk0;  // Always 0x02
                bluetooth::Address address;
                SwitchImuSensorType sensor_type;
                u8 format_version;  // If 01, colors in SPI are used. Otherwise default ones
            } __attribute__ ((__packed__)) get_device_info;

            struct {
                bool enabled;
            } shipment;

            struct {
                u32 address;
                u8 size;
                u8 data[];
            } serial_flash_read;

            struct {
                u8 status;
            } serial_flash_write;

            struct {
                u8 status;
            } serial_flash_sector_erase;

            struct {
                SwitchPlayerIndicatorType led_state;
            } get_indicator_led;
        } data;
    } PACKED;

    struct SwitchInputReport {
        SwitchHidReportId id;
        u8 latency_timer;
        SwitchPowerInfoType power_info;
        SwitchButtonData buttons;
        SwitchAnalogStickType left_stick;
        SwitchAnalogStickType right_stick;
        u8 motor_status;

        union {
            struct {
                SwitchHidCommandResponse hid_command_response;
            } type0x21;

            struct {
                u8 mcu_fw_data[37];
            } type0x23;

            struct {
                SwitchMotionData motion_data; // IMU samples at 0, 5 and 10ms
            } type0x30;

            struct {
                SwitchMotionData motion_data; // IMU samples at 0, 5 and 10ms
                SwitchMcuResponse mcu_response;
                u8 crc;
            } type0x31;
        };
    } PACKED;

    struct SwitchOutputReport {
        SwitchHidReportId id;
        u8 counter;
        SwitchEncodedMotorData enc_motor_data;

        union {
            struct{
                SwitchHidCommand hid_command;
            } type0x01;

            struct {
                SwitchMcuCommand mcu_command;
            } type0x11;
        };
    } PACKED;

}
