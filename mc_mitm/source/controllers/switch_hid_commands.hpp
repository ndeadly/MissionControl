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
#include "switch_sixaxis_types.hpp"
#include "switch_mcu_types.hpp"
#include "switch_player_indicator.hpp"
#include "../bluetooth_mitm/bluetooth/bluetooth_types.hpp"

namespace ams::controller {

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

        Ack                              = 0x80,
        PairingIn                        = 0x81,
        DeviceInfo                       = 0x82,
        LRButtonElapsedTime              = 0x83,
        SerialFlashData                  = 0x90,
        McuData                          = 0xA0,
        AttachmentData                   = 0xA8,
        AttachmentInfo                   = 0xA9,
        InternalAttachmentRead           = 0xAA,
        IndicatorLed                     = 0xB0,
        SensorData                       = 0xC0,
        BatteryVoltage                   = 0xD0,
        ExtDevRead                       = 0xD8,
        ExtDevInfo                       = 0xD9,
    };

    struct SwitchHidCommandPairing {
        u8 stage;
        union {
            struct {
                bluetooth::Address address;
            } stage1;
        };
    } PACKED;

    struct SwitchHidCommandSetDataFormat {
        // SwitchHidReportId id;
        u8 format;
    } PACKED;

    struct SwitchHidCommandReset {
        u8 mode;
    } PACKED;

    struct SwitchHidCommandShipment {
        bool enable;
    } PACKED;

    struct SwitchHidCommandSerialFlashRead {
        u32 address;
        u8 size;
    } PACKED;

    struct SwitchHidCommandSerialFlashWrite {
        u32 address;
        u8 size;
        u8 data[0x1D];
    } PACKED;

    struct SwitchHidCommandSerialFlashSectorErase {
        u32 address;
    } PACKED;

    struct SwitchHidCommandMcuWrite {
        SwitchMcuCommandId command;
        union {
            u8 raw[0x25];
            struct {
                u8 pad;
                SwitchMcuMode mode;
            } configure_mcu;
        } data;
    } PACKED;

    struct SwitchHidCommandMcuResume {
        u8 mode;
    } PACKED;

    struct SwitchHidCommandMcuPollingEnable {
        u8 data[0x26];
    } PACKED;

    struct SwitchHidCommandAttachmentWrite {
        u8 data[0x26];
    } PACKED;

    struct SwitchHidCommandAttachmentEnable {
        bool enable;
    } PACKED;

    struct SwitchHidCommandSetIndicatorLed {
        SwitchPlayerIndicatorType led_state;
    } PACKED;

    struct SwitchHidCommandSetNotificationLed {
        u8 data[0x19];
    } PACKED;

    struct SwitchHidCommandSensorSleep {
        SwitchImuSensorSleepMode mode;
    } PACKED;

    struct SwitchHidCommandSensorConfig {
        GyroSensitivity gyro_sensitivity;
        AccelSensitivity accel_sensitivity;
        GyroPerformanceRate gyro_perf_rate;
        AccelAntiAliasFilterBandwidth acc_aa_bandwidth;
    } PACKED;

    struct SwitchHidCommandSensorWrite {
        u8 register_address;
        u8 write;
        u8 write_value;
    } PACKED;

    struct SwitchHidCommandSensorRead {
        u8 register_address;
        u8 register_count;
    } PACKED;

    struct SwitchHidCommandMotorEnable {
        bool enable;
    } PACKED;

    struct SwitchHidCommandWriteChargeSetting {
        u8 setting;
    } PACKED;

    struct SwitchHidCommand {
        SwitchHidCommandId id;
        union {
            u8 data[0x26];
            SwitchHidCommandPairing pairing;
            SwitchHidCommandSetDataFormat set_data_format;
            SwitchHidCommandReset reset;
            SwitchHidCommandShipment shipment;
            SwitchHidCommandSerialFlashRead serial_flash_read;
            SwitchHidCommandSerialFlashWrite serial_flash_write;
            SwitchHidCommandSerialFlashSectorErase serial_flash_sector_erase;
            SwitchHidCommandMcuWrite mcu_write;
            SwitchHidCommandMcuResume mcu_resume;
            SwitchHidCommandMcuPollingEnable mcu_polling_enable;
            SwitchHidCommandAttachmentWrite attachment_write;
            SwitchHidCommandAttachmentEnable attachment_enable;
            SwitchHidCommandSetIndicatorLed set_indicator_led;
            SwitchHidCommandSetNotificationLed set_notification_led;
            SwitchHidCommandSensorSleep sensor_sleep;
            SwitchHidCommandSensorConfig sensor_config;
            SwitchHidCommandSensorWrite sensor_write;
            SwitchHidCommandSensorRead sensor_read;
            SwitchHidCommandMotorEnable motor_enable;
            SwitchHidCommandWriteChargeSetting charge_setting;
        };
    };
    static_assert(sizeof(SwitchHidCommand) == 0x27);




    struct SwitchHidCommandResponsePairing {
        u8 stage;
        union {
            struct {
                bluetooth::Address address;
                bluetooth::DeviceClass device_class;
                char name[0x14];
                u8 services;
            } stage1;

            struct {
                u8 link_key[0x10];
            } stage2;
        };
    } PACKED;

    struct SwitchHidCommandResponseLRButtonDetection {
        u16 l_button;
        u16 r_button;
        u16 zl_button;
        u16 zr_button;
        u16 sl_button;
        u16 sr_button;
        u16 home_button;
    } PACKED;

    struct SwitchHidCommandResponsePage {
        u8 status;
    } PACKED;

    struct SwitchHidCommandResponseGetDeviceInfo {
        u8 firmware_version_major;
        u8 firmware_version_minor;
        u8 device_type;
        u8 unk;
        bluetooth::Address address;
        u8 sensor_type;
        u8 format_version;
    } PACKED;

    struct SwitchHidCommandResponseShipment {
        u8 status;
    } PACKED;

    struct SwitchHidCommandResponseSerialFlashRead {
        u32 address;
        u8 size;
        u8 data[0x1D];
    } PACKED;

    struct SwitchHidCommandResponseSerialFlashWrite {
        u8 status;
    } PACKED;

    struct SwitchHidCommandResponseSerialFlashSectorErase {
        u8 status;
    } PACKED;

    struct SwitchHidCommandResponseMcuWrite {
        u8 data[0x21];
        u8 crc;
    } PACKED;

    struct SwitchHidCommandResponseMcuPollingEnable {
        u8 status;
    } PACKED;

    struct SwitchHidCommandResponseMcuPollingDisable {
        u8 status;
    } PACKED;

    struct SwitchHidCommandResponseAttachmentRead {
        u8 data[0x22];
    } PACKED;

    struct SwitchHidCommandResponseGetAttachmentInfo {
        u8 data[0x14];
    } PACKED;

    struct SwitchHidCommandResponseGetIndicatorLed {
        SwitchPlayerIndicatorType led_state;
    } PACKED;

    struct SwitchHidCommandResponseGetBatteryVoltage {
        u16 voltage;
    } PACKED;

    struct SwitchHidCommandResponsReadChargeSetting {
        u8 setting;
    } PACKED;

    struct SwitchHidCommandResponse {
        SwitchHidCommandId response_id;
        SwitchHidCommandId request_id;
        union {
            u8 data[0x22];
            SwitchHidCommandResponsePairing pairing;
            SwitchHidCommandResponseGetDeviceInfo get_device_info;
            SwitchHidCommandResponseLRButtonDetection lr_button_detection;
            SwitchHidCommandResponsePage page;
            SwitchHidCommandResponseShipment shipment;
            SwitchHidCommandResponseSerialFlashRead serial_flash_read;
            SwitchHidCommandResponseSerialFlashWrite serial_flash_write;
            SwitchHidCommandResponseSerialFlashSectorErase serial_flash_sector_erase;
            SwitchHidCommandResponseMcuWrite mcu_write;
            SwitchHidCommandResponseMcuPollingEnable mcu_polling_enable;
            SwitchHidCommandResponseMcuPollingDisable mcu_polling_disable;
            SwitchHidCommandResponseAttachmentRead attachment_read;
            SwitchHidCommandResponseGetAttachmentInfo get_attachment_info;
            SwitchHidCommandResponseGetIndicatorLed get_indicator_led;
            SwitchHidCommandResponseGetBatteryVoltage get_battery_voltage;
            SwitchHidCommandResponsReadChargeSetting read_charge_setting;
        };
    };
    static_assert(sizeof(SwitchHidCommandResponse) == 0x24);

}
