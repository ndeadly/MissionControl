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

    enum class SwitchMcuCommandId : u8 {
        Invalid          = 0x00,
        StateReport      = 0x01,
        IrData           = 0x03,
        BusyInitializing = 0x0B,
        IrStatus         = 0x13,
        IrRegisters      = 0x1B,
        ConfigureMcu     = 0x21,
        ConfigureIr      = 0x23,
        NfcState         = 0x2A,
        NfcReadData      = 0x3A,
        EmptyAwaitingCmd = 0xFF,
    };

    enum class SwitchMcuSubCommandId : u8 {
        SetMcuMode           = 0x00,
        GetMcuMode           = 0x01,
        ReadDeviceMode       = 0x02,
        WriteDeviceRegisters = 0x04,
    };

    enum class SwitchMcuMode : u8 {
        Suspended = 0,
        Standby   = 1,
        Ringcon   = 3,
        Nfc       = 4,
        Ir        = 5,
        Busy      = 6,
    };

    struct SwitchMcuCommand {
        SwitchMcuSubCommandId sub_command;
        union {
            u8 raw[0x26];

            struct {
                SwitchMcuMode mode;
            } set_mcu_mode;
        } data;
    } PACKED;

    struct SwitchMcuResponse {
        SwitchMcuCommandId command;
        union {
            u8 raw[0x137];

            struct {
                u8 pad[3];
                u8 unknown_1;
                u8 pad2;
                u8 unknown_2;
                SwitchMcuMode mode;
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

}
