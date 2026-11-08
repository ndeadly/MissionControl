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
#include "ble_pairing.hpp"
#include "ble_events.hpp"
#include "ble_results.hpp"
#include "ble_log.hpp"

namespace ams::ble::pairing {
    // BTA convention: 1 = BR/EDR, 2 = LE. The native driver may ignore it.
    constexpr u32 BondTransport_Le = 2;

    static Result WaitForConnection(u32 slot, u8 client_if, const bluetooth::Address &address, u32 *out_conn_id, TimeSpan timeout) {
        EventRecord e;
        if (!events::WaitFor(events::InvalidType, timeout, &e, [&](const EventRecord &r) {
            return !events::IsRadioEnabled() || events::IsSlotDisconnected(slot) ||
                (r.type == BtdrvBleEventType_ClientConnection && AddressEqual(r.info.client_connection.address, address) && r.info.client_connection.client_if == client_if);
        })) {
            R_THROW(svc::ResultTimedOut());
        }

        R_UNLESS(events::IsRadioEnabled() && e.type == BtdrvBleEventType_ClientConnection, ResultNoConnection());
        if ((e.info.client_connection.result != 0) || (e.info.client_connection.status != BleConnectionStatus_Connected)) {
            log::Write("slot %u: connection failed result=%u status=%u", slot, e.info.client_connection.result, e.info.client_connection.status);
            R_THROW(ResultNoConnection());
        }

        R_UNLESS(e.info.client_connection.conn_id != InvalidConnId, ResultNoConnection());
        *out_conn_id = e.info.client_connection.conn_id;
        R_UNLESS(!events::IsSlotDisconnected(slot), ResultNoConnection());
        log::Write("slot %u: connected conn_id=%u", slot, *out_conn_id);
        R_SUCCEED();
    }

    Result Connect(u32 slot, u8 client_if, const bluetooth::Address &address, u32 *out_conn_id, TimeSpan timeout, bool reconnect) {
        events::SetSlotConnecting(slot, address);
        R_UNLESS(events::IsRadioEnabled(), ResultNoConnection());

        if (client_if == InvalidClientIf) {
            log::Write("slot %u: no client_if available", slot);
            return ResultNoConnection();
        }

        if (!reconnect) {
            const Result rc = btdrvRemoveBond(address);
            log::Write("btdrvRemoveBond slot=%u rc=0x%x", slot, rc.GetValue());
            R_TRY(rc);
        } else {
            // Removing security state and issuing CreateBond again is the path
            // that repeatedly closes returning links with reason 0x0016. Keep
            // any native LE state and let authenticated GATT request encryption.
            log::Write("slot %u: reconnect preserving native security state", slot);
        }
        R_UNLESS(events::IsRadioEnabled(), ResultNoConnection());
        // A previously cancelled direct request can complete during the IPC.
        // Retain that live handle rather than opening twice.
        if (const u32 late_conn_id = events::GetSlotConnectionId(slot); late_conn_id != InvalidConnId) {
            *out_conn_id = late_conn_id;
            log::Write("slot %u: adopting late connection conn_id=%u", slot, late_conn_id);
            R_SUCCEED();
        }
        // Arm the slot after any synchronous cleanup events.
        events::SetSlotConnecting(slot, address);

        Result rc = btdrvConnectGattServer(client_if, address, true, 0);
        log::Write("btdrvConnectGattServer slot=%u client_if=%u rc=0x%x", slot, client_if, rc.GetValue());
        R_TRY(rc);

        rc = WaitForConnection(slot, client_if, address, out_conn_id, timeout);
        if (R_FAILED(rc) && events::IsRadioEnabled()) {
            static_cast<void>(btdrvCancelConnectGattServer(client_if, address, true));
        }
        R_RETURN(rc);
    }

    bool PrepareSecurity(u32 slot, const bluetooth::Address &address, bool reconnect) {
        if (!events::IsRadioEnabled() || events::IsSlotDisconnected(slot)) return false;
        if (reconnect) {
            log::Write("slot %u: reconnect requesting encrypted GATT access without CreateBond", slot);
            return true;
        }
        // Discovery or old bond removal can produce the same core indication
        // as pairing. Only a new indication may satisfy this attempt.
        static_cast<void>(events::TakeSlotBondComplete(slot));
        Result rc = btdrvCreateBond(address, BondTransport_Le);
        log::Write("btdrvCreateBond slot=%u rc=0x%x", slot, rc.GetValue());
        if (R_FAILED(rc)) {
            return false;
        }

        bool bonded = false;
        events::WaitFor(events::InvalidType, TimeSpan::FromSeconds(10), nullptr, [&](const EventRecord &) {
            bonded = bonded || events::TakeSlotBondComplete(slot);
            return bonded || events::IsSlotDisconnected(slot) || !events::IsRadioEnabled();
        });
        if (bonded) {
            // The probe's bond indication can precede a refused pairing's
            // disconnect and late discovery events. Drain them before use.
            events::WaitFor(events::InvalidType, TimeSpan::FromMilliSeconds(500), nullptr, [&](const EventRecord &) {
                return events::IsSlotDisconnected(slot) || !events::IsRadioEnabled();
            });
        }
        if (bonded && !events::IsSlotDisconnected(slot) && events::IsRadioEnabled()) {
            log::Write("slot %u: bond indication received; verifying encrypted GATT access", slot);
            return true;
        }
        log::Write("slot %u: pairing failed (bond_indication=%u disconnected=%u reason=0x%04x radio=%u)",
            slot, bonded, events::IsSlotDisconnected(slot), events::GetSlotDisconnectReason(slot), events::IsRadioEnabled());
        return false;
    }

}
