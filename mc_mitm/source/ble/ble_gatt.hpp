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
#include "ble_types.hpp"

namespace ams::ble::gatt {

    /*
     * The stack reports a connected device's attributes (ClientCacheSave events) when it discovers them, and
     * serves them from its own cache without reporting on later connections. Attribute tables are therefore kept
     * per slot across connections and only replaced when the stack reports a fresh discovery.
     */

    constexpr size_t MaxAttributes = 64;

    struct AttributeTable {
        bluetooth::Address address;
        u8 pad[2];
        u32 count;
        BtdrvGattAttribute attributes[MaxAttributes];
    };

    struct HidService {
        BtdrvGattAttribute service;
        BtdrvGattAttribute output_report;
        bool has_output_report;
    };

    // Event sink, see events::SetSink
    void OnEvent(const EventRecord &record);

    // Waits for the stack to report the device's attributes for the given slot, reusing the table held for it if none arrive
    void Discover(u32 slot_index, u32 conn_id, const bluetooth::Address &address);

    // Set when a cache report arrived after discovery completed, i.e. the stack re-discovered the services
    bool TakeRediscovered(u32 slot_index);

    // Enables notifications on the HID input Report characteristic(s). Fails with InsufficientAuthentication
    // when the link isn't encrypted; returns NotFound if the table holds no HID service.
    Result Subscribe(u32 slot_index, u32 conn_id, HidService *out);

    // Resets slot connection state while retaining cached attributes for the address
    void ResetSlot(u32 slot_index);

}
