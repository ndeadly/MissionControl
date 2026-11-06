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
#include "ble_controller_routes.hpp"
#include "../mcmitm_config.hpp"
#include "../controllers/controller_management.hpp"
#include "../controllers/xbox_one_ble_controller.hpp"
#include <atomic>

namespace ams::ble::events {

    namespace {

        // nn::bluetooth::BluetoothSspVariant
        constexpr u32 SspVariant_PasskeyConfirmation = 0;

        // BSA_SEC_SUSPENDED_EVT (switchbrew ConnectionEventType 3): the ACL link changed state. Reported ~5 s after
        // a CreateBond that succeeded, and at once when pairing failed and the stack drops the link.
        constexpr u32 CoreConnectionType_Suspended = 3;

        ControllerRoutes<controller::XboxOneBleController, os::SdkMutex, MaxBleControllers> g_routes;

        constexpr size_t RingSize = 64;
        constinit EventRecord g_ring[RingSize];
        constinit size_t g_ring_read = 0;
        constinit size_t g_ring_write = 0;
        constinit u32 g_ring_dropped = 0;
        constinit os::SdkMutex g_lock;
        os::Event g_ring_event(os::EventClearMode_AutoClear);
        os::Event g_radio_event(os::EventClearMode_ManualClear);
        std::atomic<bool> g_controller_count_changed{false};

        /* Guarded by g_lock */
        constinit bool g_active = false;
        constinit bool g_owns_ble = false;
        constinit bool g_registering = false;
        constinit bool g_scanning = false;
        constinit u32 g_expected_filter_events = 0;
        constexpr size_t MaxClientIfs = 4;
        constinit u8 g_available_client_ifs[MaxClientIfs] = { InvalidClientIf, InvalidClientIf, InvalidClientIf, InvalidClientIf };
        constinit size_t g_client_if_count = 0;
        constinit u32 g_client_registration_generation = 0;
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
            u32 gatt_results = 0;
            u32 gatt_failure = 0;
        };

        constinit EventSlot g_slots[MaxBleControllers] = {};

        constinit bool g_have_attempted = false;
        constinit bluetooth::Address g_attempted = {};

        constinit bool g_adopted = false;
        constinit bluetooth::Address g_adopted_address = {};
        constinit u32 g_adopted_conn_id = InvalidConnId;

        constinit bool g_radio_enabled = false;
        constinit bool g_radio_cycled = false;

        constinit s64 g_inquiry_started_tick = 0;

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

        void EnqueueRecordLocked(bluetooth::BleEventType type, bool forwarded, const bluetooth::BleEventInfo *info) {
            const size_t next = (g_ring_write + 1) % RingSize;
            if (next == g_ring_read) {
                ++g_ring_dropped;
            } else {
                g_ring[g_ring_write].type = type;
                g_ring[g_ring_write].forwarded = forwarded;
                std::memcpy(&g_ring[g_ring_write].info, info, sizeof(bluetooth::BleEventInfo));
                g_ring_write = next;
            }
            g_ring_event.Signal();
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
        g_slots[slot].gatt_results = 0;
        g_slots[slot].gatt_failure = 0;

        g_attempted = address;
        g_have_attempted = true;
    }

    void SetSlotConnection(u32 slot, const bluetooth::Address &address, u32 conn_id) {
        std::scoped_lock lk(g_lock);
        if (slot >= MaxBleControllers) {
            return;
        }
        g_slots[slot].active = true;
        g_slots[slot].address = address;
        g_slots[slot].conn_id = conn_id;
        g_slots[slot].connecting = false;
        g_slots[slot].disconnected = false;
    }

    void SetSlotConnection(u32 slot, u32 conn_id) {
        std::scoped_lock lk(g_lock);
        if (slot >= MaxBleControllers) {
            return;
        }
        g_slots[slot].active = true;
        g_slots[slot].conn_id = conn_id;
        g_slots[slot].connecting = false;
        g_slots[slot].disconnected = false;
    }

    void SetSlotClaimed(u32 slot, bool claimed) {
        std::scoped_lock lk(g_lock);
        if (slot >= MaxBleControllers) {
            return;
        }
        g_slots[slot].claimed = claimed;
    }

    void ClearSlot(u32 slot) {
        std::scoped_lock lk(g_lock);
        if (slot >= MaxBleControllers) {
            return;
        }
        UnregisterSlotController(slot);
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

    void RegisterSlotController(u32 slot, u32 conn_id, std::shared_ptr<controller::XboxOneBleController> ctrl) {
        if (slot < MaxBleControllers) g_routes.Register(slot, conn_id, std::move(ctrl));
    }

    void UnregisterSlotController(u32 slot) {
        if (slot < MaxBleControllers) g_routes.Unregister(slot);
    }

    std::shared_ptr<controller::XboxOneBleController> GetActiveController(u32 conn_id) {
        return g_routes.Find(conn_id);
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
                    if (type == BtdrvBleEventType_ClientRegistration) {
                        const u8 cif = info->client_registration.client_if;
                        if ((info->client_registration.result == 0) && (info->client_registration.status == 1)) {
                            ++g_client_registration_generation;
                            g_spare_client_if = cif;
                            bool found = false;
                            for (size_t i = 0; i < g_client_if_count; ++i) {
                                if (g_available_client_ifs[i] == cif) {
                                    found = true;
                                    break;
                                }
                            }
                            if (!found && (g_client_if_count < MaxClientIfs)) {
                                g_available_client_ifs[g_client_if_count++] = cif;
                            }
                        } else if (info->client_registration.status == 0) {
                            if (cif == InvalidClientIf) {
                                g_client_if_count = 0;
                                g_spare_client_if = InvalidClientIf;
                                for (size_t i = 0; i < MaxClientIfs; ++i) {
                                    g_available_client_ifs[i] = InvalidClientIf;
                                }
                            } else {
                                for (size_t i = 0; i < g_client_if_count; ++i) {
                                    if (g_available_client_ifs[i] == cif) {
                                        for (size_t j = i; j + 1 < g_client_if_count; ++j) {
                                            g_available_client_ifs[j] = g_available_client_ifs[j + 1];
                                        }
                                        g_available_client_ifs[--g_client_if_count] = InvalidClientIf;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    break;
                case BtdrvBleEventType_ScanResult:
                    ours = g_scanning;
                    break;
                case BtdrvBleEventType_ScanFilter:
                    ours = g_expected_filter_events > 0;
                    if (ours) {
                        --g_expected_filter_events;
                    } else if (info->scan_filter.result != 0) {
                        return false;
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

            if (!connected && (connection.conn_id == InvalidConnId) && (connection.client_if == InvalidClientIf)) {
                // Stack-wide BLE teardown/reset (e.g. result=55 status=2 client_if=255 conn_id=0xFFFFFFFF)
                for (size_t s = 0; s < MaxBleControllers; ++s) {
                    if (g_slots[s].active) {
                        g_slots[s].disconnected = true;
                        g_slots[s].disconnect_reason = connection.reason ? connection.reason : connection.result;
                        UnregisterSlotController(static_cast<u32>(s));
                    }
                }
                g_adopted = false;
            } else {
                u32 slot = FindSlotByAddressInternal(connection.address);
                if (slot == InvalidSlot) {
                    slot = FindSlotByConnIdInternal(connection.conn_id);
                }

                if (slot != InvalidSlot) {
                    if (!connected && ((g_slots[slot].conn_id == InvalidConnId) || (connection.conn_id == g_slots[slot].conn_id))) {
                        g_slots[slot].disconnected = true;
                        g_slots[slot].disconnect_reason = connection.reason;
                        UnregisterSlotController(slot);
                    } else if (connected) {
                        g_slots[slot].conn_id = connection.conn_id;
                        g_slots[slot].connecting = false;
                        g_slots[slot].disconnected = false;
                    }
                }

                if (ours && connected && (slot == InvalidSlot) && g_have_attempted && AddressEqual(connection.address, g_attempted)) {
                    g_adopted = true;
                    g_adopted_address = connection.address;
                    g_adopted_conn_id = connection.conn_id;
                }
            }
        }

        if (type == BtdrvBleEventType_ConnectionUpdate) {
            auto &cu = info->connection_update;
            profile::RecordConnectionUpdate(cu.conn_interval, cu.conn_latency, cu.supervision_tout);
        }

        EnqueueRecordLocked(type, !ours, info);

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

            if (type == BtdrvEventType_BluetoothCrash) {
                // btm must receive this event so it performs its normal recovery.
                // Discard handles from the old stack before accepting new clients.
                g_client_if_count = 0;
                g_spare_client_if = InvalidClientIf;
                std::fill(std::begin(g_available_client_ifs), std::end(g_available_client_ifs), InvalidClientIf);
                g_adopted = false;
                g_have_attempted = false;
                g_expected_filter_events = 0;
                for (u32 s = 0; s < MaxBleControllers; ++s) {
                    if (g_slots[s].active) {
                        g_slots[s].disconnected = true;
                        UnregisterSlotController(s);
                    }
                }
                g_ring_event.Signal();
                return false;
            }

            const auto check_match = [&](const bluetooth::Address &addr) -> bool {
                for (size_t s = 0; s < MaxBleControllers; ++s) {
                    if (g_slots[s].active && (g_slots[s].connecting || g_slots[s].claimed || (g_slots[s].conn_id != InvalidConnId)) && AddressEqual(addr, g_slots[s].address)) {
                        matched_slot = static_cast<u32>(s);
                        return true;
                    }
                }
                if (g_have_attempted && AddressEqual(addr, g_attempted)) {
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
            static_cast<void>(btdrvRespondToSspRequest(request.addr, SspVariant_PasskeyConfirmation, true, request.passkey));
            return true;
        }

        if (type == BtdrvEventType_Connection) {
            auto &connection = info->connection.v12;
            if (connection.type == CoreConnectionType_Suspended) {
                std::scoped_lock lk(g_lock);
                if (matched_slot != InvalidSlot) {
                    g_slots[matched_slot].bond_complete = true;
                }
                g_ring_event.Signal();
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

    void CancelExpectedFilterEvent() {
        std::scoped_lock lk(g_lock);
        if (g_expected_filter_events) --g_expected_filter_events;
    }

    void NoteGattResult(u32 conn_id, u8 notify_type, const BtdrvGattAttributeUuid &char_uuid, u32 result) {
        // Ignore asynchronous GATT write completions from rumble output reports so they never interfere with setup
        if ((notify_type == GattNotifyType_WriteCharacteristic) && IsUuid16(char_uuid, GattCharReport)) {
            return;
        }

        std::scoped_lock lk(g_lock);
        const u32 slot = FindSlotByConnIdInternal(conn_id);
        if (slot != InvalidSlot) {
            ++g_slots[slot].gatt_results;
            if (result != 0) {
                g_slots[slot].gatt_failure = result;
            }
        }
        g_ring_event.Signal();
    }

    u32 GetGattFailure(u32 slot_index) {
        std::scoped_lock lk(g_lock);
        if (slot_index >= MaxBleControllers) {
            return 0;
        }
        return g_slots[slot_index].gatt_failure;
    }

    void ClearGattFailure(u32 slot_index) {
        std::scoped_lock lk(g_lock);
        if (slot_index < MaxBleControllers) {
            g_slots[slot_index].gatt_failure = 0;
        }
    }

    u32 GetGattResultCount(u32 slot_index) {
        std::scoped_lock lk(g_lock);
        if (slot_index >= MaxBleControllers) {
            return 0;
        }
        return g_slots[slot_index].gatt_results;
    }

    bool WaitForGattResult(u32 slot_index, u32 count_before, TimeSpan timeout) {
        WaitFor(InvalidType, timeout, nullptr, [&](const EventRecord &) {
            return (GetGattResultCount(slot_index) != count_before) || IsSlotDisconnected(slot_index) || !IsRadioEnabled();
        });
        return !IsSlotDisconnected(slot_index) && IsRadioEnabled() && (GetGattResultCount(slot_index) != count_before);
    }

    bool TakeAdoptedConnection(bluetooth::Address *out_address, u32 *out_conn_id) {
        std::scoped_lock lk(g_lock);
        if (!g_adopted) {
            return false;
        }

        *out_address = g_adopted_address;
        *out_conn_id = g_adopted_conn_id;
        g_adopted = false;
        return true;
    }

    bool HasAdoptedConnection() {
        std::scoped_lock lk(g_lock);
        return g_adopted;
    }

    void SetInquiryActive(bool active) {
        std::scoped_lock lk(g_lock);
        g_inquiry_started_tick = active ? os::GetSystemTick().GetInt64Value() : 0;
        if (!active) {
            g_ring_event.Signal();
        }
    }

    bool IsPairingWindowOpen() {
        std::scoped_lock lk(g_lock);
        return g_inquiry_started_tick != 0;
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
            g_ring_event.Signal();
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

    u8 GetClientIfForSlot(u32 slot) {
        std::scoped_lock lk(g_lock);
        if (slot < g_client_if_count) {
            return g_available_client_ifs[slot];
        }
        return (g_client_if_count > 0) ? g_available_client_ifs[0] : g_spare_client_if;
    }

    u32 GetClientRegistrationGeneration() {
        std::scoped_lock lk(g_lock);
        return g_client_registration_generation;
    }

    size_t GetAvailableClientIfCount() {
        std::scoped_lock lk(g_lock);
        return g_client_if_count;
    }

    void PushRecord(const EventRecord &record) {
        std::scoped_lock lk(g_lock);
        EnqueueRecordLocked(record.type, record.forwarded, &record.info);
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
            case BtdrvBleEventType_ConnectionUpdate: {
                const u32 int_w = (info.connection_update.conn_interval * 125) / 100;
                const u32 int_f = (info.connection_update.conn_interval * 125) % 100;
                log::Write("evt ConnectionUpdate result=%u conn_id=%u interval=%u.%02ums (%u) latency=%u timeout=%ums",
                    info.connection_update.result, info.connection_update.conn_id,
                    int_w, int_f, info.connection_update.conn_interval,
                    info.connection_update.conn_latency, static_cast<u32>(info.connection_update.supervision_tout * 10));
                break;
            }
            case BtdrvBleEventType_PreferredConnectionParameters: {
                auto &pref = info.preferred_connection_parameters;
                const u32 min_w = (pref.min_conn_interval * 125) / 100;
                const u32 min_f = (pref.min_conn_interval * 125) % 100;
                const u32 max_w = (pref.max_conn_interval * 125) / 100;
                const u32 max_f = (pref.max_conn_interval * 125) % 100;
                log::Write("evt PreferredConnectionParameters conn_id=%u min=%u (%u.%02ums) max=%u (%u.%02ums) latency=%u timeout=%ums",
                    pref.conn_id,
                    pref.min_conn_interval, min_w, min_f,
                    pref.max_conn_interval, max_w, max_f,
                    pref.slave_latency,
                    static_cast<u32>(pref.supervision_tout * 10));
                break;
            }
            case BtdrvBleEventType_ClientConnection:
                log::Write("evt ClientConnection result=%u status=%u client_if=%u conn_id=%u reason=0x%04x", info.client_connection.result, info.client_connection.status, info.client_connection.client_if, info.client_connection.conn_id, info.client_connection.reason);
                LogAddress("  address", info.client_connection.address);
                break;
            case BtdrvBleEventType_ScanResult:
                if (info.scan_result.status != BleScanStatus_NewDevice) {
                    log::Write("evt ScanResult result=%u status=0x%02x", info.scan_result.result, info.scan_result.status);
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

    u16 GetTargetMaxConnectionInterval(size_t active_count) {
        const size_t total_controllers = std::max({
            static_cast<size_t>(1),
            active_count,
            GetActiveConnectionCount(),
            controller::GetConnectedControllerCount()
        });

        if ((total_controllers >= 4) && (total_controllers > GetActiveConnectionCount())) {
            return 20; // 25.0 ms max (min=16 / 20.0 ms)
        } else if (total_controllers >= 2) {
            return 12; // 15.0 ms harmonic
        } else {
            return mitm::GetGlobalConfig()->ble.low_latency ? 9 : 0; // 11.25 ms if low_latency enabled
        }
    }

    u16 RequestLowLatencyConnectionParameters(const bluetooth::Address &address, size_t active_count) {
        const u16 max_interval = GetTargetMaxConnectionInterval(active_count);
        if (max_interval == 0) return 0;
        const u16 min_interval = max_interval == 20 ? 16 : max_interval == 9 ? 6 : max_interval;

        BtdrvBleConnectionParameter param = {
            .min_conn_interval = min_interval,
            .max_conn_interval = max_interval,
            .min_ce_length     = 0,
            .max_ce_length     = 0,
            .slave_latency     = 0,            // no skipped polling intervals
            .supervision_tout  = 300,          // 3000 ms
        };

        Result rc = btdrvSetBleConnectionParameter(address, &param, false);
        char addr_str[18];
        std::snprintf(addr_str, sizeof(addr_str), "%02x:%02x:%02x:%02x:%02x:%02x",
            address.address[0], address.address[1], address.address[2],
            address.address[3], address.address[4], address.address[5]);
        log::Write("btdrvSetBleConnectionParameter [%s] min=%u max=%u total_pads=%u rc=0x%x",
            addr_str, min_interval, max_interval, static_cast<u32>(controller::GetConnectedControllerCount()), rc.GetValue());
        return R_SUCCEEDED(rc) ? max_interval : 0;
    }

    void NotifyControllerCountChanged() {
        g_controller_count_changed.store(true, std::memory_order_release);
        g_ring_event.Signal();
    }

    bool ConsumeControllerCountChanged() {
        return g_controller_count_changed.exchange(false, std::memory_order_acq_rel);
    }

}
