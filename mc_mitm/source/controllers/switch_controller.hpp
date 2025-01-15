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
#include "switch_analog_stick.hpp"
#include "../bluetooth_mitm/bluetooth/bluetooth_types.hpp"
#include "../bluetooth_mitm/bluetooth/bluetooth_hid_report.hpp"
#include "../async/future_response.hpp"
#include "switch_rumble_handler.hpp"
#include "switch_motion_packing.hpp"
#include <queue>

namespace ams::controller {

    using HidResponse = FutureResponse<bluetooth::HidEventType, bluetooth::HidReportEventInfo, u8>;

    constexpr auto BATTERY_MAX = 8;

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

    struct ProControllerColours {
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
        u8              : 2; // SR, SL (Right Joy)
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
        u8              : 2; // SR, SL (Left Joy)
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

    enum HidCommandType : u8 {
        HidCommand_PairingOut             = 0x01,
        HidCommand_GetDeviceInfo          = 0x02,
        HidCommand_SetDataFormat          = 0x03,
        HidCommand_LRButtonDetection      = 0x04,
        HidCommand_Page                   = 0x05,
        HidCommand_Reset                  = 0x06,
        HidCommand_ClearPairingInfo       = 0x07,
        HidCommand_Shipment               = 0x08,
        HidCommand_SerialFlashRead        = 0x10,
        HidCommand_SerialFlashWrite       = 0x11,
        HidCommand_SerialFlashSectorErase = 0x12,
        HidCommand_McuReset               = 0x20,
        HidCommand_McuWrite               = 0x21,
        HidCommand_McuResume              = 0x22,
        HidCommand_McuPollingEnable       = 0x24,
        HidCommand_McuPollingDisable      = 0x25,
        HidCommand_AttachmentWrite        = 0x28,
        HidCommand_AttachmentRead         = 0x29,
        HidCommand_AttachmentEnable       = 0x2a,
        HidCommand_SetIndicatorLed        = 0x30,
        HidCommand_GetIndicatorLed        = 0x31,
        HidCommand_SetNotificationLed     = 0x38,
        HidCommand_SensorSleep            = 0x40,
        HidCommand_SensorConfig           = 0x41,
        HidCommand_SensorWrite            = 0x42,
        HidCommand_SensorRead             = 0x43,
        HidCommand_MotorEnable            = 0x48,
        HidCommand_GetBatteryVoltage      = 0x50,
        HidCommand_WriteChargeSetting     = 0x51,
        HidCommand_ReadChargeSetting      = 0x52,
    };

    enum McuCommandType : u8 {
        McuCommand_Invalid = 0x00,
        McuCommand_StateReport = 0x01,
        McuCommand_IrData = 0x03,
        McuCommand_BusyInitializing = 0x0b,
        McuCommand_IrStatus = 0x13,
        McuCommand_IrRegisters = 0x1b,
        McuCommand_ConfigureMcu = 0x21,
        McuCommand_ConfigureIr= 0x23,
        McuCommand_NfcState = 0x2a,
        McuCommand_NfcReadData = 0x3a,
        McuCommand_EmptyAwaitingCmd = 0xff,
    };

    enum McuSubCommandType : u8 {
        McuSubCommand_SetMcuMode = 0x00,
        McuSubCommand_GetMcuMode = 0x01,
        McuSubCommand_ReadDeviceMode = 0x02,
        McuSubCommand_WriteDeviceRegisters = 0x04,
    };

    enum McuModeType : u8 {
        McuMode_Suspended = 0,
        McuMode_Standby = 1,
        McuMode_Ringcon = 3,
        McuMode_Nfc = 4,
        McuMode_Ir = 5,
        McuMode_Busy = 6,
    };

    enum SensorSleepType : u8 {
        SensorSleepType_Inactive          = 0x0,
        SensorSleepType_Active            = 0x1,
        SensorSleepType_ActiveDscaleMode1 = 0x2,
        SensorSleepType_ActiveDscaleMode2 = 0x3,
        SensorSleepType_ActiveDscaleMode3 = 0x4,
        SensorSleepType_ActiveDscaleMode4 = 0x5,
    };

    enum SensorType : u8 {
        SensorType_LSM6DS3H   = 0x1,
        SensorType_ICM20600   = 0x3,
        SensorType_LSM6DS3TRC = 0x4
    };

    struct SwitchHidCommand {
        u8 id;
        union {
            u8 data[0x26];

            struct {
                u8 id;
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
                union {
                    u8 leds;

                    struct {
                        u8 leds_flash : 4;
                        u8 leds_on    : 4;
                    };
                };
            } set_indicator_led;

            struct {
                SensorSleepType mode;
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
                McuCommandType command;
                union {
                    u8 raw[0x25];
                    struct {
                        u8 pad;
                        McuModeType mode;
                    } configure_mcu;
                } data;
            } mcu_write;
            
            struct {
                bool enabled;
            } mcu_resume;
        };
    } PACKED;

    struct SwitchHidCommandResponse {
        u8 ack;
        u8 id;
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
                SensorType sensor_type;
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
                union {
                    u8 leds;

                    struct {
                        u8 leds_flash : 4;
                        u8 leds_on    : 4;
                    };
                };
            } get_indicator_led;
        } data;
    } PACKED;

    struct SwitchMcuCommand {
        McuSubCommandType sub_command;
        union {
            u8 raw[0x26];

            struct {
                McuModeType mode;
            } set_mcu_mode;
        } data;
    } PACKED;

    struct SwitchMcuResponse {
        McuCommandType command;
        union {
            u8 raw[0x137];

            struct {
                u8 pad[3];
                u8 unknown_1;
                u8 pad2;
                u8 unknown_2;
                McuModeType mode;
            } get_mcu_mode;
            
            struct {
                u8 pad;
                u8 unknown_1;
                u8 pad2[2];
                u8 unknown_2;
                u8 unknown_3;
                u8 is_ready;
            } read_device_mode;
        } data;
    } PACKED;

    struct SwitchInputReport {
        u8 id;
        u8 timer;
        u8 conn_info : 4;
        u8 battery   : 4;
        SwitchButtonData buttons;
        SwitchAnalogStick left_stick;
        SwitchAnalogStick right_stick;
        u8 vibrator;

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
        u8 id;
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

    Result LedsMaskToPlayerNumber(u8 led_mask, u8 *player_number);

    std::string GetControllerDirectory(const bluetooth::Address *address);

    class SwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = {
                {0x057e, 0x2006},   // Official Joycon(L) Controller
                {0x057e, 0x2007},   // Official Joycon(R) Controller/NES Online Controller
                {0x057e, 0x2009},   // Official Switch Pro Controller
                {0x057e, 0x2017},   // Official SNES Online Controller
                {0x057e, 0x2019},   // Official N64 Online Controller
                {0x057e, 0x201a}    // Official Genesis/Megadrive Online Controller
            };

            SwitchController(const bluetooth::Address *address, HardwareID id)
            : m_address(*address)
            , m_id(id) { }

            virtual ~SwitchController() { };

            const bluetooth::Address& Address() const { return m_address; }

            virtual bool IsOfficialController() { return true; }

            virtual Result Initialize();

            virtual Result HandleDataReportEvent(const bluetooth::HidReportEventInfo *event_info);
            virtual Result HandleSetReportEvent(const bluetooth::HidReportEventInfo *event_info);
            virtual Result HandleGetReportEvent(const bluetooth::HidReportEventInfo *event_info);
            virtual Result HandleOutputDataReport(const bluetooth::HidReport *report);

        protected:
            Result WriteDataReport(const bluetooth::HidReport *report);
            Result WriteDataReport(const bluetooth::HidReport *report, u8 response_id, bluetooth::HidReport *out_report);
            Result SetReport(BtdrvBluetoothHhReportType type, const bluetooth::HidReport *report);
            Result GetReport(u8 id, BtdrvBluetoothHhReportType type, bluetooth::HidReport *out_report);

            virtual void UpdateControllerState(const bluetooth::HidReport *report);
            virtual void ApplyButtonCombos(SwitchButtonData *buttons);

            bluetooth::Address m_address;
            HardwareID m_id;

            os::SdkMutex m_input_mutex;
            bluetooth::HidReport m_input_report;

            os::SdkMutex m_output_mutex;
            bluetooth::HidReport m_output_report;

            std::queue<std::shared_ptr<HidResponse>> m_future_responses;
    };

}
