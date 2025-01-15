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
#include "btdrv_ext.h"
#include <string.h>

static const BtdrvAddress magic_address = {};

// WriteHidData2 IPC function has been hooked via exefs patches to implement custom commands when sent an all-zero bluetooth address
Result _btdrvCustomCommand(const void *data, size_t size) {
    return btdrvWriteHidData2(magic_address, data, size);
}

Result btdrvextGetHciHandle(BtdrvAddress address) {
    const struct {
        u32 type;
        BtdrvAddress address;
    } cmd = { BtdrvExtCustomEventType_GetHciHandle, address };

    return _btdrvCustomCommand(&cmd, sizeof(cmd));
}

Result btdrvextSendHciCommand(u16 opcode, const void *data, size_t size) {
    if (size > 255) {
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    }

    struct {
        u32 type;
        u16 opcode;
        u16 size;
        u8 data[255];
    } cmd = { BtdrvExtCustomEventType_SendHciCommand, opcode, size, {0} };
    memcpy(cmd.data, data, size);

    return _btdrvCustomCommand(&cmd, sizeof(cmd));
}

Result btdrvextDmSetConfig(const tBSA_DM_SET_CONFIG *set_config) {
    const struct NX_PACKED {
        u32 type;
        tBSA_DM_SET_CONFIG set_config;
    } cmd = { BtdrvExtCustomEventType_DmSetConfig, *set_config };

    return _btdrvCustomCommand(&cmd, sizeof(cmd));
}
