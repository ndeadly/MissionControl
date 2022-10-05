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
#include "switch_analog_stick.hpp"
#include "../bluetooth_mitm/bluetooth/bluetooth_types.hpp"
#include "../bluetooth_mitm/bluetooth/bluetooth_hid_report.hpp"
#include "../async/future_response.hpp"
#include <queue>

namespace ams::controller {

    using HidResponse = FutureResponse<bluetooth::HidEventType, bluetooth::HidReportEventInfo, uint8_t>;

    constexpr auto BATTERY_MAX = 8;

    enum SwitchPlayerNumber : uint8_t {
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
        uint16_t vid;
        uint16_t pid;
    };

    struct RGBColour {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    } __attribute__ ((__packed__));

    struct ProControllerColours {
        RGBColour body;
        RGBColour buttons;
        RGBColour left_grip;
        RGBColour right_grip;
    } __attribute__ ((__packed__));

    struct SwitchButtonData {
        uint8_t Y              : 1;
        uint8_t X              : 1;
        uint8_t B              : 1;
        uint8_t A              : 1;
        uint8_t                : 2; // SR, SL (Right Joy)
        uint8_t R              : 1;
        uint8_t ZR             : 1;

        uint8_t minus          : 1;
        uint8_t plus           : 1;
        uint8_t rstick_press   : 1;
        uint8_t lstick_press   : 1;
        uint8_t home           : 1;
        uint8_t capture        : 1;
        uint8_t                : 0;

        uint8_t dpad_down      : 1;
        uint8_t dpad_up        : 1;
        uint8_t dpad_right     : 1;
        uint8_t dpad_left      : 1;
        uint8_t                : 2; // SR, SL (Left Joy)
        uint8_t L              : 1;
        uint8_t ZL             : 1;
    } __attribute__ ((__packed__));

    struct Switch6AxisData {
        int16_t accel_x;
        int16_t accel_y;
        int16_t accel_z;
        int16_t gyro_1;
        int16_t gyro_2;
        int16_t gyro_3;
    } __attribute__ ((__packed__));

    struct Switch6AxisCalibrationData {
        struct {
            int16_t x;
            int16_t y;
            int16_t z;
        } acc_bias;

        struct {
            int16_t x;
            int16_t y;
            int16_t z;
        } acc_sensitivity;

        struct {
            int16_t roll;
            int16_t pitch;
            int16_t yaw;
        } gyro_bias;

        struct {
            int16_t roll;
            int16_t pitch;
            int16_t yaw;
        } gyro_sensitivity;
    } __attribute__((packed));

    struct Switch6AxisHorizontalOffset {
        int16_t x;
        int16_t y;
        int16_t z;
    } __attribute__((packed));

    struct SwitchRumbleDataEncoded {
        uint8_t left_motor[4];
        uint8_t right_motor[4];
    } __attribute__ ((__packed__));

    struct SwitchRumbleData {
        float high_band_freq;
        float high_band_amp;
        float low_band_freq;
        float low_band_amp;
    } __attribute__ ((__packed__));

    enum HidCommandType : uint8_t {
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

    struct SwitchHidCommand {
        uint8_t id;
        union {
            uint8_t data[0x26];

            struct {
                uint8_t id;
            } set_data_format;

            struct {
                uint32_t address;
                uint8_t size;
            } serial_flash_read;

            struct {
                uint32_t address;
                uint8_t size;
                uint8_t data[];
            } serial_flash_write;

            struct {
                uint32_t address;
            } serial_flash_sector_erase;

            struct {
                union {
                    uint8_t leds;

                    struct {
                        uint8_t leds_flash : 4;
                        uint8_t leds_on    : 4;
                    };
                };
            } set_indicator_led;

            struct {
                bool disabled;
            } sensor_sleep;

            struct {
                uint8_t gyro_sensitivity;
                uint8_t acc_sensitivity;
                uint8_t gyro_perf_rate;
                uint8_t acc_aa_bandwidth;
            } sensor_config;

            struct {
                bool enabled;
            } motor_enable;
        };
    } __attribute__ ((__packed__));

    struct SwitchHidCommandResponse {
        uint8_t ack;
        uint8_t id;
        union {
            uint8_t raw[0x23];

            struct {
                struct {
                    uint8_t major;
                    uint8_t minor;
                } fw_ver;
                uint8_t type;
                uint8_t _unk0;  // Always 0x02
                bluetooth::Address address;
                uint8_t _unk1;  // Always 0x01
                uint8_t _unk2;  // If 01, colors in SPI are used. Otherwise default ones
            } __attribute__ ((__packed__)) get_device_info;

            struct {
                bool enabled;
            } shipment;

            struct {
                uint32_t address;
                uint8_t size;
                uint8_t data[];
            } serial_flash_read;

            struct {
                uint8_t status;
            } serial_flash_write;

            struct {
                uint8_t status;
            } serial_flash_sector_erase;

            struct {
                union {
                    uint8_t leds;

                    struct {
                        uint8_t leds_flash : 4;
                        uint8_t leds_on    : 4;
                    };
                };
            } get_indicator_led;
        } data;
    } __attribute__ ((__packed__));

    struct SwitchNfcIrResponse {
        uint8_t data[0x138];
    } __attribute__ ((__packed__));

    struct SwitchInputReport {
        uint8_t id;
        uint8_t timer;
        uint8_t conn_info : 4;
        uint8_t battery   : 4;
        SwitchButtonData buttons;
        SwitchAnalogStick left_stick;
        SwitchAnalogStick right_stick;
        uint8_t vibrator;

        union {
            struct {
                SwitchHidCommandResponse hid_command_response;
            } type0x21;

            struct {
                uint8_t mcu_fw_data[37];
            } type0x23;

            struct {
                Switch6AxisData motion_data[3]; // IMU samples at 0, 5 and 10ms
            } type0x30;

            struct {
                Switch6AxisData motion_data[3]; // IMU samples at 0, 5 and 10ms
                SwitchNfcIrResponse nfc_ir_response;
                uint8_t crc;
            } type0x31;
        };
    } __attribute__ ((__packed__));

    struct SwitchOutputReport {
        uint8_t id;
        uint8_t counter;
        SwitchRumbleDataEncoded rumble_data;

        union {
            struct{
                SwitchHidCommand hid_command;
            } type0x01;

            struct {
                uint8_t nfc_ir_data[0x16];
            } type0x11;
        };
    } __attribute__ ((__packed__));

    Result LedsMaskToPlayerNumber(uint8_t led_mask, uint8_t *player_number);

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
            , m_id(id)
            , m_settsi_supported(true)
            , m_input_mutex(false)
            , m_output_mutex(false) { }

            virtual ~SwitchController() { };

            const bluetooth::Address& Address() const { return m_address; }

            virtual bool IsOfficialController() { return true; }
            virtual bool SupportsSetTsiCommand() { return m_settsi_supported; }

            virtual Result Initialize();

            virtual Result HandleDataReportEvent(const bluetooth::HidReportEventInfo *event_info);
            virtual Result HandleSetReportEvent(const bluetooth::HidReportEventInfo *event_info);
            virtual Result HandleGetReportEvent(const bluetooth::HidReportEventInfo *event_info);
            virtual Result HandleOutputDataReport(const bluetooth::HidReport *report);
        private:
            bool HasSetTsiDisableFlag();

        protected:
            Result WriteDataReport(const bluetooth::HidReport *report);
            Result WriteDataReport(const bluetooth::HidReport *report, uint8_t response_id, bluetooth::HidReport *out_report);
            Result SetFeatureReport(const bluetooth::HidReport *report);
            Result GetFeatureReport(uint8_t id, bluetooth::HidReport *out_report);

            virtual void UpdateControllerState(const bluetooth::HidReport *report);
            virtual void ApplyButtonCombos(SwitchButtonData *buttons);

            bluetooth::Address m_address;
            HardwareID m_id;

            bool m_settsi_supported;

            os::Mutex m_input_mutex;
            bluetooth::HidReport m_input_report;

            os::Mutex m_output_mutex;
            bluetooth::HidReport m_output_report;

            std::queue<std::shared_ptr<HidResponse>> m_future_responses;
    };

}
