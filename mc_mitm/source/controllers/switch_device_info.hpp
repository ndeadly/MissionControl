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

namespace ams::controller {

    struct FirmwareVersion {
        u8 major;
        u8 minor;
    };

    struct SwitchDeviceInfo {
        FirmwareVersion bt_firmware;
        FirmwareVersion mcu_firmware;
        u8 device_type;
        const char *device_name;
        u16 vendor_id;
        u16 product_id;
        u8 unk;
        u8 sensor_type;
        u8 format_version;
    };

    struct SwitchDevices {
        static constexpr SwitchDeviceInfo JoyConLeft = {
            .bt_firmware    = { 0x04, 0x19 },
            .device_type    = 0x01,
            .device_name    = "JoyCon (L)",
            .vendor_id      = 0x057E,
            .product_id     = 0x2006,
            .format_version = 0x01
        };

        static constexpr SwitchDeviceInfo JoyConRight = {
            .bt_firmware    = { 0x04, 0x19 },
            .mcu_firmware   = { 0x08, 0x1B },
            .device_type    = 0x02,
            .device_name    = "JoyCon (R)",
            .vendor_id      = 0x057E,
            .product_id     = 0x2007,
            .format_version = 0x01
        };

        static constexpr SwitchDeviceInfo ProController = {
            .bt_firmware    = { 0x04, 0x21 },
            .mcu_firmware   = { 0x03, 0x05 },
            .device_type    = 0x06, //0x03,
            .device_name    = "Lic Pro Controller", //"Pro Controller",
            .vendor_id      = 0x057E,
            .product_id     = 0x2009,
            .unk            = 0x02,
            .sensor_type    = 0x01,
            .format_version = 0x02
        };

        static constexpr SwitchDeviceInfo FamiconControllerLeft = {
            .bt_firmware    = { 0x03, 0x90 },
            .device_type    = 0x07,
            .device_name    = "HVC Controller (1)",
            .vendor_id      = 0x057E,
            .product_id     = 0x2007,
            .unk            = 0x02,
            .format_version = 0x02
        };

        static constexpr SwitchDeviceInfo FamiconControllerRight = {
            .bt_firmware    = { 0x03, 0x90 },
            .device_type    = 0x08,
            .device_name    = "HVC Controller (2)",
            .vendor_id      = 0x057E,
            .product_id     = 0x2007,
            .unk            = 0x02,
            .format_version = 0x02
        };

        static constexpr SwitchDeviceInfo SnesController = {
            .bt_firmware    = { 0x03, 0xB8 },
            .device_type    = 0x0B,
            .device_name    = "SNES Controller",
            .vendor_id      = 0x057E,
            .product_id     = 0x2017,
            .unk            = 0x02,
            .format_version = 0x02
        };

        static constexpr SwitchDeviceInfo N64Controller = {
            .bt_firmware    = { 0x04, 0x11 },
            .device_type    = 0x0C,
            .device_name    = "N64 Controller",
            .vendor_id      = 0x057E,
            .product_id     = 0x2019,
            .unk            = 0x02,
            .format_version = 0x02
        };

        static constexpr SwitchDeviceInfo SegaGenesisController = {
            .bt_firmware    = { 0x04, 0x14 },
            .device_type    = 0x0D,
            .device_name    = "MD/Gen Control Pad",
            .vendor_id      = 0x057E,
            .product_id     = 0x201A,
            .unk            = 0x02,
            .format_version = 0x02
        };
    };

}
