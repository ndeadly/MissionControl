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
#pragma once
#include "switch_analog_stick.hpp"
#include "../bluetooth_mitm/bluetooth/bluetooth_types.hpp"
#include "../bluetooth_mitm/bluetooth/bluetooth_hid_report.hpp"

namespace ams::controller {

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
		
    enum SwitchControllerType : uint8_t {
        SwitchControllerType_LeftJoyCon     = 1,
        SwitchControllerType_RightJoyCon    = 2,
        SwitchControllerType_ProController  = 3,
    };

    struct FirmwareVersion {
        uint8_t major;
        uint8_t minor;
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
        uint8_t SR_R           : 1;
        uint8_t SL_R           : 1;
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
        uint8_t SR_L           : 1;
        uint8_t SL_L           : 1;
        uint8_t L              : 1;
        uint8_t ZL             : 1;
    } __attribute__ ((__packed__));

    struct Switch6AxisData {
        uint16_t    accel_x;
        uint16_t    accel_y;
        uint16_t    accel_z;
        uint16_t    gyro_1;
        uint16_t    gyro_2;
        uint16_t    gyro_3;
    } __attribute__ ((__packed__));

    struct SwitchRumbleData {
        float high_band_freq;
        float high_band_amp;
        float low_band_freq;
        float low_band_amp;
    } __attribute__ ((__packed__));

    enum SubCmdType : uint8_t {
        SubCmd_GetControllerState   = 0x00,
        SubCmd_ManualPair           = 0x01,
        SubCmd_RequestDeviceInfo    = 0x02,
        SubCmd_SetInputReportMode   = 0x03,
        SubCmd_TriggersElapsedTime  = 0x04,
        SubCmd_GetPageListState     = 0x05,
        SubCmd_SetHciState          = 0x06,
        SubCmd_ResetPairingInfo     = 0x07,
        SubCmd_SetShipPowerState    = 0x08,
        SubCmd_SpiFlashRead         = 0x10,
        SubCmd_SpiFlashWrite        = 0x11,
        SubCmd_SpiSectorErase       = 0x12,
        SubCmd_ResetMcu             = 0x20,
        SubCmd_SetMcuConfig         = 0x21,
        SubCmd_SetMcuState          = 0x22,
        SubCmd_SetPlayerLeds        = 0x30,
        SubCmd_GetPlayerLeds        = 0x31,
        SubCmd_SetHomeLed           = 0x38,
        SubCmd_EnableImu            = 0x40,
        SubCmd_SetImuSensitivity    = 0x41,
        SubCmd_WriteImuRegisters    = 0x42,
        SubCmd_ReadImuRegisters     = 0x43,
        SubCmd_EnableVibration      = 0x48,
        SubCmd_GetRegulatedVoltage  = 0x50,
        SubCmd_SetGpioPinValue      = 0x51,
        SubCmd_GetGpioPinValue      = 0x52,
    };

    struct SwitchSubcommand {
        uint8_t id;
        union {
            uint8_t data[0x26];

            struct {
                uint32_t address;
                uint8_t size;
            } spi_flash_read;
        };
    } __attribute__ ((__packed__));

    struct SwitchSubcommandResponse {
        uint8_t ack;
        uint8_t id;
        union {
            uint8_t data[0x23];

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
            } __attribute__ ((__packed__)) device_info;

            struct {
                bool enabled;
            } set_ship_power_state;

            struct {
                uint32_t address;
                uint8_t size;
                uint8_t data[];
            } spi_flash_read;

            struct {
                uint8_t status;
            } spi_flash_write;

            struct {
                uint8_t status;
            } spi_sector_erase;
        };
    } __attribute__ ((__packed__));

    struct SwitchOutputReport0x01 {
        uint8_t counter;
        uint8_t rumble_data[8];
        SwitchSubcommand subcmd;
    } __attribute__ ((__packed__));

    struct SwitchOutputReport0x03;

    struct SwitchOutputReport0x10 {
        uint8_t timer;
        uint8_t left_motor[4];
        uint8_t right_motor[4];
    }__attribute__ ((__packed__));

    struct SwitchOutputReport0x11;
    struct SwitchOutputReport0x12;

    struct SwitchInputReport0x21 {
        uint8_t                     timer;
        uint8_t                     conn_info      : 4;
        uint8_t                     battery        : 4;
        SwitchButtonData            buttons;
        SwitchAnalogStick           left_stick;
        SwitchAnalogStick           right_stick;
        uint8_t                     vibrator;
        SwitchSubcommandResponse    response;
    } __attribute__ ((__packed__));

    struct SwitchInputReport0x23;

    struct SwitchInputReport0x30 {
        uint8_t             timer;
        uint8_t             conn_info      : 4;
        uint8_t             battery        : 4;
        SwitchButtonData    buttons;
        SwitchAnalogStick           left_stick;
        SwitchAnalogStick           right_stick;
        uint8_t             vibrator;

        // IMU samples at 0, 5 and 10ms
        Switch6AxisData     motion[3];
    } __attribute__ ((__packed__));

    struct SwitchInputReport0x31;
    struct SwitchInputReport0x32;
    struct SwitchInputReport0x33;
    struct SwitchInputReport0x3f;

    struct SwitchReportData {
        uint8_t id;
        union {
            SwitchOutputReport0x01 output0x01;
            //SwitchOutputReport0x03 output0x03;
            SwitchOutputReport0x10 output0x10;
            //SwitchOutputReport0x11 output0x11;
            //SwitchOutputReport0x12 output0x12;
            SwitchInputReport0x21  input0x21;
            SwitchInputReport0x30  input0x30;
            //SwitchInputReport0x31  input0x31;
            //SwitchInputReport0x32  input0x32;
            //SwitchInputReport0x33  input0x33;
            //SwitchInputReport0x3f  input0x3f;

        };
    } __attribute__ ((__packed__));

    Result LedsMaskToPlayerNumber(uint8_t led_mask, uint8_t *player_number);
	
    constexpr const FirmwareVersion joycon_fw_version         = {0x04, 0x07};
    constexpr const FirmwareVersion pro_controller_fw_version = {0x03, 0x48};

    class SwitchController {

        public: 
            static constexpr const HardwareID hardware_ids[] = { 
                {0x057e, 0x2006},   // Official Joycon(L) Controller
                {0x057e, 0x2007},   // Official Joycon(R) Controller/NES Online Controller
                {0x057e, 0x2009},   // Official Switch Pro Controller
                {0x057e, 0x2017}    // Official SNES Online Controller
            };

            SwitchController(const bluetooth::Address *address)
                : m_address(*address) { };

            const bluetooth::Address& Address(void) const { return m_address; };

            virtual bool IsOfficialController(void) { return true; };

            virtual Result Initialize(void) { return ams::ResultSuccess(); };
            virtual Result HandleIncomingReport(const bluetooth::HidReport *report);
            virtual Result HandleOutgoingReport(const bluetooth::HidReport *report);

        protected:
            virtual void ApplyButtonCombos(SwitchButtonData *buttons);

            bluetooth::Address m_address;

            static bluetooth::HidReport s_input_report;
            static bluetooth::HidReport s_output_report;
    };

}
