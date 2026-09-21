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

        /* The controller currently presented to the system */
        constinit os::SdkMutex g_controller_lock;
        std::shared_ptr<controller::XboxOneBleController> g_controller;

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

        // The name only travels in the scan response, which isn't always reported
        bool IsXboxController(const AdvertisementSummary &summary) {
            return (std::strncmp(summary.name, "Xbox", 4) == 0) || (summary.has_appearance && (summary.appearance == AppearanceGamepad));
        }

        /* LE HID event stream: notifications from services registered with RegisterGattHidDataPath, and the results
           of our GATT requests on them, arrive here rather than on the managed BLE event */

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
                    log::Write("btdrvGetLeHidEventInfo rc=0x%x", rc.GetValue());
                    continue;
                }

                auto &notify = g_le_hid_record.info.client_notify;
                if ((g_le_hid_record.type != BtdrvBleEventType_ClientNotify) || (notify.conn_id != events::GetConnection())) {
                    events::Log(g_le_hid_record);
                    continue;
                }

                if ((notify.type == GattNotifyType_Notification) && IsUuid16(notify.char_uuid, GattCharReport)) {
                    std::shared_ptr<controller::XboxOneBleController> controller;
                    {
                        std::scoped_lock lk(g_controller_lock);
                        controller = g_controller;
                    }
                    if (controller) {
                        static_cast<void>(controller->HandleInputReport(notify.data, std::min<size_t>(notify.size, sizeof(notify.data))));
                    }
                } else {
                    events::NoteGattResult(notify.result);
                    events::Log(g_le_hid_record);
                }
            }
        }

        void StartLeHidListener() {
            R_ABORT_UNLESS(os::CreateThread(&g_le_hid_thread,
                LeHidThreadFunc,
                nullptr,
                g_le_hid_thread_stack,
                LeHidThreadStackSize,
                ThreadPriority
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

            // btm leaves BLE disabled on a retail console. Enabling it here makes btdrv raise a BluetoothCrash
            // event, which btm answers by cycling the radio and bringing BLE up itself, registering four GATT
            // clients on the way; scans only return results after that, so the event is deliberately forwarded.
            Result rc = btdrvEnableBle();
            log::Write("btdrvEnableBle rc=0x%x", rc.GetValue());
            events::Pump(TimeSpan::FromSeconds(6));

            // btm's registrations take most of the stack's client slots; borrow the last one rather than risk
            // finding none free
            g_client_if = events::GetSpareClientIf();
            if (g_client_if != InvalidClientIf) {
                log::Write("using gatt client_if=%u", g_client_if);
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

        // Blocks while the system has BLE switched off (sleep): any radio activity then wakes the console. A wake
        // re-registers btm's clients, so ours is picked up again afterwards.
        void WaitForRadio() {
            if (!events::IsRadioEnabled()) {
                log::Write("ble switched off by the system, waiting");
                events::WaitRadioEnabled();
                log::Write("ble switched back on");
            }

            if (events::TakeRadioCycled()) {
                events::Pump(TimeSpan::FromSeconds(2));
                if (u8 spare = events::GetSpareClientIf(); spare != InvalidClientIf) {
                    g_client_if = spare;
                }
                log::Write("using gatt client_if=%u", g_client_if);
            }
        }

        /* Finding and connecting the controller */

        Result Scan(bluetooth::Address *out_address, TimeSpan duration) {
            events::SetScanning(true);
            ON_SCOPE_EXIT { events::SetScanning(false); };

            // Nothing is reported while btm's scan filter conditions are installed; the system itself clears
            // them whenever the HOME menu is left, so nothing depends on them
            events::ExpectFilterEvent();
            static_cast<void>(btdrvClearBleScanFilters());
            events::ExpectFilterEvent();
            static_cast<void>(btdrvEnableBleScanFilter(false));

            Result rc = btdrvStartBleScan();
            if (R_FAILED(rc)) {
                log::Write("btdrvStartBleScan rc=0x%x", rc.GetValue());
                events::Pump(TimeSpan::FromSeconds(1));
                return rc;
            }

            EventRecord e;
            // Ends early if a link came up by itself or the console is going to sleep
            const bool found = events::WaitFor(events::InvalidType, duration, &e, [](const EventRecord &r) {
                if (r.type != BtdrvBleEventType_ScanResult) {
                    return events::HasAdoptedConnection() || !events::IsRadioEnabled();
                }
                if (r.info.scan_result.status != BleScanStatus_NewDevice) {
                    return false;
                }
                AdvertisementSummary summary;
                ParseAdvertisements(r.info.scan_result.ad_list, r.info.scan_result.count, &summary);
                return IsXboxController(summary);
            });

            static_cast<void>(btdrvStopBleScan());
            events::ExpectFilterEvent();
            static_cast<void>(btdrvEnableBleScanFilter(true));

            // The scan-complete and filter events trail the calls above
            events::Pump(TimeSpan::FromSeconds(1));

            R_UNLESS(!events::HasAdoptedConnection() && events::IsRadioEnabled(), svc::ResultCancelled());
            R_UNLESS(found, svc::ResultTimedOut());

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

        Result WaitForConnection(const bluetooth::Address &address, u32 *out_conn_id, TimeSpan timeout) {
            EventRecord e;
            if (!events::WaitFor(BtdrvBleEventType_ClientConnection, timeout, &e, [&](const EventRecord &r) {
                return AddressEqual(r.info.client_connection.address, address) || (r.info.client_connection.result != 0);
            })) {
                R_THROW(svc::ResultTimedOut());
            }

            if ((e.info.client_connection.result != 0) || (e.info.client_connection.status != BleConnectionStatus_Connected)) {
                log::Write("connection failed result=%u status=%u", e.info.client_connection.result, e.info.client_connection.status);
                R_THROW(ResultNoConnection());
            }

            *out_conn_id = e.info.client_connection.conn_id;
            events::SetConnection(*out_conn_id);
            log::Write("connected conn_id=%u", *out_conn_id);
            R_SUCCEED();
        }

        Result Connect(const bluetooth::Address &address, u32 *out_conn_id, TimeSpan timeout) {
            events::SetTarget(&address);

            // A record left for this address, including the one a classic firmware left behind, is encrypted
            // with the moment the link comes up, and the controller drops the link over a key it doesn't hold
            Result rc = btdrvRemoveBond(address);
            log::Write("btdrvRemoveBond rc=0x%x", rc.GetValue());

            rc = btdrvConnectGattServer(g_client_if, address, true, 0);
            log::Write("btdrvConnectGattServer rc=0x%x", rc.GetValue());
            R_TRY(rc);

            rc = WaitForConnection(address, out_conn_id, timeout);
            if (R_FAILED(rc)) {
                static_cast<void>(btdrvCancelConnectGattServer(g_client_if, address, true));
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

        // The stack never pairs on its own: an authenticated request on an unbonded link just fails. Pairing is
        // Just Works and needs no confirmation from the controller.
        bool Pair(const bluetooth::Address &address) {
            Result rc = btdrvCreateBond(address, BondTransport_Le);
            log::Write("btdrvCreateBond rc=0x%x", rc.GetValue());
            if (R_FAILED(rc)) {
                return false;
            }

            // The link-state event that reports the outcome is the same for success (~5 s) and refusal (at once,
            // followed by the stack dropping the link)
            const auto deadline = os::GetSystemTick().ToTimeSpan() + TimeSpan::FromSeconds(10);
            while (os::GetSystemTick().ToTimeSpan() < deadline) {
                events::Pump(TimeSpan::FromMilliSeconds(200));
                if (events::IsDisconnected()) {
                    break;
                }
                if (events::TakeBondComplete()) {
                    events::Pump(TimeSpan::FromSeconds(1));
                    if (!events::IsDisconnected()) {
                        log::Write("paired");
                        return true;
                    }
                    break;
                }
            }
            log::Write("pairing failed");
            return false;
        }

        /* Presenting the controller to the system */

        void Present(u32 conn_id, const bluetooth::Address &address, const gatt::HidService &hid) {
            if (!hid.has_output_report) {
                log::Write("no output Report characteristic, rumble unavailable");
            }

            auto controller = std::make_shared<controller::XboxOneBleController>(address, conn_id, hid.service.id, hid.service.is_primary, hid.output_report.id);
            {
                std::scoped_lock lk(g_controller_lock);
                g_controller = controller;
            }
            controller::AttachVirtualController(controller);
            g_pairing_failed_tick = 0;

            // hid registers a controller on the first input report, and the pad only reports when something is
            // touched, so its resting state is delivered once here (sticks centred, nothing pressed)
            u8 neutral[sizeof(controller::XboxOneInputReport0x01)] = { 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80 };
            static_cast<void>(controller->HandleInputReport(neutral, sizeof(neutral)));

            log::Write("controller presented to the system");
        }

        void Dismiss(const bluetooth::Address &address) {
            std::shared_ptr<controller::XboxOneBleController> controller;
            {
                std::scoped_lock lk(g_controller_lock);
                controller = std::move(g_controller);
                g_controller = nullptr;
            }

            if (controller) {
                controller::DetachVirtualController(address);
                log::Write("controller removed from the system");
            }
        }

        void Disconnect() {
            const u32 conn_id = events::GetConnection();

            bluetooth::Address address;
            if (events::GetTarget(&address)) {
                Dismiss(address);
            }

            // Nothing may touch the radio while the system has BLE switched off: it wakes the console
            if ((conn_id != InvalidConnId) && !events::IsDisconnected() && events::IsRadioEnabled()) {
                static_cast<void>(btdrvDisconnectGattServer(conn_id));
            }

            // Keep the target until the stack has finished tearing the link down
            events::Pump(TimeSpan::FromSeconds(1));

            events::SetConnection(InvalidConnId);
            events::SetTarget(nullptr);
            events::SetClaimedAddress(nullptr);
        }

        void RunConnected(u32 conn_id) {
            for (;;) {
                events::Pump(TimeSpan::FromSeconds(1));

                if (gatt::TakeRediscovered()) {
                    log::Write("services re-discovered, subscribing again");
                    gatt::HidService hid;
                    static_cast<void>(gatt::Subscribe(conn_id, &hid));
                }

                if (events::IsDisconnected()) {
                    log::Write("controller disconnected reason=0x%04x", events::GetDisconnectReason());
                    return;
                }

                if (!events::IsRadioEnabled()) {
                    return;
                }
            }
        }

        /*
         * A controller is paired while the Controllers screen has the system searching, the same window in which
         * a Pro Controller pairs, and used until its link drops. Every connection is a fresh pairing: the console's
         * stack (Broadcom BSA 1.6) only knows LE legacy pairing and these pads keep a bond only from LE Secure
         * Connections, so a pad that reconnected on its own would refuse the stored key anyway.
         */
        void HostThreadFunc(void *) {
            mitm::WaitInitialized();
            bluetooth::ble::WaitInitialized();

            // Bringing BLE up cycles the radio (see PrepareClient); let the system finish its own bring-up first
            os::SleepThread(TimeSpan::FromSeconds(20));
            log::Write("starting");

            if (R_FAILED(PrepareClient())) {
                log::Write("giving up: no gatt client available");
                return;
            }

            StartLeHidListener();
            events::SetSink(gatt::OnEvent);

            for (;;) {
                WaitForRadio();

                bluetooth::Address address;
                u32 conn_id = InvalidConnId;

                bool connected = Adopt(&address, &conn_id);
                if (!connected && events::IsPairingWindowOpen() && events::IsRadioEnabled() && !PairingBackoffActive()) {
                    Result rc = Scan(&address, TimeSpan::FromSeconds(10));
                    if (Adopt(&address, &conn_id)) {
                        connected = true;
                    } else if (R_SUCCEEDED(rc)) {
                        connected = R_SUCCEEDED(Connect(address, &conn_id, TimeSpan::FromSeconds(15)));
                        if (!connected) {
                            Disconnect();
                        }
                    }
                }

                if (!connected) {
                    os::SleepThread(TimeSpan::FromSeconds(2));
                    continue;
                }

                // Pairing traffic for this pad is answered on its behalf from here on
                events::SetClaimedAddress(&address);

                gatt::Discover(conn_id, address);

                if (!Pair(address)) {
                    g_pairing_failed_tick = os::GetSystemTick().GetInt64Value();
                    log::Write("backing off for %lds before trying to pair again", PairingRetryBackoff.GetSeconds());
                    Disconnect();
                    os::SleepThread(TimeSpan::FromSeconds(1));
                    continue;
                }

                // Hands the HID service to the stack's HID host layer for this link: its notifications, and the results
                // of our requests on it, then arrive on the LE HID event stream
                auto hid_uuid = MakeUuid16(GattServiceHid);
                Result rc = btdrvRegisterGattHidDataPath(&hid_uuid);
                log::Write("btdrvRegisterGattHidDataPath rc=0x%x", rc.GetValue());

                // The subscription is judged by the CCCD write's result; the pad only reports on input
                events::ClearGattFailure();
                gatt::HidService hid;
                rc = gatt::Subscribe(conn_id, &hid);
                if (R_FAILED(rc)) {
                    log::Write("subscription failed rc=0x%x gatt_result=%u", rc.GetValue(), events::GetGattFailure());
                    Disconnect();
                    os::SleepThread(TimeSpan::FromSeconds(1));
                    continue;
                }

                Present(conn_id, address, hid);
                RunConnected(conn_id);

                // Nothing beyond the teardown is asked of the stack once the link is gone: removing the bond and
                // detaching the HID host layer here were both tried and changed nothing
                Disconnect();
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
