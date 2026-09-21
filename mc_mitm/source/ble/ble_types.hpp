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
#include <switch.h>
#include <stratosphere.hpp>
#include "../bluetooth_mitm/bluetooth/bluetooth_types.hpp"

namespace ams::ble {

    constexpr u32 InvalidConnId   = 0xFFFFFFFF;
    constexpr u8  InvalidClientIf = 0xFF;

    /* BtdrvBleEventInfo field values not named by libnx */
    constexpr u8 BleConnectionStatus_Connected    = 0;
    constexpr u8 BleConnectionStatus_Disconnected = 2;
    constexpr u8 BleScanStatus_NewDevice          = 2;
    constexpr u8 GattNotifyType_Notification      = 4;

    /* Bluetooth SIG assigned numbers for HID over GATT (HOGP) */
    constexpr u16 GattServiceHid           = 0x1812;
    constexpr u16 GattCharHidInformation   = 0x2A4A;
    constexpr u16 GattCharReportMap        = 0x2A4B;
    constexpr u16 GattCharReport           = 0x2A4D;
    constexpr u16 GattCharProtocolMode     = 0x2A4E;
    constexpr u16 GattDescClientCharConfig = 0x2902;
    constexpr u16 GattDescReportReference  = 0x2908;
    constexpr u16 AppearanceGamepad        = 0x03C4;

    /* Bluetooth base UUID 00000000-0000-1000-8000-00805F9B34FB, little-endian */
    constexpr u8 BluetoothBaseUuidLE[0x10] = { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    struct EventRecord {
        bluetooth::BleEventType type;
        bool forwarded;             // the copy btm receives was not withheld
        bluetooth::BleEventInfo info;
    };

    inline bool AddressEqual(const bluetooth::Address &a, const bluetooth::Address &b) {
        return std::memcmp(&a, &b, sizeof(bluetooth::Address)) == 0;
    }

    constexpr BtdrvGattAttributeUuid MakeUuid16(u16 value) {
        BtdrvGattAttributeUuid uuid = {};
        uuid.size = 2;
        uuid.uuid[0] = value & 0xff;
        uuid.uuid[1] = value >> 8;
        return uuid;
    }

    // Accepts a 16-bit UUID in either byte order, or expanded to 128 bits on the Bluetooth base UUID
    inline bool IsUuid16(const BtdrvGattAttributeUuid &uuid, u16 value) {
        const u8 lo = value & 0xff;
        const u8 hi = value >> 8;

        if (uuid.size == 2) {
            return ((uuid.uuid[0] == lo) && (uuid.uuid[1] == hi)) || ((uuid.uuid[0] == hi) && (uuid.uuid[1] == lo));
        }

        if (uuid.size == 0x10) {
            if ((std::memcmp(uuid.uuid, BluetoothBaseUuidLE, 12) == 0) && (uuid.uuid[12] == lo) && (uuid.uuid[13] == hi) && (uuid.uuid[14] == 0) && (uuid.uuid[15] == 0)) {
                return true;
            }
            if ((uuid.uuid[0] == 0) && (uuid.uuid[1] == 0) && (uuid.uuid[2] == hi) && (uuid.uuid[3] == lo)) {
                for (int i = 0; i < 12; ++i) {
                    if (uuid.uuid[4 + i] != BluetoothBaseUuidLE[11 - i]) {
                        return false;
                    }
                }
                return true;
            }
        }

        return false;
    }

    inline void FormatUuid(const BtdrvGattAttributeUuid &uuid, char *out, size_t size) {
        size_t n = 0;
        size_t count = std::min<size_t>(uuid.size, sizeof(uuid.uuid));
        for (size_t i = 0; i < count && n + 3 < size; ++i) {
            n += std::snprintf(out + n, size - n, "%02x", uuid.uuid[i]);
        }
        if (n < size) {
            out[n] = '\0';
        }
    }

}
