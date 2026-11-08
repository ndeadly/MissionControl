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
#include "ble_hid_host.hpp"
#include "ble_events.hpp"
#include "ble_gatt.hpp"
#include "ble_log.hpp"
#include "ble_profile.hpp"
#include "ble_results.hpp"
#include "ble_recovery.hpp"
#include "ble_pairing.hpp"
#include "ble_advertising.hpp"
#include "../mcmitm_config.hpp"
#include "../mcmitm_initialization.hpp"
#include "../bluetooth_mitm/bluetooth/bluetooth_ble.hpp"
#include "../controllers/controller_management.hpp"
#include "../controllers/xbox_one_ble_controller.hpp"

namespace ams::ble {

    namespace {

        constexpr s32 ThreadPriority = 9;
        constexpr size_t ThreadStackSize = 0x4000;
        alignas(os::ThreadStackAlignment) constinit u8 g_thread_stack[ThreadStackSize];
        constinit os::ThreadType g_thread;
        constinit bool g_launched = false;

        constexpr size_t LeHidThreadStackSize = 0x2000;
        alignas(os::ThreadStackAlignment) constinit u8 g_le_hid_thread_stack[LeHidThreadStackSize];
        constinit os::ThreadType g_le_hid_thread;
        constinit EventRecord g_le_hid_record;

        // Application UUID for our own GATT client, should we need to register one
        constexpr BtdrvGattAttributeUuid GattClientAppUuid = { 0x10, { 0x4d, 0x43, 0x42, 0x4c, 0x45, 0x48, 0x4f, 0x53, 0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 } };

        constinit u8 g_client_if = InvalidClientIf;
        RadioRecovery g_radio_recovery;
        pairing::SessionHistory g_sessions;
        u32 g_hid_data_path_generation = UINT32_MAX;

        // A refused pairing leaves the stack refusing the next ones for a while, so asking again straight away
        // only keeps that going
        constexpr TimeSpan PairingRetryBackoff = TimeSpan::FromSeconds(30);
        struct PairingFailure {
            bluetooth::Address address = {};
            s64 tick = 0;
            s64 delay_ms = 5000;
        };
        PairingFailure g_pairing_failures[MaxBleControllers];

        bool PairingBackoffActive(const bluetooth::Address &address) {
            for (const auto &failure : g_pairing_failures) {
                if (failure.tick && AddressEqual(failure.address, address) &&
                    ((os::GetSystemTick() - os::Tick(failure.tick)).ToTimeSpan().GetMilliSeconds() < failure.delay_ms)) return true;
            }
            return false;
        }

        void NotePairingFailure(const bluetooth::Address &address, bool bond_refused = false) {
            auto *oldest = &g_pairing_failures[0];
            for (auto &failure : g_pairing_failures) {
                if (AddressEqual(failure.address, address)) { oldest = &failure; break; }
                if (failure.tick < oldest->tick) oldest = &failure;
            }
            *oldest = {address, os::GetSystemTick().GetInt64Value(), bond_refused ? PairingRetryBackoff.GetMilliSeconds() : 5000};
        }

        /* Multi-device slot management */

        struct BleDeviceSlot {
            enum class State : u8 {
                Vacant,
                Connecting,
                Discovering,
                Pairing,
                Connected,
                Disconnecting
            };

            State state = State::Vacant;
            u32 conn_id = InvalidConnId;
            u8 client_if = InvalidClientIf;
            u16 negotiated_max_interval = 0;
            bluetooth::Address address = {};
            gatt::HidService hid = {};
            std::shared_ptr<controller::XboxOneBleController> controller = nullptr;
        };

        constinit os::SdkMutex g_slots_lock;
        BleDeviceSlot g_host_slots[MaxBleControllers];

        size_t GetConnectedCount() {
            std::scoped_lock lk(g_slots_lock);
            size_t count = 0;
            for (size_t i = 0; i < MaxBleControllers; ++i) {
                if (g_host_slots[i].state == BleDeviceSlot::State::Connected) {
                    ++count;
                }
            }
            return count;
        }

        int FindFreeSlot() {
            std::scoped_lock lk(g_slots_lock);
            for (size_t i = 0; i < MaxBleControllers; ++i) {
                if (g_host_slots[i].state == BleDeviceSlot::State::Vacant) {
                    return static_cast<int>(i);
                }
            }
            return -1;
        }

        u32 FindSlotForAddress(const bluetooth::Address &address, u32 fallback) {
            for (u32 i = 0; i < MaxBleControllers; ++i) {
                if ((g_host_slots[i].state == BleDeviceSlot::State::Vacant) && gatt::HasCachedAddress(i, address)) return i;
            }
            return fallback;
        }

        bool IsAddressConnected(const bluetooth::Address &address, u32 exclude_slot = InvalidSlot) {
            std::scoped_lock lk(g_slots_lock);
            for (size_t i = 0; i < MaxBleControllers; ++i) {
                if (i == exclude_slot) {
                    continue;
                }
                if ((g_host_slots[i].state != BleDeviceSlot::State::Vacant) && AddressEqual(g_host_slots[i].address, address)) {
                    return true;
                }
            }
            return false;
        }

        /* LE HID event stream: bounded lookup with ownership across dispatch */

        void LeHidThreadFunc(void *) {
            Event event;
            Result rc = btdrvRegisterBleHidEvent(&event);
            log::Write("btdrvRegisterBleHidEvent rc=0x%x", rc.GetValue());
            if (R_FAILED(rc)) {
                return;
            }

            os::SystemEvent system_event;
            system_event.AttachReadableHandle(event.revent, false, os::EventClearMode_AutoClear);

            for (;;) {
                system_event.Wait();

                rc = btdrvGetLeHidEventInfo(&g_le_hid_record.info, sizeof(g_le_hid_record.info), &g_le_hid_record.type);
                if (R_FAILED(rc)) {
                    continue;
                }

                if (g_le_hid_record.type != BtdrvBleEventType_ClientNotify) {
                    // The HID data path can deliver connection lifecycle events too.
                    // Apply them to slot state, not just to the logging/GATT queue.
                    static_cast<void>(events::OnBleEvent(g_le_hid_record.type, &g_le_hid_record.info));
                    continue;
                }

                auto &notify = g_le_hid_record.info.client_notify;

                if ((notify.op_type == GattNotifyType_Notification) && (notify.result == 0) && IsUuid16(notify.char_uuid, GattCharReport)) {
                    // Keep the controller alive until dispatch finishes, including during disconnect.
                    if (auto controller = events::GetActiveController(notify.conn_id)) {
                        const u64 start_tick = profile::RecordReportStart();
                        static_cast<void>(controller->HandleInputReport(notify.data, std::min<size_t>(notify.size, sizeof(notify.data))));
                        profile::RecordReportEnd(start_tick);
                    }
                } else if (!((notify.op_type == GattNotifyType_WriteCharacteristic) && IsUuid16(notify.char_uuid, GattCharReport))) {
                    events::NoteGattResult(notify.conn_id, notify.op_type, notify.char_uuid, notify.result);
                    events::PushRecord(g_le_hid_record);
                }
            }
        }

        constexpr s32 LowLatencyThreadPriority = -10; // 1 level below mc::HidReportThread (-11) to avoid preempting Classic BT report draining
        constexpr s32 DefaultThreadPriority = 9;

        void StartLeHidListener() {
            const s32 priority = mitm::GetGlobalConfig()->ble.low_latency ? LowLatencyThreadPriority : DefaultThreadPriority;
            R_ABORT_UNLESS(os::CreateThread(&g_le_hid_thread,
                LeHidThreadFunc,
                nullptr,
                g_le_hid_thread_stack,
                LeHidThreadStackSize,
                priority
            ));

            os::SetThreadNamePointer(&g_le_hid_thread, "mc::BleHidEventThread");
            os::StartThread(&g_le_hid_thread);
        }

        /* Bring-up */

        Result RegisterGattClient() {
            events::SetRegistering(true);
            ON_SCOPE_EXIT { events::SetRegistering(false); };

            Result rc = btdrvRegisterGattClient(&GattClientAppUuid);
            log::Write("btdrvRegisterGattClient rc=0x%x", rc.GetValue());
            R_TRY(rc);

            EventRecord e;
            if (!events::WaitFor(BtdrvBleEventType_ClientRegistration, TimeSpan::FromSeconds(5), &e, [](const EventRecord &r) { return r.info.client_registration.status == 1; })) {
                R_THROW(svc::ResultTimedOut());
            }

            g_client_if = e.info.client_registration.client_if;
            R_SUCCEED();
        }

        Result PrepareClient() {
            events::TakeOwnership();
            events::WaitRadioEnabled();

            Result rc = btdrvEnableBle();
            log::Write("btdrvEnableBle rc=0x%x", rc.GetValue());
            events::Pump(TimeSpan::FromSeconds(6));

            if (events::GetAvailableClientIfCount() > 0) {
                log::Write("found %zu gatt client_ifs from btm (primary=%u)", events::GetAvailableClientIfCount(), events::GetClientIfForSlot(0));
                R_SUCCEED();
            }

            for (int attempt = 0; attempt < 3; ++attempt) {
                if (R_SUCCEEDED(RegisterGattClient())) {
                    log::Write("registered gatt client_if=%u", g_client_if);
                    R_SUCCEED();
                }
                os::SleepThread(TimeSpan::FromSeconds(2));
            }

            R_THROW(svc::ResultTimedOut());
        }

        void DismissSlot(u32 slot_index, bool close_transport = true);

        void DisconnectAll(bool close_transport = true) {
            for (size_t i = 0; i < MaxBleControllers; ++i) {
                if (g_host_slots[i].state != BleDeviceSlot::State::Vacant) {
                    DismissSlot(static_cast<u32>(i), close_transport);
                }
            }
            events::Pump(TimeSpan::FromSeconds(1));
        }

        void CleanupSleepIfNeeded() {
            events::WaitForSleepPreparation();
            if (events::TakeSleepCleanup()) {
                const bool had_links = GetConnectedCount() != 0;
                DisconnectAll(false); // the PSC thread already closed the transport
                g_radio_recovery.Sleep(had_links);
            }
        }

        void WaitForRadio() {
            CleanupSleepIfNeeded();
            if (!events::IsRadioEnabled()) {
                log::Write("BLE suspended, waiting for wake");
                events::WaitRadioEnabled();
                log::Write("BLE awake");
            }
            // Covers a complete sleep/wake transition while setup was unwinding.
            CleanupSleepIfNeeded();
            if (events::TakeRadioCycled()) {
                events::Pump(TimeSpan::FromSeconds(2));
                log::Write("radio awake: %zu GATT clients available", events::GetAvailableClientIfCount());
            }
        }

        /* Scanning and connection */

        Result ConfigureScanFilter(u32 action) {
            events::ExpectFilterEvent();
            Result rc = action == 2 ? btdrvClearBleScanFilters() : btdrvEnableBleScanFilter(action == 3);
            log::Write("scan filter request action=%u rc=0x%x", action, rc.GetValue());
            if (R_FAILED(rc)) {
                // A rejected IPC does not have an asynchronous completion.
                events::CancelExpectedFilterEvent();
                return rc;
            }
            EventRecord e = {};
            const bool completed = events::WaitFor(BtdrvBleEventType_ScanFilter, TimeSpan::FromSeconds(1), &e,
                [=](const EventRecord &r) { return r.info.scan_filter.action == action; });
            if (!completed) log::Write("scan filter completion timed out action=%u", action);
            R_UNLESS(completed, svc::ResultTimedOut());
            R_UNLESS(e.info.scan_filter.result == 0, svc::ResultInvalidState());
            R_SUCCEED();
        }

        Result Scan(bluetooth::Address *out_address, TimeSpan duration) {
            events::SetScanning(true);
            ON_SCOPE_EXIT { events::SetScanning(false); };

            // Filter and scan calls complete asynchronously. Keep ownership until
            // their completions have been drained, including when no pad is found.
            ON_SCOPE_EXIT {
                if (events::IsRadioEnabled()) {
                    Result restore_rc = ConfigureScanFilter(3);
                    if (R_FAILED(restore_rc)) log::Write("scan filter restore failed rc=0x%x", restore_rc.GetValue());
                }
            };
            R_TRY(ConfigureScanFilter(2));
            R_TRY(ConfigureScanFilter(4));
            R_UNLESS(events::IsPairingWindowOpen() && events::IsRadioEnabled(), svc::ResultCancelled());

            Result rc = btdrvStartBleScan();
            log::Write("btdrvStartBleScan rc=0x%x", rc.GetValue());
            if (R_FAILED(rc)) {
                // The observed Classic inquiry occupies the radio for 10.24 s.
                // Retrying five times per second only floods it with filter IPCs.
                events::Pump(TimeSpan::FromSeconds(1));
                return rc;
            }

            u32 advertisements = 0, rejected_transport = 0, rejected_identity = 0, unavailable = 0, failed = 0;
            bluetooth::Address logged_addresses[8] = {};
            size_t logged_count = 0;
            bool scan_complete = false;
            EventRecord e = {};
            const bool found = events::WaitFor(events::InvalidType, duration, &e, [&](const EventRecord &r) {
                if (!events::IsPairingWindowOpen() || events::HasAdoptedConnection() || !events::IsRadioEnabled()) {
                    return true;
                }
                if (r.type != BtdrvBleEventType_ScanResult) {
                    return false;
                }
                const auto &scan = r.info.scan_result;
                if (scan.status == 1) {
                    scan_complete = true;
                    return true;
                }
                if (scan.status != BleScanStatus_NewDevice) return false;
                ++advertisements;
                const auto summary = ParseAdvertisements(scan.ad_list, scan.count);
                const char *decision = "candidate";
                if (scan.result != 0) { ++failed; decision = "scan-error"; }
                else if (!IsBleAdvertisement(summary, scan.device_type)) { ++rejected_transport; decision = "classic"; }
                else if (!IsXboxCandidate(summary)) { ++rejected_identity; decision = "identity"; }
                else if (IsAddressConnected(scan.address) || PairingBackoffActive(scan.address)) { ++unavailable; decision = "active-or-backoff"; }
                const bool candidate = std::strcmp(decision, "candidate") == 0;

                // Bound diagnostic SD writes: one detail per address (up to eight
                // per scan), plus the selected candidate. No input-thread I/O.
                if (log::IsEnabled() && (candidate || (logged_count < std::size(logged_addresses) &&
                        std::none_of(logged_addresses, logged_addresses + logged_count,
                            [&](const auto &address) { return AddressEqual(address, scan.address); })))) {
                    if (logged_count < std::size(logged_addresses)) logged_addresses[logged_count++] = scan.address;
                    events::LogAddress("scan device", scan.address);
                    log::Write("  result=%u type=%u addr_type=%u rssi=%ld name='%s' appearance=0x%04x flags=0x%02x hid=%u decision=%s",
                        scan.result, scan.device_type, scan.ble_addr_type, static_cast<long>(scan.rssi), summary.name,
                        summary.appearance, summary.flags, summary.has_hid_service, decision);
                }
                return candidate;
            });

            if (!scan_complete && events::IsRadioEnabled()) {
                Result stop_rc = btdrvStopBleScan();
                log::Write("btdrvStopBleScan rc=0x%x", stop_rc.GetValue());
                if (R_SUCCEEDED(stop_rc)) {
                    scan_complete = events::WaitFor(BtdrvBleEventType_ScanResult, TimeSpan::FromSeconds(1), nullptr,
                        [](const EventRecord &r) { return r.info.scan_result.status == 1; });
                    if (!scan_complete) log::Write("scan stop completion timed out");
                }
                R_TRY(stop_rc);
                R_UNLESS(scan_complete, svc::ResultTimedOut());
            }

            log::Write("scan summary: advertisements=%u classic=%u identity=%u unavailable=%u errors=%u dropped=%u",
                advertisements, rejected_transport, rejected_identity, unavailable, failed, events::TakeRingDropped());

            R_UNLESS(events::IsPairingWindowOpen() && !events::HasAdoptedConnection() && events::IsRadioEnabled(), svc::ResultCancelled());
            R_UNLESS(found && (e.type == BtdrvBleEventType_ScanResult) &&
                (e.info.scan_result.status == BleScanStatus_NewDevice), svc::ResultTimedOut());

            *out_address = e.info.scan_result.address;
            events::LogAddress("found controller", *out_address);

            if (log::IsEnabled()) {
                SetSysBluetoothDevicesSettings settings = {};
                Result info_rc = btdrvGetPairedDeviceInfo(*out_address, &settings);
                log::Write("btdrvGetPairedDeviceInfo rc=0x%x key_present=%u key_type=%u device_type=%u",
                    info_rc.GetValue(), settings.link_key_present, settings.key_type, settings.device_type);
            }

            R_SUCCEED();
        }

        bool Adopt(bluetooth::Address *out_address, u32 *out_conn_id) {
            if (!events::TakeAdoptedConnection(out_address, out_conn_id)) {
                return false;
            }
            log::Write("controller connected on its own, conn_id=%u", *out_conn_id);
            return true;
        }

        /* Presenting and dismissing controllers */

        void RebalanceConnectionParameters() {
            const size_t connected_count = GetConnectedCount();
            if (connected_count == 0) {
                return;
            }

            const u16 target_max = events::GetTargetMaxConnectionInterval(connected_count);
            if (target_max == 0) {
                return;
            }

            for (size_t i = 0; i < MaxBleControllers; ++i) {
                if (g_host_slots[i].state == BleDeviceSlot::State::Connected) {
                    if (g_host_slots[i].negotiated_max_interval != target_max) {
                        g_host_slots[i].negotiated_max_interval = events::RequestLowLatencyConnectionParameters(g_host_slots[i].address, connected_count);
                    }
                }
            }
        }

        void PresentSlot(u32 slot_index) {
            auto &slot = g_host_slots[slot_index];
            if (!slot.hid.has_output_report) {
                log::Write("slot %u: no output Report characteristic, rumble unavailable", slot_index);
            }

            auto controller = std::make_shared<controller::XboxOneBleController>(
                slot.address, slot.conn_id, slot.hid.service.id, slot.hid.service.is_primary, slot.hid.output_report.id, slot.hid.has_output_report);

            {
                std::scoped_lock lk(g_slots_lock);
                slot.controller = controller;
                slot.state = BleDeviceSlot::State::Connected;
            }

            // Initialize settings and storage before the input thread can see this controller.
            controller::AttachVirtualController(controller);
            if (GetConnectedCount() == 1) profile::BeginSession();
            for (auto &failure : g_pairing_failures) {
                if (AddressEqual(failure.address, slot.address)) failure = {};
            }

            u8 neutral[sizeof(controller::XboxOneInputReport0x01)] = { 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80 };
            static_cast<void>(controller->HandleInputReport(neutral, sizeof(neutral)));
            events::RegisterSlotController(slot_index, slot.conn_id, controller);

            // Request low-latency parameters and rebalance active BLE slots
            slot.negotiated_max_interval = 0;
            RebalanceConnectionParameters();

            log::Write("slot %u: controller presented to system (conn_id=%u, total active=%zu)", slot_index, slot.conn_id, GetConnectedCount());
        }

        void DismissSlot(u32 slot_index, bool close_transport) {
            events::WaitForSleepPreparation();
            std::shared_ptr<controller::XboxOneBleController> controller;
            bluetooth::Address address = {};
            u32 conn_id = InvalidConnId;

            {
                std::scoped_lock lk(g_slots_lock);
                auto &slot = g_host_slots[slot_index];
                if (slot.state == BleDeviceSlot::State::Vacant) {
                    return;
                }

                slot.state = BleDeviceSlot::State::Disconnecting;
                conn_id = slot.conn_id;
                address = slot.address;

                events::UnregisterSlotController(slot_index);

                controller = std::move(slot.controller);
                slot.controller = nullptr;
                slot.conn_id = InvalidConnId;
                slot.negotiated_max_interval = 0;
            }

            if (controller) {
                // Ensure any in-flight HandleInputReport / HandleOutputDataReport on another core has finished
                controller->Disconnect();
            }

            // Cancel the client request and remove notification registrations before
            // recycling the slot. Keep event ownership until the disconnect completes.
            if (close_transport && events::IsRadioEnabled()) {
                if (conn_id != InvalidConnId) {
                    gatt::Unsubscribe(slot_index, conn_id);
                    if (!events::IsSlotDisconnected(slot_index)) {
                        Result rc = btdrvDisconnectGattServer(conn_id);
                        log::Write("btdrvDisconnectGattServer slot=%u conn_id=%u rc=0x%x", slot_index, conn_id, rc.GetValue());
                        events::WaitFor(events::InvalidType, TimeSpan::FromSeconds(2), nullptr, [&](const EventRecord &) {
                            return events::IsSlotDisconnected(slot_index) || !events::IsRadioEnabled();
                        });
                    }
                }
                const u8 client_if = g_host_slots[slot_index].client_if;
                if (client_if != InvalidClientIf) {
                    static_cast<void>(btdrvCancelConnectGattServer(client_if, address, true));
                }
            }

            gatt::ResetSlot(slot_index);
            events::ClearSlot(slot_index);
            {
                std::scoped_lock lk(g_slots_lock);
                g_host_slots[slot_index] = {};
            }
            if (controller) {
                profile::FlushNow();
                controller::DetachVirtualController(address);
                log::Write("slot %u: controller removed from system", slot_index);
            }

        }

        void ProcessPendingDisconnections() {
            bool any_disconnected = false;
            for (size_t i = 0; i < MaxBleControllers; ++i) {
                if (g_host_slots[i].state == BleDeviceSlot::State::Connected) {
                    const u32 slot_idx = static_cast<u32>(i);
                    if (events::IsSlotDisconnected(slot_idx)) {
                        log::Write("slot %u: controller disconnected (reason=0x%04x)", slot_idx, events::GetSlotDisconnectReason(slot_idx));
                        g_radio_recovery.Request();
                        DismissSlot(slot_idx);
                        any_disconnected = true;
                    } else if (gatt::TakeRediscovered(slot_idx)) {
                        log::Write("slot %u: services re-discovered, subscribing again", slot_idx);
                        gatt::HidService hid;
                        static_cast<void>(gatt::Subscribe(slot_idx, g_host_slots[i].conn_id, &hid));
                    }
                }
            }

            const bool count_changed = events::ConsumeControllerCountChanged();
            if (any_disconnected || count_changed) {
                RebalanceConnectionParameters();
            }
        }

        void ScanAndPairNextController() {
            if (events::ConsumeControllerCountChanged()) {
                RebalanceConnectionParameters();
            }

            int slot_idx = FindFreeSlot();
            if (slot_idx < 0) {
                return;
            }
            u32 slot = static_cast<u32>(slot_idx);

            bluetooth::Address address;
            u32 conn_id = InvalidConnId;
            u32 session_generation = events::GetClientRegistrationGeneration();
            bool connected = Adopt(&address, &conn_id);

            if (!connected) {
                Result rc = Scan(&address, TimeSpan::FromSeconds(5));
                if (events::ConsumeControllerCountChanged()) {
                    RebalanceConnectionParameters();
                }
                if (Adopt(&address, &conn_id)) {
                    connected = true;
                } else if (R_SUCCEEDED(rc)) {
                    if (IsAddressConnected(address, slot)) {
                        return;
                    }
                    slot = FindSlotForAddress(address, slot);
                    g_host_slots[slot].state = BleDeviceSlot::State::Connecting;
                    g_host_slots[slot].address = address;
                    const u8 client_if = events::GetClientIfForSlot(slot);
                    g_host_slots[slot].client_if = client_if;
                    session_generation = events::GetClientRegistrationGeneration();
                    connected = R_SUCCEEDED(pairing::Connect(slot, client_if, address, &conn_id, TimeSpan::FromSeconds(15), g_sessions.Contains(address, session_generation)));
                    if (!connected) {
                        NotePairingFailure(address);
                        DismissSlot(slot);
                        return;
                    }
                } else {
                    // When controllers (Joy-Cons or BLE pads) are already active, pace BLE scans so we don't
                    // monopolize the shared 2.4 GHz radio while the Change Grip/Order screen stays open.
                    if (svc::ResultTimedOut::Includes(rc) && events::IsPairingWindowOpen() && ((GetConnectedCount() > 0) || (controller::GetConnectedControllerCount() > 0))) {
                        events::Pump(TimeSpan::FromSeconds(2));
                    }
                    return;
                }
            }

            if (IsAddressConnected(address, slot)) {
                DismissSlot(slot);
                return;
            }

            if (g_host_slots[slot].state == BleDeviceSlot::State::Vacant) slot = FindSlotForAddress(address, slot);
            g_host_slots[slot].conn_id = conn_id;
            g_host_slots[slot].address = address;
            g_host_slots[slot].state = BleDeviceSlot::State::Discovering;
            if (!events::IsAddressActive(address)) {
                events::SetSlotConnecting(slot, address);
                events::SetSlotConnection(slot, address, conn_id);
            }
            if (events::IsSlotDisconnected(slot)) {
                DismissSlot(slot);
                return;
            }
            events::SetSlotClaimed(slot, true);

            Result rc = gatt::Discover(slot, conn_id, address);
            if (R_FAILED(rc)) {
                log::Write("slot %u: GATT setup failed before pairing rc=0x%x", slot, rc.GetValue());
                NotePairingFailure(address);
                DismissSlot(slot);
                return;
            }

            g_host_slots[slot].state = BleDeviceSlot::State::Pairing;
            if (session_generation != events::GetClientRegistrationGeneration()) {
                log::Write("slot %u: native clients changed during setup; discarding old session", slot);
                DismissSlot(slot, false);
                return;
            }
            if (!pairing::PrepareSecurity(slot, address, g_sessions.Contains(address, session_generation))) {
                g_radio_recovery.Request();
                NotePairingFailure(address, true);
                if (events::IsRadioEnabled()) {
                    const Result cancel_rc = btdrvCancelBond(address);
                    log::Write("btdrvCancelBond slot=%u rc=0x%x", slot, cancel_rc.GetValue());
                }
                log::Write("slot %u: backing off for %lds before trying to pair again", slot, PairingRetryBackoff.GetSeconds());
                DismissSlot(slot);
                os::SleepThread(TimeSpan::FromSeconds(1));
                return;
            }

            rc = ResultSuccess();
            const u32 generation = events::GetClientRegistrationGeneration();
            if (g_hid_data_path_generation != generation) {
                auto hid_uuid = MakeUuid16(GattServiceHid);
                rc = btdrvRegisterGattHidDataPath(&hid_uuid);
                log::Write("btdrvRegisterGattHidDataPath slot=%u rc=0x%x", slot, rc.GetValue());
                if (R_FAILED(rc)) {
                    DismissSlot(slot);
                    return;
                }
                g_hid_data_path_generation = generation;
            }
            events::ClearGattFailure(slot);
            gatt::HidService hid = {};
            rc = gatt::Subscribe(slot, conn_id, &hid);
            if (R_FAILED(rc)) {
                log::Write("slot %u: subscription failed rc=0x%x gatt_result=%u", slot, rc.GetValue(), events::GetGattFailure(slot));
                g_radio_recovery.Request();
                NotePairingFailure(address, g_sessions.Contains(address, session_generation));
                if (events::IsRadioEnabled()) {
                    const Result cancel_rc = btdrvCancelBond(address);
                    log::Write("btdrvCancelBond after subscription failure slot=%u rc=0x%x", slot, cancel_rc.GetValue());
                }
                DismissSlot(slot);
                os::SleepThread(TimeSpan::FromSeconds(1));
                return;
            }

            if (session_generation != events::GetClientRegistrationGeneration()) {
                DismissSlot(slot, false);
                return;
            }
            g_host_slots[slot].hid = hid;
            g_sessions.Remember(slot, address, session_generation);
            log::Write("slot %u: paired and subscribed", slot);
            PresentSlot(slot);
        }

        bool RecoverRadioIfNeeded() {
            static size_t reported_active_links = 0;
            const size_t active_links = events::GetActiveConnectionCount();
            const bool deferred = g_radio_recovery.Pending() && mitm::GetGlobalConfig()->ble.radio_cycle &&
                events::IsPairingWindowOpen() && events::IsRadioEnabled() && active_links != 0;
            if (deferred && active_links != reported_active_links) {
                log::Write("BLE pairing recovery deferred: %zu BLE links still active; turn all Xbox BLE pads off, then hold sync in Change Grip/Order", active_links);
            }
            reported_active_links = deferred ? active_links : 0;
            if (!g_radio_recovery.WaitingForClients()) {
                if (!g_radio_recovery.Begin(mitm::GetGlobalConfig()->ble.radio_cycle,
                        events::IsPairingWindowOpen(), events::IsRadioEnabled(), events::GetActiveConnectionCount(),
                        os::GetSystemTick().ToTimeSpan().GetMilliSeconds())) {
                    // A wake inside the reset cooldown must not spend the only
                    // zero-link recovery window connecting a pad first.
                    if (mitm::GetGlobalConfig()->ble.radio_cycle && g_radio_recovery.Pending() &&
                            events::IsPairingWindowOpen() && events::IsRadioEnabled() && events::GetActiveConnectionCount() == 0) {
                        events::Pump(TimeSpan::FromMilliSeconds(100));
                        return false;
                    }
                    return true;
                }

                // No active BLE link is allowed through the recovery gate.
                log::Write("BLE pairing recovery: no active BLE links, resetting stack");

                const u32 generation = events::GetClientRegistrationGeneration();
                Result rc = btdrvEmulateBluetoothCrash(BtdrvFatalReason_Unknown1);
                log::Write("BLE pairing recovery: radio cycle rc=0x%x", rc.GetValue());
                if (R_FAILED(rc)) return true;
                g_radio_recovery.AwaitClients(generation);
                g_hid_data_path_generation = UINT32_MAX;
            }

            // A timeout must not resume scanning with stale client handles. Keep
            // waiting on later coordinator passes without issuing another reset.
            events::WaitFor(events::InvalidType, TimeSpan::FromSeconds(8), nullptr, [&](const EventRecord &) {
                return !events::IsRadioEnabled() || g_radio_recovery.AcceptClients(
                    events::GetClientRegistrationGeneration(), events::GetAvailableClientIfCount());
            });
            if (g_radio_recovery.WaitingForClients() || !events::IsRadioEnabled()) {
                log::Write("BLE pairing recovery: still waiting for fresh GATT clients");
                return false;
            }

            for (auto &failure : g_pairing_failures) failure = {};
            log::Write("BLE pairing recovery: new GATT clients available; hold Xbox sync buttons to pair again");
            // Give Classic controllers time to reconnect before BLE scanning
            // competes with their connection setup on the shared stack.
            events::Pump(TimeSpan::FromSeconds(12));
            return true;
        }

        /* Coordinator loop */

        void HostThreadFunc(void *) {
            mitm::WaitInitialized();
            bluetooth::ble::WaitInitialized();

            os::SleepThread(TimeSpan::FromSeconds(20));
            log::Write("starting multi-device BLE host (reconnect-fix-r7, HOS=0x%x)", static_cast<u32>(hos::GetVersion()));

            if (R_FAILED(PrepareClient())) {
                log::Write("giving up: no gatt client available");
                return;
            }

            StartLeHidListener();
            events::SetSink(gatt::OnEvent);

            for (;;) {
                WaitForRadio();
                ProcessPendingDisconnections();
                profile::FlushIfDue();
                if (!RecoverRadioIfNeeded()) continue;

                if ((events::IsPairingWindowOpen() || events::HasAdoptedConnection()) && events::IsRadioEnabled() && (GetConnectedCount() < MaxBleControllers)) {
                    ScanAndPairNextController();
                } else {
                    events::Pump(TimeSpan::FromMilliSeconds(100));
                }
            }
        }

    }

    bool IsEnabled() {
        return mitm::GetGlobalConfig()->ble.enable && (hos::GetVersion() >= hos::Version_13_0_0);
    }

    void Launch() {
        if (!IsEnabled()) {
            return;
        }

        auto config = mitm::GetGlobalConfig();
        if (config->ble.log) {
            log::Initialize();
        }
        if (config->ble.profile) {
            profile::Initialize();
        }
        events::Activate();

        R_ABORT_UNLESS(os::CreateThread(&g_thread,
            HostThreadFunc,
            nullptr,
            g_thread_stack,
            ThreadStackSize,
            ThreadPriority
        ));

        os::SetThreadNamePointer(&g_thread, "mc::BleHidHostThread");
        os::StartThread(&g_thread);

        g_launched = true;
    }

    void WaitFinished() {
        if (g_launched) {
            os::WaitThread(&g_thread);
        }
    }

}
