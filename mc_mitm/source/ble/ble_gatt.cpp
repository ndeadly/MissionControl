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
                if ((desc.type == BtdrvGattAttributeType_Descriptor) && IsUuid16(desc.id.uuid, uuid16)) {
                    const auto *owner = OwningCharacteristic(slot_index, desc);
                    if (owner && owner->handle == characteristic.handle) return &desc;
                }
            }
            return nullptr;
        }

        bool HasHidInputReport(u32 slot_index) {
            const auto *service = FindService(slot_index, GattServiceHid);
            if (!service) return false;
            const auto &table = g_slots[slot_index].table;
            for (size_t i = 0; i < table.count; ++i) {
                const auto &attr = table.attributes[i];
                if ((attr.type == BtdrvGattAttributeType_Characteristic) && IsWithinService(attr, *service) &&
                    IsUuid16(attr.id.uuid, GattCharReport) && (attr.property & BtdrvGattCharacteristicProperty_Notify) &&
                    FindDescriptor(slot_index, attr, GattDescClientCharConfig)) return true;
            }
            return false;
        }

    }

    void OnEvent(const EventRecord &e) {
        if (e.type != BtdrvBleEventType_ClientCacheSave) {
            return;
        }

        if (e.info.client_cache_save.result != 0) return;
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
            const auto &attr = e.info.client_cache_save.attr_list[i];
            size_t index = 0;
            while ((index < s.table.count) && (s.table.attributes[index].handle != attr.handle)) ++index;
            if (index < MaxAttributes) {
                s.table.attributes[index] = attr;
                if (index == s.table.count) ++s.table.count;
            }
        }
    }

    Result Discover(u32 slot_index, u32 conn_id, const bluetooth::Address &address) {
        R_UNLESS(slot_index < MaxBleControllers, svc::ResultInvalidArgument());
        auto &s = g_slots[slot_index];
        // Reconnect order need not match the previous slot order.
        if (!AddressEqual(s.table.address, address)) {
            s.table = {};
            for (const auto &cached : g_slots) {
                if (AddressEqual(cached.table.address, address)) {
                    s.table = cached.table;
                    break;
                }
            }
        }
        s.table.address = address;
        s.conn_id = conn_id;
        s.discovery_complete = false;
        s.rediscovered = false;
        s.replace_pending = true;

        const auto link_lost = [&] {
            return events::IsSlotDisconnected(slot_index) || !events::IsRadioEnabled();
        };
        const auto disconnected = [&](const EventRecord &) { return link_lost(); };

        // Preserve the probe's connection setup sequence. An accepted cache
        // query immediately after connection need not emit any attributes.
        // Do not gate CreateBond on an attribute table that may arrive later.
        events::WaitFor(events::InvalidType, TimeSpan::FromSeconds(2), nullptr, disconnected);
        R_UNLESS(!link_lost(), ResultNoConnection());

        Result rc = btdrvGetGattAttribute(address, conn_id);
        log::Write("btdrvGetGattAttribute slot=%u rc=0x%x", slot_index, rc.GetValue());

        rc = btdrvConfigureAttMtu(conn_id, 0x100);
        log::Write("btdrvConfigureAttMtu slot=%u rc=0x%x", slot_index, rc.GetValue());
        if (R_SUCCEEDED(rc)) {
            const bool completed = events::WaitFor(events::InvalidType, TimeSpan::FromSeconds(5), nullptr, [&](const EventRecord &r) {
                return disconnected(r) || ((r.type == BtdrvBleEventType_ClientConfigureMtu) &&
                    (r.info.client_configure_mtu.conn_id == conn_id));
            });
            R_UNLESS(!link_lost(), ResultNoConnection());
            // A larger MTU is optional; a missing/refused response does not
            // prove that pairing failed. Input still fits the default MTU.
            if (!completed) log::Write("slot %u: MTU completion timed out, continuing to pairing", slot_index);
        }
        events::WaitFor(events::InvalidType, TimeSpan::FromMilliSeconds(500), nullptr, disconnected);
        R_UNLESS(!link_lost(), ResultNoConnection());
        log::Write("slot %u: GATT setup finished, attributes=%u hid_input=%u; pairing next",
            slot_index, s.table.count, HasHidInputReport(slot_index));
        R_SUCCEED();
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

        R_UNLESS(!events::IsSlotDisconnected(slot_index) && events::IsRadioEnabled(), ResultNoConnection());
        if (!HasHidInputReport(slot_index)) {
            // Called after CreateBond. Retry the cache query now that link
            // setup/security has progressed, instead of disconnecting beforehand.
            Result rc = btdrvGetGattAttribute(g_slots[slot_index].table.address, conn_id);
            log::Write("btdrvGetGattAttribute after pairing slot=%u rc=0x%x", slot_index, rc.GetValue());
            R_TRY(rc);
            // Cache tables arrive in chunks. Give the query's remaining chunks
            // the same settling window as initial discovery before snapshotting.
            events::WaitFor(events::InvalidType, TimeSpan::FromMilliSeconds(500), nullptr, [&](const EventRecord &) {
                return events::IsSlotDisconnected(slot_index) || !events::IsRadioEnabled();
            });
            const bool ready = events::WaitFor(events::InvalidType, TimeSpan::FromSeconds(5), nullptr, [&](const EventRecord &) {
                return events::IsSlotDisconnected(slot_index) || !events::IsRadioEnabled() || HasHidInputReport(slot_index);
            });
            R_UNLESS(!events::IsSlotDisconnected(slot_index) && events::IsRadioEnabled(), ResultNoConnection());
            if (!ready) log::Write("slot %u: HID attributes unavailable after pairing (attributes=%u)", slot_index, g_slots[slot_index].table.count);
            R_UNLESS(ready, svc::ResultTimedOut());
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
            const auto attr = s.table.attributes[i];
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
                R_UNLESS(events::WaitForGattResult(slot_index, results, TimeSpan::FromSeconds(10)), svc::ResultTimedOut());
                R_UNLESS(events::GetGattFailure(slot_index) == 0, ResultInsufficientAuthentication());
            }
        }

        Result result = svc::ResultNotFound();
        for (size_t i = 0; i < count; ++i) {
            const auto attr = s.table.attributes[i];
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

            R_TRY(rc);

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

        s.discovery_complete = R_SUCCEEDED(result);
        R_RETURN(result);
    }

    void Unsubscribe(u32 slot_index, u32 conn_id) {
        const auto *service = FindService(slot_index, GattServiceHid);
        if (!service) return;
        const auto &table = g_slots[slot_index].table;
        for (size_t i = 0; i < table.count; ++i) {
            const auto &attr = table.attributes[i];
            if ((attr.type == BtdrvGattAttributeType_Characteristic) && IsWithinService(attr, *service) &&
                IsUuid16(attr.id.uuid, GattCharReport) && (attr.property & BtdrvGattCharacteristicProperty_Notify)) {
                static_cast<void>(btdrvUnregisterGattNotification(conn_id, service->is_primary, &service->id, &attr.id));
            }
        }
    }

    bool HasCachedAddress(u32 slot_index, const bluetooth::Address &address) {
        return slot_index < MaxBleControllers && g_slots[slot_index].table.count > 0 &&
            AddressEqual(g_slots[slot_index].table.address, address);
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
