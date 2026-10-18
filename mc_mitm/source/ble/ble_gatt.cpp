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
#include "ble_gatt.hpp"
#include "ble_results.hpp"
#include "ble_events.hpp"
#include "ble_log.hpp"

namespace ams::ble::gatt {

    namespace {

        struct SlotGattState {
            AttributeTable table = {};
            u32 conn_id = InvalidConnId;
            bool replace_pending = false;
            bool discovery_complete = false;
            bool rediscovered = false;
        };

        constinit SlotGattState g_slots[MaxBleControllers] = {};

        const BtdrvGattAttribute *FindService(u32 slot_index, u16 uuid16) {
            if (slot_index >= MaxBleControllers) {
                return nullptr;
            }
            auto &tbl = g_slots[slot_index].table;
            for (size_t i = 0; i < tbl.count; ++i) {
                auto &attr = tbl.attributes[i];
                if ((attr.type == BtdrvGattAttributeType_Service) && IsUuid16(attr.id.uuid, uuid16)) {
                    return &attr;
                }
            }
            return nullptr;
        }

        bool IsWithinService(const BtdrvGattAttribute &attr, const BtdrvGattAttribute &service) {
            return (attr.handle > service.handle) && (attr.handle <= service.group_end_handle);
        }

        // Descriptors belong to the characteristic that precedes them by handle
        const BtdrvGattAttribute *OwningCharacteristic(u32 slot_index, const BtdrvGattAttribute &desc) {
            if (slot_index >= MaxBleControllers) {
                return nullptr;
            }
            auto &tbl = g_slots[slot_index].table;
            const BtdrvGattAttribute *best = nullptr;
            for (size_t i = 0; i < tbl.count; ++i) {
                auto &attr = tbl.attributes[i];
                if ((attr.type == BtdrvGattAttributeType_Characteristic) && (attr.handle < desc.handle)) {
                    if (!best || (attr.handle > best->handle)) {
                        best = &attr;
                    }
                }
            }
            return best;
        }

        const BtdrvGattAttribute *FindDescriptor(u32 slot_index, const BtdrvGattAttribute &characteristic, u16 uuid16) {
            if (slot_index >= MaxBleControllers) {
                return nullptr;
            }
            auto &tbl = g_slots[slot_index].table;
            for (size_t i = 0; i < tbl.count; ++i) {
                auto &desc = tbl.attributes[i];
                if ((desc.type == BtdrvGattAttributeType_Descriptor) && IsUuid16(desc.id.uuid, uuid16) && (OwningCharacteristic(slot_index, desc) == &characteristic)) {
                    return &desc;
                }
            }
            return nullptr;
        }

    }

    void OnEvent(const EventRecord &e) {
        if (e.type != BtdrvBleEventType_ClientCacheSave) {
            return;
        }

        const u32 conn_id = e.info.client_cache_save.conn_id;
        u32 slot = InvalidSlot;
        for (size_t s = 0; s < MaxBleControllers; ++s) {
            if (g_slots[s].conn_id == conn_id) {
                slot = static_cast<u32>(s);
                break;
            }
        }
        if (slot == InvalidSlot) {
            return;
        }

        auto &s = g_slots[slot];
        if (s.discovery_complete) {
            s.rediscovered = true;
        }

        if (s.replace_pending) {
            s.replace_pending = false;
            s.table.count = 0;
        }

        for (u32 i = 0; i < std::min<u32>(e.info.client_cache_save.count, 10); ++i) {
            if (s.table.count < MaxAttributes) {
                s.table.attributes[s.table.count++] = e.info.client_cache_save.attr_list[i];
            }
        }
    }

    void Discover(u32 slot_index, u32 conn_id, const bluetooth::Address &address) {
        if (slot_index >= MaxBleControllers) {
            return;
        }

        auto &s = g_slots[slot_index];
        s.conn_id = conn_id;
        s.discovery_complete = false;
        s.rediscovered = false;
        s.replace_pending = true;

        const bool reuse = (s.table.count > 0) && AddressEqual(s.table.address, address);
        if (!reuse) {
            s.table.count = 0;
        }
        s.table.address = address;

        // The stack runs discovery on its own shortly after connecting to a device it hasn't seen
        events::Pump(TimeSpan::FromSeconds(2));

        Result rc = btdrvGetGattAttribute(address, conn_id);
        log::Write("btdrvGetGattAttribute slot=%u rc=0x%x", slot_index, rc.GetValue());

        rc = btdrvConfigureAttMtu(conn_id, 0x100);
        log::Write("btdrvConfigureAttMtu slot=%u rc=0x%x", slot_index, rc.GetValue());

        // The MTU result is the last thing the stack reports for a fresh link
        events::WaitFor(BtdrvBleEventType_ClientConfigureMtu, TimeSpan::FromSeconds(5), nullptr, [&](const EventRecord &r) { return r.info.client_configure_mtu.conn_id == conn_id; });
        events::Pump(TimeSpan::FromMilliSeconds(500));

        s.discovery_complete = true;
        s.replace_pending = false;
        log::Write("slot %u attribute table holds %u entries%s", slot_index, s.table.count, reuse ? " (kept from previous connection)" : "");
    }

    bool TakeRediscovered(u32 slot_index) {
        if (slot_index >= MaxBleControllers) {
            return false;
        }
        const bool rediscovered = g_slots[slot_index].rediscovered;
        g_slots[slot_index].rediscovered = false;
        return rediscovered;
    }

    Result Subscribe(u32 slot_index, u32 conn_id, HidService *out) {
        if (slot_index >= MaxBleControllers) {
            R_THROW(svc::ResultInvalidArgument());
        }

        auto found = FindService(slot_index, GattServiceHid);
        if (!found) {
            log::Write("slot %u: no HID service in attribute table", slot_index);
            R_THROW(svc::ResultNotFound());
        }

        auto &s = g_slots[slot_index];
        const BtdrvGattAttribute service = *found;
        const size_t count = s.table.count;

        out->service = service;
        out->has_output_report = false;

        // Authenticated reads are what make the stack encrypt the link (pairing first if it has to)
        for (size_t i = 0; i < count; ++i) {
            auto &attr = s.table.attributes[i];
            if ((attr.type != BtdrvGattAttributeType_Characteristic) || !IsWithinService(attr, service)) {
                continue;
            }

            // The stack drops the link itself when encryption fails
            R_UNLESS(!events::IsSlotDisconnected(slot_index), ResultNoConnection());

            if (IsUuid16(attr.id.uuid, GattCharHidInformation) || IsUuid16(attr.id.uuid, GattCharReportMap)) {
                const u32 results = events::GetGattResultCount(slot_index);
                Result rc;
                for (int attempt = 0; ; ++attempt) {
                    rc = btdrvReadGattCharacteristic(conn_id, service.is_primary, &service.id, &attr.id, BtdrvGattAuthReqType_NoMitm);
                    log::Write("btdrvReadGattCharacteristic slot=%u handle=0x%04x rc=0x%x", slot_index, attr.handle, rc.GetValue());
                    // Refused locally while the pad's connection parameter update is in flight; it clears within a second or two
                    if (!ResultInsufficientAuthentication::Includes(rc) || (attempt >= 4) || events::IsSlotDisconnected(slot_index)) {
                        break;
                    }
                    events::Pump(TimeSpan::FromSeconds(1));
                }
                R_TRY(rc);
                // The first authenticated read may carry a whole Just Works pairing before it completes
                if (!events::WaitForGattResult(slot_index, results, TimeSpan::FromSeconds(10))) {
                    events::Pump(TimeSpan::FromMilliSeconds(500));
                }
            }
        }

        Result result = svc::ResultNotFound();
        for (size_t i = 0; i < count; ++i) {
            auto &attr = s.table.attributes[i];
            if ((attr.type != BtdrvGattAttributeType_Characteristic) || !IsWithinService(attr, service) || !IsUuid16(attr.id.uuid, GattCharReport)) {
                continue;
            }

            R_UNLESS(!events::IsSlotDisconnected(slot_index), ResultNoConnection());

            // Rumble goes to the writable Report
            if (attr.property & (BtdrvGattCharacteristicProperty_Write | BtdrvGattCharacteristicProperty_WriteNoResponse)) {
                out->output_report = attr;
                out->has_output_report = true;
            }

            if (!(attr.property & BtdrvGattCharacteristicProperty_Notify)) {
                continue;
            }

            auto cccd = FindDescriptor(slot_index, attr, GattDescClientCharConfig);
            if (!cccd) {
                continue;
            }

            Result rc = btdrvRegisterGattNotification(conn_id, service.is_primary, &service.id, &attr.id);
            log::Write("btdrvRegisterGattNotification slot=%u handle=0x%04x rc=0x%x", slot_index, attr.handle, rc.GetValue());

            // The write's asynchronous result is the subscription's: an error means the link never encrypted
            const u32 results = events::GetGattResultCount(slot_index);
            const u8 enable_notifications[2] = { 0x01, 0x00 };
            rc = btdrvWriteGattDescriptor(conn_id, service.is_primary, &service.id, &attr.id, &cccd->id, enable_notifications, sizeof(enable_notifications), BtdrvGattAuthReqType_NoMitm);
            log::Write("btdrvWriteGattDescriptor(CCCD) slot=%u handle=0x%04x rc=0x%x", slot_index, cccd->handle, rc.GetValue());
            R_TRY(rc);
            R_UNLESS(events::WaitForGattResult(slot_index, results, TimeSpan::FromSeconds(2)), svc::ResultTimedOut());
            R_UNLESS(!events::IsSlotDisconnected(slot_index), ResultNoConnection());
            R_UNLESS(events::GetGattFailure(slot_index) == 0, ResultInsufficientAuthentication());
            result = ResultSuccess();
        }

        R_RETURN(result);
    }

    void ResetSlot(u32 slot_index) {
        if (slot_index < MaxBleControllers) {
            g_slots[slot_index].conn_id = InvalidConnId;
            g_slots[slot_index].replace_pending = false;
            g_slots[slot_index].discovery_complete = false;
            g_slots[slot_index].rediscovered = false;
        }
    }

}
