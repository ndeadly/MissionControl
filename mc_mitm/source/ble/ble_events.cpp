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
#include "ble_events.hpp"
#include "ble_log.hpp"
#include "ble_profile.hpp"
#include "../mcmitm_config.hpp"
#include "../controllers/xbox_one_ble_controller.hpp"
#include <atomic>

namespace ams::ble::events {

    constinit std::atomic<controller::XboxOneBleController *> g_active_controllers[MaxConnIdRange] = {};

    namespace {

        // nn::bluetooth::BluetoothSspVariant
        constexpr u32 SspVariant_PasskeyConfirmation = 0;

        // BSA_SEC_SUSPENDED_EVT (switchbrew ConnectionEventType 3): the ACL link changed state. Reported ~5 s after
        // a CreateBond that succeeded, and at once when pairing failed and the stack drops the link.
        constexpr u32 CoreConnectionType_Suspended = 3;

        constexpr size_t RingSize = 64;
        constinit EventRecord g_ring[RingSize];
        constinit size_t g_ring_read = 0;
        constinit size_t g_ring_write = 0;
        constinit u32 g_ring_dropped = 0;
        constinit os::SdkMutex g_lock;
        os::Event g_ring_event(os::EventClearMode_AutoClear);
        os::Event g_radio_event(os::EventClearMode_ManualClear);

        /* Guarded by g_lock */
        constinit bool g_active = false;
        constinit bool g_owns_ble = false;
        constinit bool g_registering = false;
        constinit bool g_scanning = false;
        constinit u32 g_expected_filter_events = 0;
        constinit u8 g_spare_client_if = InvalidClientIf;

        struct EventSlot {
            bool active = false;
            bluetooth::Address address = {};
            u32 conn_id = InvalidConnId;
            bool connecting = false;
            bool claimed = false;
            bool disconnected = false;
            u16 disconnect_reason = 0;
            bool bond_complete = false;
        };

        constinit EventSlot g_slots[MaxBleControllers] = {};

        constinit bool g_have_target = false;
        constinit bluetooth::Address g_target = {};
        constinit u32 g_conn_id = InvalidConnId;
        constinit std::atomic<u32> g_conn_id_fast = InvalidConnId;
        constinit bool g_disconnected = false;
        constinit u16 g_disconnect_reason = 0;
        constinit u32 g_gatt_results = 0;
        constinit u32 g_gatt_failure = 0;

        constinit bool g_have_claimed = false;
        constinit bluetooth::Address g_claimed = {};

        constinit bool g_have_attempted = false;
        constinit bluetooth::Address g_attempted = {};

        constinit bool g_adopted = false;
        constinit bluetooth::Address g_adopted_address = {};
        constinit u32 g_adopted_conn_id = InvalidConnId;

        constinit bool g_radio_enabled = false;
        constinit bool g_radio_cycled = false;

        constinit s64 g_inquiry_started_tick = 0;
        constinit bool g_bond_complete = false;

        constinit Sink g_sink = nullptr;

        void LogGattAttribute(const BtdrvGattAttribute &attr) {
            char uuid[0x40];
            FormatUuid(attr.id.uuid, uuid, sizeof(uuid));
            log::Write("  attr type=%u handle=0x%04x end=0x%04x inst=%u prop=0x%02x primary=%u uuid=%s",
                attr.type, attr.handle, attr.group_end_handle, attr.id.instance_id, attr.property, attr.is_primary, uuid);
        }

        bool IsConnectionFamily(bluetooth::BleEventType type) {
            switch (type) {
                case BtdrvBleEventType_ClientConnection:
                case BtdrvBleEventType_ConnectionUpdate:
                case BtdrvBleEventType_PreferredConnectionParameters:
                case BtdrvBleEventType_ClientNotify:
                case BtdrvBleEventType_ClientCacheSave:
                case BtdrvBleEventType_ClientCacheLoad:
                case BtdrvBleEventType_ClientConfigureMtu:
                    return true;
                default:
                    return false;
            }
        }

        u32 FindSlotByConnIdInternal(u32 conn_id) {
            if (conn_id == InvalidConnId) {
                return InvalidSlot;
            }
            for (size_t i = 0; i < MaxBleControllers; ++i) {
                if (g_slots[i].active && (g_slots[i].conn_id == conn_id)) {
                    return static_cast<u32>(i);
                }
            }
            return InvalidSlot;
        }

        u32 FindSlotByAddressInternal(const bluetooth::Address &address) {
            for (size_t i = 0; i < MaxBleControllers; ++i) {
                if (g_slots[i].active && AddressEqual(g_slots[i].address, address)) {
                    return static_cast<u32>(i);
                }
            }
            return InvalidSlot;
        }

    }

    u32 FindSlotByConnId(u32 conn_id) {
        std::scoped_lock lk(g_lock);
        return FindSlotByConnIdInternal(conn_id);
    }

    u32 FindSlotByAddress(const bluetooth::Address &address) {
        std::scoped_lock lk(g_lock);
        return FindSlotByAddressInternal(address);
    }

    bool IsAddressActive(const bluetooth::Address &address) {
        std::scoped_lock lk(g_lock);
        return FindSlotByAddressInternal(address) != InvalidSlot;
    }

    size_t GetActiveConnectionCount() {
        std::scoped_lock lk(g_lock);
        size_t count = 0;
        for (size_t i = 0; i < MaxBleControllers; ++i) {
            if (g_slots[i].active && (g_slots[i].conn_id != InvalidConnId) && !g_slots[i].disconnected) {
                ++count;
            }
        }
        return count;
    }

    void SetSlotConnecting(u32 slot, const bluetooth::Address &address) {
        std::scoped_lock lk(g_lock);
        if (slot >= MaxBleControllers) {
            return;
        }
        g_slots[slot].active = true;
        g_slots[slot].address = address;
        g_slots[slot].conn_id = InvalidConnId;
        g_slots[slot].connecting = true;
        g_slots[slot].claimed = false;
        g_slots[slot].disconnected = false;
        g_slots[slot].disconnect_reason = 0;
        g_slots[slot].bond_complete = false;

        g_target = address;
        g_have_target = true;
        g_attempted = address;
        g_have_attempted = true;
        g_disconnected = false;
        g_bond_complete = false;
    }

    void SetSlotConnection(u32 slot, u32 conn_id) {
        std::scoped_lock lk(g_lock);
        if (slot >= MaxBleControllers) {
            return;
        }
        g_slots[slot].conn_id = conn_id;
        g_slots[slot].connecting = false;
        g_slots[slot].disconnected = false;

        g_conn_id = conn_id;
        g_conn_id_fast.store(conn_id, std::memory_order_relaxed);
        g_disconnected = false;
    }

    void SetSlotClaimed(u32 slot, bool claimed) {
        std::scoped_lock lk(g_lock);
        if (slot >= MaxBleControllers) {
            return;
        }
        g_slots[slot].claimed = claimed;
        if (claimed) {
            g_claimed = g_slots[slot].address;
            g_have_claimed = true;
        }
    }

    void ClearSlot(u32 slot) {
        std::scoped_lock lk(g_lock);
        if (slot >= MaxBleControllers) {
            return;
        }
        if (g_slots[slot].conn_id < MaxConnIdRange) {
            g_active_controllers[g_slots[slot].conn_id].store(nullptr, std::memory_order_release);
        }
        g_slots[slot] = {};
    }

    bool IsSlotDisconnected(u32 slot) {
        std::scoped_lock lk(g_lock);
        if (slot >= MaxBleControllers) {
            return true;
        }
        return g_slots[slot].disconnected;
    }

    u16 GetSlotDisconnectReason(u32 slot) {
        std::scoped_lock lk(g_lock);
        if (slot >= MaxBleControllers) {
            return 0;
        }
        return g_slots[slot].disconnect_reason;
    }

    bool TakeSlotBondComplete(u32 slot) {
        std::scoped_lock lk(g_lock);
        if (slot >= MaxBleControllers) {
            return false;
        }
        const bool complete = g_slots[slot].bond_complete;
        g_slots[slot].bond_complete = false;
        return complete;
    }

    void RegisterActiveController(u32 conn_id, controller::XboxOneBleController *ctrl) {
        if (conn_id < MaxConnIdRange) {
            g_active_controllers[conn_id].store(ctrl, std::memory_order_release);
        }
    }

    void UnregisterActiveController(u32 conn_id) {
        if (conn_id < MaxConnIdRange) {
            g_active_controllers[conn_id].store(nullptr, std::memory_order_release);
        }
    }

    controller::XboxOneBleController *GetActiveController(u32 conn_id) noexcept {
        if (conn_id < MaxConnIdRange) {
            return g_active_controllers[conn_id].load(std::memory_order_acquire);
        }
        return nullptr;
    }

    bool OnBleEvent(bluetooth::BleEventType type, const bluetooth::BleEventInfo *info) {
        std::scoped_lock lk(g_lock);

        if (!g_active) {
            return false;
        }

        bool ours = false;
        if (g_owns_ble) {
            switch (type) {
                case BtdrvBleEventType_ClientRegistration:
                case BtdrvBleEventType_ServerRegistration:
                    ours = g_registering;
                    if (!ours && (type == BtdrvBleEventType_ClientRegistration) && (info->client_registration.result == 0) && (info->client_registration.status == 1)) {
                        g_spare_client_if = info->client_registration.client_if;
                    }
                    break;
                case BtdrvBleEventType_ScanResult:
                    ours = g_scanning;
                    break;
                case BtdrvBleEventType_ScanFilter:
                    ours = g_expected_filter_events > 0;
                    if (ours) {
                        --g_expected_filter_events;
                    }
                    break;
                default:
                    ours = IsConnectionFamily(type);
                    break;
            }
        }

        if (type == BtdrvBleEventType_ClientConnection) {
            auto &connection = info->client_connection;
            const bool connected = (connection.result == 0) && (connection.status == BleConnectionStatus_Connected);

            u32 slot = FindSlotByAddressInternal(connection.address);
            if (slot == InvalidSlot && connection.conn_id < MaxConnIdRange) {
                slot = FindSlotByConnIdInternal(connection.conn_id);
            }

            if (slot != InvalidSlot) {
                if (!connected) {
                    g_slots[slot].disconnected = true;
                    g_slots[slot].disconnect_reason = connection.reason;
                    if (g_slots[slot].conn_id < MaxConnIdRange) {
                        g_active_controllers[g_slots[slot].conn_id].store(nullptr, std::memory_order_release);
                    }
                } else {
                    g_slots[slot].conn_id = connection.conn_id;
                    g_slots[slot].disconnected = false;
                }
            }

            if (g_have_target && AddressEqual(connection.address, g_target) && !connected) {
                g_disconnected = true;
                g_disconnect_reason = connection.reason;
            }

            if (ours && connected && (slot == InvalidSlot) && g_have_attempted && AddressEqual(connection.address, g_attempted)) {
                g_adopted = true;
                g_adopted_address = connection.address;
                g_adopted_conn_id = connection.conn_id;
            }
        }

        if (type == BtdrvBleEventType_PreferredConnectionParameters) {
            auto &pref = info->preferred_connection_parameters;
            log::Write("evt PreferredConnectionParameters conn_id=%u min=%u (%.2fms) max=%u (%.2fms) latency=%u timeout=%ums",
                pref.conn_id, pref.min_conn_interval, pref.min_conn_interval * 1.25f,
                pref.max_conn_interval, pref.max_conn_interval * 1.25f,
                pref.slave_latency, static_cast<u32>(pref.supervision_tout * 10));

            if (mitm::GetGlobalConfig()->ble.low_latency) {
                u32 slot = FindSlotByConnIdInternal(pref.conn_id);
                bluetooth::Address target_address = {};
                bool have_address = false;
                if (slot != InvalidSlot && g_slots[slot].active) {
                    target_address = g_slots[slot].address;
                    have_address = true;
                } else if (g_have_target) {
                    target_address = g_target;
                    have_address = true;
                }

                if (have_address) {
                    size_t active_count = 0;
                    for (size_t s = 0; s < MaxBleControllers; ++s) {
                        if (g_slots[s].active && (g_slots[s].conn_id != InvalidConnId) && !g_slots[s].disconnected) {
                            ++active_count;
                        }
                    }
                    const u16 max_interval = (active_count > 1) ? 12 : 9;

                    BtdrvBleConnectionParameter param = {
                        .min_conn_interval = std::max<u16>(6, pref.min_conn_interval),
                        .max_conn_interval = std::max<u16>(pref.min_conn_interval, std::min<u16>(max_interval, pref.max_conn_interval)),
                        .min_ce_length     = 0,
                        .max_ce_length     = 0,
                        .slave_latency     = 0,
                        .supervision_tout  = pref.supervision_tout ? pref.supervision_tout : static_cast<u16>(300),
                    };
                    Result rc = btdrvSetBleConnectionParameter(target_address, &param, false);
                    log::Write("btdrvSetBleConnectionParameter(preferred, active=%zu, max=%u) rc=0x%x", active_count, max_interval, rc.GetValue());
                }
            }
        }

        if (type == BtdrvBleEventType_ConnectionUpdate) {
            auto &cu = info->connection_update;
            profile::RecordConnectionUpdate(cu.conn_interval, cu.conn_latency, cu.supervision_tout);
        }

        const size_t next = (g_ring_write + 1) % RingSize;
        if (next == g_ring_read) {
            ++g_ring_dropped;
        } else {
            g_ring[g_ring_write].type = type;
            g_ring[g_ring_write].forwarded = !ours;
            std::memcpy(&g_ring[g_ring_write].info, info, sizeof(bluetooth::BleEventInfo));
            g_ring_write = next;
        }
        g_ring_event.Signal();

        return ours;
    }

    bool OnCoreEvent(u32 type, const bluetooth::EventInfo *info) {
        if (hos::GetVersion() < hos::Version_12_0_0) {
            return false;
        }

        bool is_ours = false;
        u32 matched_slot = InvalidSlot;

        {
            std::scoped_lock lk(g_lock);
            if (!g_owns_ble) {
                return false;
            }

            const auto check_match = [&](const bluetooth::Address &addr) -> bool {
                for (size_t s = 0; s < MaxBleControllers; ++s) {
                    if (g_slots[s].active && (g_slots[s].connecting || g_slots[s].claimed) && AddressEqual(addr, g_slots[s].address)) {
                        matched_slot = static_cast<u32>(s);
                        return true;
                    }
                }
                if ((g_have_target && AddressEqual(addr, g_target)) || (g_have_claimed && AddressEqual(addr, g_claimed)) || (g_have_attempted && AddressEqual(addr, g_attempted))) {
                    return true;
                }
                return false;
            };

            if (type == BtdrvEventType_SspRequest) {
                if (check_match(info->ssp_request.v12.addr)) {
                    is_ours = true;
                }
            } else if (type == BtdrvEventType_Connection) {
                if (check_match(info->connection.v12.addr)) {
                    is_ours = true;
                }
            }
        }

        if (!is_ours) {
            return false;
        }

        if (type == BtdrvEventType_SspRequest) {
            auto &request = info->ssp_request.v12;
            Result rc = btdrvRespondToSspRequest(request.addr, SspVariant_PasskeyConfirmation, true, request.passkey);
            log::Write("btdrvRespondToSspRequest(accept) rc=0x%x", rc.GetValue());
            return true;
        }

        if (type == BtdrvEventType_Connection) {
            auto &connection = info->connection.v12;
            log::Write("core evt Connection type=%u for controller", connection.type);
            LogAddress("  address", connection.addr);
            if (connection.type == CoreConnectionType_Suspended) {
                std::scoped_lock lk(g_lock);
                if (matched_slot != InvalidSlot) {
                    g_slots[matched_slot].bond_complete = true;
                }
                g_bond_complete = true;
            }
            return true;
        }

        return false;
    }

    void Activate() {
        std::scoped_lock lk(g_lock);
        g_active = true;
    }

    void TakeOwnership() {
        std::scoped_lock lk(g_lock);
        g_owns_ble = true;
        g_radio_enabled = true;
        g_radio_event.Signal();
    }

    void SetRegistering(bool registering) {
        std::scoped_lock lk(g_lock);
        g_registering = registering;
    }

    void SetScanning(bool scanning) {
        std::scoped_lock lk(g_lock);
        g_scanning = scanning;
    }

    void ExpectFilterEvent() {
        std::scoped_lock lk(g_lock);
        ++g_expected_filter_events;
    }

    void SetTarget(const bluetooth::Address *address) {
        std::scoped_lock lk(g_lock);
        if (address) {
            g_target = *address;
            g_have_target = true;
            g_attempted = *address;
            g_have_attempted = true;
        } else {
            g_have_target = false;
        }
        g_disconnected = false;
        g_bond_complete = false;
    }

    bool GetTarget(bluetooth::Address *out) {
        std::scoped_lock lk(g_lock);
        if (g_have_target) {
            *out = g_target;
        }
        return g_have_target;
    }

    void SetConnection(u32 conn_id) {
        std::scoped_lock lk(g_lock);
        g_conn_id = conn_id;
        g_conn_id_fast.store(conn_id, std::memory_order_relaxed);
        g_disconnected = false;
    }

    void NoteGattResult(u32 result) {
        std::scoped_lock lk(g_lock);
        ++g_gatt_results;
        if (result != 0) {
            g_gatt_failure = result;
        }
    }

    u32 GetGattFailure() {
        std::scoped_lock lk(g_lock);
        return g_gatt_failure;
    }

    void ClearGattFailure() {
        std::scoped_lock lk(g_lock);
        g_gatt_failure = 0;
    }

    u32 GetGattResultCount() {
        std::scoped_lock lk(g_lock);
        return g_gatt_results;
    }

    bool WaitForGattResult(u32 slot_index, u32 count_before, TimeSpan timeout) {
        const auto deadline = os::GetSystemTick().ToTimeSpan() + timeout;
        while (os::GetSystemTick().ToTimeSpan() < deadline) {
            if ((GetGattResultCount() != count_before) || IsSlotDisconnected(slot_index)) {
                return true;
            }
            Pump(TimeSpan::FromMilliSeconds(50));
        }
        return false;
    }

    bool WaitForGattResult(u32 count_before, TimeSpan timeout) {
        return WaitForGattResult(0, count_before, timeout);
    }

    u32 GetConnection() {
        std::scoped_lock lk(g_lock);
        return g_conn_id;
    }

    u32 GetConnectionFast() noexcept {
        return g_conn_id_fast.load(std::memory_order_relaxed);
    }

    bool IsDisconnected() {
        std::scoped_lock lk(g_lock);
        return g_disconnected;
    }

    u16 GetDisconnectReason() {
        std::scoped_lock lk(g_lock);
        return g_disconnect_reason;
    }

    bool TakeAdoptedConnection(bluetooth::Address *out_address, u32 *out_conn_id) {
        std::scoped_lock lk(g_lock);
        if (!g_adopted) {
            return false;
        }

        *out_address = g_adopted_address;
        *out_conn_id = g_adopted_conn_id;
        g_adopted = false;

        g_target = g_adopted_address;
        g_have_target = true;
        g_conn_id = g_adopted_conn_id;
        g_conn_id_fast.store(g_adopted_conn_id, std::memory_order_relaxed);
        g_disconnected = false;
        return true;
    }

    bool HasAdoptedConnection() {
        std::scoped_lock lk(g_lock);
        return g_adopted;
    }

    void SetClaimedAddress(const bluetooth::Address *address) {
        std::scoped_lock lk(g_lock);
        if (address) {
            g_claimed = *address;
            g_have_claimed = true;
        } else {
            g_have_claimed = false;
        }
    }

    void SetInquiryActive(bool active) {
        std::scoped_lock lk(g_lock);
        g_inquiry_started_tick = active ? os::GetSystemTick().GetInt64Value() : 0;
    }

    bool IsPairingWindowOpen() {
        std::scoped_lock lk(g_lock);
        return g_inquiry_started_tick != 0;
    }

    bool TakeBondComplete() {
        std::scoped_lock lk(g_lock);
        const bool complete = g_bond_complete;
        g_bond_complete = false;
        return complete;
    }

    void SetRadioEnabled(bool enabled) {
        std::scoped_lock lk(g_lock);
        if (enabled) {
            if (!g_radio_enabled) {
                g_radio_cycled = true;
            }
            g_radio_enabled = true;
            g_radio_event.Signal();
        } else {
            g_radio_enabled = false;
            g_radio_event.Clear();
        }
    }

    bool IsRadioEnabled() {
        std::scoped_lock lk(g_lock);
        return g_radio_enabled;
    }

    void WaitRadioEnabled() {
        g_radio_event.Wait();
    }

    bool TakeRadioCycled() {
        std::scoped_lock lk(g_lock);
        const bool cycled = g_radio_cycled;
        g_radio_cycled = false;
        return cycled;
    }

    u8 GetSpareClientIf() {
        std::scoped_lock lk(g_lock);
        return g_spare_client_if;
    }

    u32 TakeRingDropped() {
        std::scoped_lock lk(g_lock);
        const u32 dropped = g_ring_dropped;
        g_ring_dropped = 0;
        return dropped;
    }

    bool Pop(EventRecord *out) {
        std::scoped_lock lk(g_lock);
        if (g_ring_read == g_ring_write) {
            return false;
        }

        *out = g_ring[g_ring_read];
        g_ring_read = (g_ring_read + 1) % RingSize;
        return true;
    }

    void WaitForQueue(TimeSpan timeout) {
        g_ring_event.TimedWait(timeout);
    }

    void SetSink(Sink sink) {
        g_sink = sink;
    }

    Sink GetSink() {
        return g_sink;
    }

    void LogAddress(const char *label, const bluetooth::Address &address) {
        log::Write("%s %02x:%02x:%02x:%02x:%02x:%02x", label,
            address.address[0], address.address[1], address.address[2],
            address.address[3], address.address[4], address.address[5]);
    }

    void Log(const EventRecord &e) {
        if (!log::IsEnabled()) {
            return;
        }

        if (u32 dropped = TakeRingDropped(); dropped > 0) {
            log::Write("queue full, %u events lost", dropped);
        }

        auto &info = e.info;
        switch (e.type) {
            case BtdrvBleEventType_ClientRegistration:
                log::Write("evt ClientRegistration result=%u client_if=%u status=%u", info.client_registration.result, info.client_registration.client_if, info.client_registration.status);
                break;
            case BtdrvBleEventType_ConnectionUpdate:
                log::Write("evt ConnectionUpdate result=%u conn_id=%u interval=%.2fms (%u) latency=%u timeout=%ums",
                    info.connection_update.result, info.connection_update.conn_id,
                    info.connection_update.conn_interval * 1.25f, info.connection_update.conn_interval,
                    info.connection_update.conn_latency, static_cast<u32>(info.connection_update.supervision_tout * 10));
                break;
            case BtdrvBleEventType_PreferredConnectionParameters:
                log::Write("evt PreferredConnectionParameters conn_id=%u min=%u (%.2fms) max=%u (%.2fms) latency=%u timeout=%ums",
                    info.preferred_connection_parameters.conn_id,
                    info.preferred_connection_parameters.min_conn_interval,
                    info.preferred_connection_parameters.min_conn_interval * 1.25f,
                    info.preferred_connection_parameters.max_conn_interval,
                    info.preferred_connection_parameters.max_conn_interval * 1.25f,
                    info.preferred_connection_parameters.slave_latency,
                    static_cast<u32>(info.preferred_connection_parameters.supervision_tout * 10));
                break;
            case BtdrvBleEventType_ClientConnection:
                log::Write("evt ClientConnection result=%u status=%u client_if=%u conn_id=%u reason=0x%04x", info.client_connection.result, info.client_connection.status, info.client_connection.client_if, info.client_connection.conn_id, info.client_connection.reason);
                LogAddress("  address", info.client_connection.address);
                break;
            case BtdrvBleEventType_ScanResult:
                if (info.scan_result.status != BleScanStatus_NewDevice) {
                    log::Write("evt ScanResult status=0x%02x", info.scan_result.status);
                }
                break;
            case BtdrvBleEventType_ScanFilter:
                log::Write("evt ScanFilter result=%u action=%u", info.scan_filter.result, info.scan_filter.action);
                break;
            case BtdrvBleEventType_ClientNotify: {
                auto &notify = info.client_notify;
                char chr[0x40];
                FormatUuid(notify.char_uuid, chr, sizeof(chr));
                log::Write("evt ClientNotify result=%u conn_id=%u type=%u char=%s size=%u", notify.result, notify.conn_id, notify.type, chr, notify.size);
                break;
            }
            case BtdrvBleEventType_ClientCacheSave:
                log::Write("evt ClientCacheSave result=%u conn_id=%u count=%u", info.client_cache_save.result, info.client_cache_save.conn_id, info.client_cache_save.count);
                for (u32 i = 0; i < std::min<u32>(info.client_cache_save.count, 10); ++i) {
                    LogGattAttribute(info.client_cache_save.attr_list[i]);
                }
                break;
            case BtdrvBleEventType_ClientConfigureMtu:
                log::Write("evt ClientConfigureMtu result=%u conn_id=%u mtu=%u", info.client_configure_mtu.result, info.client_configure_mtu.conn_id, info.client_configure_mtu.mtu);
                break;
            default:
                log::Write("evt type=%u", static_cast<u32>(e.type));
                break;
        }

        if (e.forwarded && IsConnectionFamily(e.type)) {
            log::Write("  (forwarded to btm)");
        }
    }

    void RequestLowLatencyConnectionParameters(const bluetooth::Address &address, size_t active_count) {
        if (!mitm::GetGlobalConfig()->ble.low_latency) {
            return;
        }

        if (active_count == 0) {
            active_count = std::max<size_t>(1, GetActiveConnectionCount());
        }

        const u16 max_interval = (active_count > 1) ? 12 : 9;

        BtdrvBleConnectionParameter param = {
            .min_conn_interval = 6,            // 7.5 ms
            .max_conn_interval = max_interval, // 9 (11.25 ms) for 1 pad, 12 (15.0 ms) for 2-4 pads
            .min_ce_length     = 0,
            .max_ce_length     = 0,
            .slave_latency     = 0,            // no skipped polling intervals
            .supervision_tout  = 300,          // 3000 ms
        };

        Result rc = btdrvSetBleConnectionParameter(address, &param, false);
        log::Write("btdrvSetBleConnectionParameter(low_latency, active=%zu, max=%u) rc=0x%x", active_count, max_interval, rc.GetValue());
    }

}
