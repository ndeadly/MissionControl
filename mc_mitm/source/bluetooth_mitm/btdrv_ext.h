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
#include <switch.h>
#include "bsa_defs.h"

#define BtdrvEventType_MissionControlCustomEvent 0x69

typedef enum {
    BtdrvExtCustomEventType_GetHciHandle   = 0x0,
    BtdrvExtCustomEventType_SendHciCommand = 0x1,
    BtdrvExtCustomEventType_DmSetConfig    = 0x2,
} BtdrvExtCustomEventType;

typedef struct {
    u32 type;
    union {
        u8 data[sizeof(BtdrvEventInfo) - sizeof(u32)];

        struct {
            BtdrvAddress address;
            u16 status;
            u16 handle;
        } get_hci_handle;

        struct {
            u16 opcode;
            u16 status;
            u16 size;
            u8 data[255];
        } hci_command_response;
    };
} BtdrvExtCustomEventInfo;

#ifdef __cplusplus
extern "C" {
#endif

Result btdrvextGetHciHandle(BtdrvAddress address);
Result btdrvextSendHciCommand(u16 opcode, const void *data, size_t size);
Result btdrvextDmSetConfig(const tBSA_DM_SET_CONFIG *add_dev);

#ifdef __cplusplus
}
#endif
