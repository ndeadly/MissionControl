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

        // Transport argument to btdrvCreateBond (BTA convention: 1 = BR/EDR, 2 = LE)
        constexpr u32 BondTransport_Le = 2;

        constinit u8 g_client_if = InvalidClientIf;

        // A refused pairing leaves the stack refusing the next ones for a while, so asking again straight away
        // only keeps that going
        constexpr TimeSpan PairingRetryBackoff = TimeSpan::FromSeconds(30);
        constinit s64 g_pairing_failed_tick = 0;

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
            s64 last_active_tick = 0;
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

        /* Advertisement parsing */

        struct AdvertisementSummary {
            char name[0x20];
            bool has_appearance;
            u16 appearance;
        };

        void ParseAdvertisements(const BtdrvBleAdvertisement *ad_list, u8 count, AdvertisementSummary *out) {
            std::memset(out, 0, sizeof(*out));

            count = std::min<u8>(count, 10);
            for (u8 i = 0; i < count; ++i) {
                auto &ad = ad_list[i];
                size_t len = std::min<size_t>(ad.size, sizeof(ad.data));

                switch (ad.type) {
                    case BtdrvAdType_ShortenedLocalName:
                    case BtdrvAdType_CompleteLocalName: {
                        size_t n = std::min(len, sizeof(out->name) - 1);
                        std::memcpy(out->name, ad.data, n);
                        out->name[n] = '\0';
                        break;
                    }
                    case BtdrvAdType_Appearance:
                        if (len >= 2) {
                            out->has_appearance = true;
                            out->appearance = ad.data[0] | (ad.data[1] << 8);
                        }
                        break;
                    default:
                        break;
                }
            }
        }

        bool IsXboxController(const AdvertisementSummary &summary) {
            return (std::strncmp(summary.name, "Xbox", 4) == 0) || (summary.has_appearance && (summary.appearance == AppearanceGamepad));
        }

        /* LE HID event stream: lock-free demultiplexing via per-slot fast routing table */

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
                    events::PushRecord(g_le_hid_record);
                    continue;
                }

                auto &notify = g_le_hid_record.info.client_notify;

                if ((notify.type == GattNotifyType_Notification) && IsUuid16(notify.char_uuid, GattCharReport)) {
                    // Lock-free O(1) routing via per-slot atomic lookup table (supports any 32-bit conn_id)
                    if (auto *controller = events::GetActiveController(notify.conn_id)) {
                        const u64 start_tick = profile::RecordReportStart();
                        static_cast<void>(controller->HandleInputReport(notify.data, std::min<size_t>(notify.size, sizeof(notify.data))));
                        profile::RecordReportEnd(start_tick);
                    }
                } else if (!((notify.type == GattNotifyType_WriteCharacteristic) && IsUuid16(notify.char_uuid, GattCharReport))) {
                    events::NoteGattResult(notify.conn_id, notify.type, notify.char_uuid, notify.result);
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

        void DismissSlot(u32 slot_index);

        void DisconnectAll() {
            for (size_t i = 0; i < MaxBleControllers; ++i) {
                if (g_host_slots[i].state != BleDeviceSlot::State::Vacant) {
                    DismissSlot(static_cast<u32>(i));
                }
            }
            events::Pump(TimeSpan::FromSeconds(1));
        }

        void WaitForRadio() {
            if (!events::IsRadioEnabled()) {
                log::Write("ble switched off by the system, disconnecting all");
                DisconnectAll();
                events::WaitRadioEnabled();
                log::Write("ble switched back on");
            }

            if (events::TakeRadioCycled()) {
                events::Pump(TimeSpan::FromSeconds(2));
                if (events::GetAvailableClientIfCount() > 0) {
                    log::Write("radio cycled: %zu gatt client_ifs available (slot 0 uses %u)", events::GetAvailableClientIfCount(), events::GetClientIfForSlot(0));
                }
            }
        }

        /* Scanning and connection */

        Result Scan(bluetooth::Address *out_address, TimeSpan duration) {
            events::SetScanning(true);
            ON_SCOPE_EXIT { events::SetScanning(false); };

            events::ExpectFilterEvent();
            static_cast<void>(btdrvClearBleScanFilters());
            events::ExpectFilterEvent();
            static_cast<void>(btdrvEnableBleScanFilter(false));

            Result rc = btdrvStartBleScan();
            if (R_FAILED(rc)) {
                log::Write("btdrvStartBleScan rc=0x%x", rc.GetValue());
                events::ExpectFilterEvent();
                static_cast<void>(btdrvEnableBleScanFilter(true));
                // btm's classic inquiry burst occupies the radio for ~10.24s (rc=0x2a871); back off before retrying
                events::Pump(TimeSpan::FromSeconds(2));
                return rc;
            }

            EventRecord e = {};
            const bool found = events::WaitFor(events::InvalidType, duration, &e, [](const EventRecord &r) {
                if (!events::IsPairingWindowOpen() || events::HasAdoptedConnection() || !events::IsRadioEnabled()) {
                    return true;
                }
                if (r.type != BtdrvBleEventType_ScanResult) {
                    return false;
                }
                if (r.info.scan_result.status != BleScanStatus_NewDevice) {
                    return false;
                }
                if (IsAddressConnected(r.info.scan_result.address)) {
                    return false;
                }
                AdvertisementSummary summary;
                ParseAdvertisements(r.info.scan_result.ad_list, r.info.scan_result.count, &summary);
                return IsXboxController(summary);
            });

            static_cast<void>(btdrvStopBleScan());
            events::ExpectFilterEvent();
            static_cast<void>(btdrvEnableBleScanFilter(true));

            events::Pump(TimeSpan::FromSeconds(1));

            R_UNLESS(events::IsPairingWindowOpen() && !events::HasAdoptedConnection() && events::IsRadioEnabled(), svc::ResultCancelled());
            R_UNLESS(found && (e.type == BtdrvBleEventType_ScanResult), svc::ResultTimedOut());

            *out_address = e.info.scan_result.address;
            events::LogAddress("found controller", *out_address);

            if (log::IsEnabled()) {
                SetSysBluetoothDevicesSettings settings;
                Result info_rc = btdrvGetPairedDeviceInfo(*out_address, &settings);
                log::Write("btdrvGetPairedDeviceInfo rc=0x%x key_present=%u key_type=%u device_type=%u",
                    info_rc.GetValue(), settings.link_key_present, settings.key_type, settings.device_type);
            }

            R_SUCCEED();
        }

        Result WaitForConnection(u32 slot, const bluetooth::Address &address, u32 *out_conn_id, TimeSpan timeout) {
            EventRecord e;
            if (!events::WaitFor(BtdrvBleEventType_ClientConnection, timeout, &e, [&](const EventRecord &r) {
                return AddressEqual(r.info.client_connection.address, address);
            })) {
                R_THROW(svc::ResultTimedOut());
            }

            if ((e.info.client_connection.result != 0) || (e.info.client_connection.status != BleConnectionStatus_Connected)) {
                log::Write("slot %u: connection failed result=%u status=%u", slot, e.info.client_connection.result, e.info.client_connection.status);
                R_THROW(ResultNoConnection());
            }

            *out_conn_id = e.info.client_connection.conn_id;
            events::SetSlotConnection(slot, address, *out_conn_id);
            log::Write("slot %u: connected conn_id=%u", slot, *out_conn_id);
            R_SUCCEED();
        }

        Result Connect(u32 slot, const bluetooth::Address &address, u32 *out_conn_id, TimeSpan timeout) {
            events::SetSlotConnecting(slot, address);

            const u8 client_if = events::GetClientIfForSlot(slot);
            if (client_if == InvalidClientIf) {
                log::Write("slot %u: no client_if available", slot);
                return ResultNoConnection();
            }
            g_host_slots[slot].client_if = client_if;

            Result rc = btdrvRemoveBond(address);
            log::Write("btdrvRemoveBond slot=%u rc=0x%x", slot, rc.GetValue());

            rc = btdrvConnectGattServer(client_if, address, true, 0);
            log::Write("btdrvConnectGattServer slot=%u client_if=%u rc=0x%x", slot, client_if, rc.GetValue());
            R_TRY(rc);

            rc = WaitForConnection(slot, address, out_conn_id, timeout);
            if (R_FAILED(rc)) {
                static_cast<void>(btdrvCancelConnectGattServer(client_if, address, true));
            }
            R_RETURN(rc);
        }

        bool PairingBackoffActive() {
            if (g_pairing_failed_tick == 0) {
                return false;
            }
            return (os::GetSystemTick() - os::Tick(g_pairing_failed_tick)).ToTimeSpan() < PairingRetryBackoff;
        }

        bool Adopt(bluetooth::Address *out_address, u32 *out_conn_id) {
            if (!events::TakeAdoptedConnection(out_address, out_conn_id)) {
                return false;
            }
            log::Write("controller connected on its own, conn_id=%u", *out_conn_id);
            return true;
        }

        bool Pair(u32 slot, const bluetooth::Address &address) {
            Result rc = btdrvCreateBond(address, BondTransport_Le);
            log::Write("btdrvCreateBond slot=%u rc=0x%x", slot, rc.GetValue());
            if (R_FAILED(rc)) {
                return false;
            }

            const auto deadline = os::GetSystemTick().ToTimeSpan() + TimeSpan::FromSeconds(10);
            while (os::GetSystemTick().ToTimeSpan() < deadline) {
                events::Pump(TimeSpan::FromMilliSeconds(200));
                if (events::IsSlotDisconnected(slot)) {
                    break;
                }
                if (events::TakeSlotBondComplete(slot)) {
                    events::Pump(TimeSpan::FromSeconds(1));
                    if (!events::IsSlotDisconnected(slot)) {
                        log::Write("slot %u: paired", slot);
                        return true;
                    }
                    break;
                }
            }
            log::Write("slot %u: pairing failed", slot);
            return false;
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

            // Register into lock-free per-slot lookup table for O(1) input routing
            events::RegisterSlotController(slot_index, slot.conn_id, controller.get());

            controller::AttachVirtualController(controller);
            g_pairing_failed_tick = 0;

            u8 neutral[sizeof(controller::XboxOneInputReport0x01)] = { 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80 };
            static_cast<void>(controller->HandleInputReport(neutral, sizeof(neutral)));

            // Request low-latency parameters and rebalance active BLE slots
            slot.negotiated_max_interval = 0;
            RebalanceConnectionParameters();

            log::Write("slot %u: controller presented to system (conn_id=%u, total active=%zu)", slot_index, slot.conn_id, GetConnectedCount());
        }

        void DismissSlot(u32 slot_index) {
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
                controller->WaitIdle();
            }

            const bool was_disconnected = events::IsSlotDisconnected(slot_index);

            gatt::ResetSlot(slot_index);
            events::ClearSlot(slot_index);

            {
                std::scoped_lock lk(g_slots_lock);
                g_host_slots[slot_index].state = BleDeviceSlot::State::Vacant;
            }

            if (controller) {
                profile::FlushNow();
                controller::DetachVirtualController(address);
                log::Write("slot %u: controller removed from system", slot_index);
            }

            if ((conn_id != InvalidConnId) && !was_disconnected && events::IsRadioEnabled()) {
                static_cast<void>(btdrvDisconnectGattServer(conn_id));
            }
        }

        void ProcessPendingDisconnections() {
            bool any_disconnected = false;
            for (size_t i = 0; i < MaxBleControllers; ++i) {
                if (g_host_slots[i].state == BleDeviceSlot::State::Connected) {
                    const u32 slot_idx = static_cast<u32>(i);
                    if (events::IsSlotDisconnected(slot_idx)) {
                        log::Write("slot %u: controller disconnected (reason=0x%04x)", slot_idx, events::GetSlotDisconnectReason(slot_idx));
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
            const u32 slot = static_cast<u32>(slot_idx);

            bluetooth::Address address;
            u32 conn_id = InvalidConnId;
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
                    g_host_slots[slot].state = BleDeviceSlot::State::Connecting;
                    g_host_slots[slot].address = address;
                    connected = R_SUCCEEDED(Connect(slot, address, &conn_id, TimeSpan::FromSeconds(15)));
                    if (!connected) {
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

            g_host_slots[slot].conn_id = conn_id;
            g_host_slots[slot].address = address;
            g_host_slots[slot].state = BleDeviceSlot::State::Discovering;
            events::SetSlotConnection(slot, address, conn_id);
            events::SetSlotClaimed(slot, true);

            gatt::Discover(slot, conn_id, address);

            g_host_slots[slot].state = BleDeviceSlot::State::Pairing;
            if (!Pair(slot, address)) {
                g_pairing_failed_tick = os::GetSystemTick().GetInt64Value();
                log::Write("slot %u: backing off for %lds before trying to pair again", slot, PairingRetryBackoff.GetSeconds());
                DismissSlot(slot);
                os::SleepThread(TimeSpan::FromSeconds(1));
                return;
            }

            auto hid_uuid = MakeUuid16(GattServiceHid);
            Result rc = btdrvRegisterGattHidDataPath(&hid_uuid);
            log::Write("btdrvRegisterGattHidDataPath slot=%u rc=0x%x", slot, rc.GetValue());

            events::ClearGattFailure(slot);
            gatt::HidService hid;
            rc = gatt::Subscribe(slot, conn_id, &hid);
            if (R_FAILED(rc)) {
                log::Write("slot %u: subscription failed rc=0x%x gatt_result=%u", slot, rc.GetValue(), events::GetGattFailure(slot));
                DismissSlot(slot);
                os::SleepThread(TimeSpan::FromSeconds(1));
                return;
            }

            g_host_slots[slot].hid = hid;
            PresentSlot(slot);
        }

        /* Coordinator loop */

        void HostThreadFunc(void *) {
            mitm::WaitInitialized();
            bluetooth::ble::WaitInitialized();

            os::SleepThread(TimeSpan::FromSeconds(20));
            log::Write("starting multi-device BLE host");

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

                if (events::IsPairingWindowOpen() && events::IsRadioEnabled() && !PairingBackoffActive() && (GetConnectedCount() < MaxBleControllers)) {
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
