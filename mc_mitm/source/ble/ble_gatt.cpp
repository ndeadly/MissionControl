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

        constinit AttributeTable g_table = {};

        // The first cache report after a connection replaces the table; reports after discovery has completed
        // mean the stack re-discovered the services
        constinit bool g_replace_pending = false;
        constinit bool g_discovery_complete = false;
        constinit bool g_rediscovered = false;

        const BtdrvGattAttribute *FindService(u16 uuid16) {
            for (size_t i = 0; i < g_table.count; ++i) {
                auto &attr = g_table.attributes[i];
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
        const BtdrvGattAttribute *OwningCharacteristic(const BtdrvGattAttribute &desc) {
            const BtdrvGattAttribute *best = nullptr;
            for (size_t i = 0; i < g_table.count; ++i) {
                auto &attr = g_table.attributes[i];
                if ((attr.type == BtdrvGattAttributeType_Characteristic) && (attr.handle < desc.handle)) {
                    if (!best || (attr.handle > best->handle)) {
                        best = &attr;
                    }
                }
            }
            return best;
        }

        const BtdrvGattAttribute *FindDescriptor(const BtdrvGattAttribute &characteristic, u16 uuid16) {
            for (size_t i = 0; i < g_table.count; ++i) {
                auto &desc = g_table.attributes[i];
                if ((desc.type == BtdrvGattAttributeType_Descriptor) && IsUuid16(desc.id.uuid, uuid16) && (OwningCharacteristic(desc) == &characteristic)) {
                    return &desc;
                }
            }
            return nullptr;
        }

    }

    void OnEvent(const EventRecord &e) {
        if ((e.type != BtdrvBleEventType_ClientCacheSave) || (e.info.client_cache_save.conn_id != events::GetConnection())) {
            return;
        }

        if (g_discovery_complete) {
            g_rediscovered = true;
        }

        if (g_replace_pending) {
            g_replace_pending = false;
            g_table.count = 0;
        }

        for (u32 i = 0; i < std::min<u32>(e.info.client_cache_save.count, 10); ++i) {
            if (g_table.count < MaxAttributes) {
                g_table.attributes[g_table.count++] = e.info.client_cache_save.attr_list[i];
            }
        }
    }

    void Discover(u32 conn_id, const bluetooth::Address &address) {
        g_discovery_complete = false;
        g_rediscovered = false;
        g_replace_pending = true;

        const bool reuse = (g_table.count > 0) && AddressEqual(g_table.address, address);
        if (!reuse) {
            g_table.count = 0;
        }
        g_table.address = address;

        // The stack runs discovery on its own shortly after connecting to a device it hasn't seen
        events::Pump(TimeSpan::FromSeconds(2));

        Result rc = btdrvGetGattAttribute(address, conn_id);
        log::Write("btdrvGetGattAttribute rc=0x%x", rc.GetValue());

        rc = btdrvConfigureAttMtu(conn_id, 0x100);
        log::Write("btdrvConfigureAttMtu rc=0x%x", rc.GetValue());

        // The MTU result is the last thing the stack reports for a fresh link
        events::WaitFor(BtdrvBleEventType_ClientConfigureMtu, TimeSpan::FromSeconds(5), nullptr, [&](const EventRecord &r) { return r.info.client_configure_mtu.conn_id == conn_id; });
        events::Pump(TimeSpan::FromMilliSeconds(500));

        g_discovery_complete = true;
        g_replace_pending = false;
        log::Write("attribute table holds %u entries%s", g_table.count, reuse ? " (kept from previous connection)" : "");
    }

    bool TakeRediscovered() {
        const bool rediscovered = g_rediscovered;
        g_rediscovered = false;
        return rediscovered;
    }

    Result Subscribe(u32 conn_id, HidService *out) {
        auto found = FindService(GattServiceHid);
        if (!found) {
            log::Write("no HID service in attribute table");
            R_THROW(svc::ResultNotFound());
        }

        // The stack can report a re-discovery while this runs, appending to the table; work from a copy of the
        // service and only over the entries that were there to begin with
        const BtdrvGattAttribute service = *found;
        const size_t count = g_table.count;

        out->service = service;
        out->has_output_report = false;

        // Authenticated reads are what make the stack encrypt the link (pairing first if it has to)
        for (size_t i = 0; i < count; ++i) {
            auto &attr = g_table.attributes[i];
            if ((attr.type != BtdrvGattAttributeType_Characteristic) || !IsWithinService(attr, service)) {
                continue;
            }

            // The stack drops the link itself when encryption fails
            R_UNLESS(!events::IsDisconnected(), ResultNoConnection());

            if (IsUuid16(attr.id.uuid, GattCharHidInformation) || IsUuid16(attr.id.uuid, GattCharReportMap)) {
                const u32 results = events::GetGattResultCount();
                Result rc;
                for (int attempt = 0; ; ++attempt) {
                    rc = btdrvReadGattCharacteristic(conn_id, service.is_primary, &service.id, &attr.id, BtdrvGattAuthReqType_NoMitm);
                    log::Write("btdrvReadGattCharacteristic handle=0x%04x rc=0x%x", attr.handle, rc.GetValue());
                    // Refused locally while the pad's connection parameter update is in flight; it clears within a second or two
                    if (!ResultInsufficientAuthentication::Includes(rc) || (attempt >= 4) || events::IsDisconnected()) {
                        break;
                    }
                    events::Pump(TimeSpan::FromSeconds(1));
                }
                R_TRY(rc);
                // The first authenticated read may carry a whole Just Works pairing before it completes
                if (!events::WaitForGattResult(results, TimeSpan::FromSeconds(10))) {
                    events::Pump(TimeSpan::FromMilliSeconds(500));
                }
            }
        }

        Result result = svc::ResultNotFound();
        for (size_t i = 0; i < count; ++i) {
            auto &attr = g_table.attributes[i];
            if ((attr.type != BtdrvGattAttributeType_Characteristic) || !IsWithinService(attr, service) || !IsUuid16(attr.id.uuid, GattCharReport)) {
                continue;
            }

            R_UNLESS(!events::IsDisconnected(), ResultNoConnection());

            // Rumble goes to the writable Report
            if (attr.property & (BtdrvGattCharacteristicProperty_Write | BtdrvGattCharacteristicProperty_WriteNoResponse)) {
                out->output_report = attr;
                out->has_output_report = true;
            }

            if (!(attr.property & BtdrvGattCharacteristicProperty_Notify)) {
                continue;
            }

            auto cccd = FindDescriptor(attr, GattDescClientCharConfig);
            if (!cccd) {
                continue;
            }

            Result rc = btdrvRegisterGattNotification(conn_id, service.is_primary, &service.id, &attr.id);
            log::Write("btdrvRegisterGattNotification handle=0x%04x rc=0x%x", attr.handle, rc.GetValue());

            // The write's asynchronous result is the subscription's: an error means the link never encrypted
            const u32 results = events::GetGattResultCount();
            const u8 enable_notifications[2] = { 0x01, 0x00 };
            rc = btdrvWriteGattDescriptor(conn_id, service.is_primary, &service.id, &attr.id, &cccd->id, enable_notifications, sizeof(enable_notifications), BtdrvGattAuthReqType_NoMitm);
            log::Write("btdrvWriteGattDescriptor(CCCD) handle=0x%04x rc=0x%x", cccd->handle, rc.GetValue());
            R_TRY(rc);
            R_UNLESS(events::WaitForGattResult(results, TimeSpan::FromSeconds(2)), svc::ResultTimedOut());
            R_UNLESS(!events::IsDisconnected(), ResultNoConnection());
            R_UNLESS(events::GetGattFailure() == 0, ResultInsufficientAuthentication());
            result = ResultSuccess();
        }

        R_RETURN(result);
    }

}
