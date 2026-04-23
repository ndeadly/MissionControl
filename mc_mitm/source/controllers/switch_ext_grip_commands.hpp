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

    enum class SwitchExtGripCommandId : u8 {
        Inquiry             = 0x01,
        CreateHidConnection = 0x02,
        DeleteHidConnection = 0x03,
        StartHidData        = 0x04,
        StopHidData         = 0x05,
        Command0x06         = 0x06,
        GetFirmwareVersion  = 0x07,
        Command0x08         = 0x08,
        LoopbackTest        = 0x91,
        Command0x92         = 0x92,
        SetUartClockTrim    = 0xA0,
    };

    struct SwitchExtGripCommandResponseInquiry {
        u8 device_type;
        bluetooth::Address address;
    } PACKED;

    struct SwitchExtGripCommandResponseGetFirmwareVersion {
        u8 version[4];
    } PACKED;

    struct SwitchExtGripCommandResponseCommand0x08 {
        u8 unk;
    } PACKED;

    struct SwitchExtGripCommand {
        SwitchExtGripCommandId id;
        union {
            u8 data[0x3E];
        };
    };
    static_assert(sizeof(SwitchExtGripCommand) == 0x3F);

    struct SwitchExtGripCommandResponse {
        SwitchExtGripCommandId command_id;
        u8 unk;
        union {
            u8 data[0x3D];
            SwitchExtGripCommandResponseInquiry inquiry;
            SwitchExtGripCommandResponseGetFirmwareVersion get_firmware_version;
            SwitchExtGripCommandResponseCommand0x08 command0x08;
        };
    };
    static_assert(sizeof(SwitchExtGripCommandResponse) == 0x3F);

}
