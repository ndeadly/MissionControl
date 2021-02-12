/*
 * Copyright (c) 2020-2021 ndeadly
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
#include "bluetooth_hid.hpp"
#include "../btdrv_mitm_flags.hpp"
#include "../../controllers/controller_management.hpp"
#include <mutex>
#include <cstring>

namespace ams::bluetooth::hid {

    namespace {

        os::Mutex g_event_info_lock(false);
        bluetooth::HidEventInfo g_event_info;
        bluetooth::HidEventType g_current_event_type;

        os::SystemEvent g_system_event;
        os::SystemEvent g_system_event_fwd(os::EventClearMode_AutoClear, true);
        os::SystemEvent g_system_event_user_fwd(os::EventClearMode_AutoClear, true);

        os::Event g_init_event(os::EventClearMode_ManualClear);
        os::Event g_data_read_event(os::EventClearMode_AutoClear);

    }

    bool IsInitialized() {
        return g_init_event.TryWait();
    }

    void SignalInitialized(void) {
        g_init_event.Signal();
    }

    void WaitInitialized(void) {
        g_init_event.Wait();
    }

    os::SystemEvent *GetSystemEvent(void) {
        return &g_system_event;
    }

    os::SystemEvent *GetForwardEvent(void) {
        return &g_system_event_fwd;
    }

    os::SystemEvent *GetUserForwardEvent(void) {
        return &g_system_event_user_fwd;
    }
	
	void SignalFakeEvent(bluetooth::HidEventType type, const void *data, size_t size) {
        g_current_event_type = type;
        std::memcpy(&g_event_info, data, size);

        g_system_event_fwd.Signal();
    }

    Result VirtualReconnect(const bluetooth::Address *address) {
        auto event_info = reinterpret_cast<HidEventInfo *>(g_event_info_buffer);
        event_info->connection_state.address = *address;

        //g_redirect_hid_report_events = true;

        // Signal fake disconnection event
        g_current_event_type = BtdrvHidEventType_ConnectionState;
        event_info->connection_state.state = BtdrvHidConnectionState_Disconnected;
        os::SignalSystemEvent(&g_system_event_fwd);
        os::WaitEvent(&g_data_read_event);

        // If we don't wait a bit the console disconnects the controller for real
        svcSleepThread(100'000'000ULL);

        // Signal fake connection event
        g_current_event_type = BtdrvHidEventType_ConnectionState;
        event_info->connection_state.state = BtdrvHidConnectionState_Connected;
        os::SignalSystemEvent(&g_system_event_fwd);
        os::WaitEvent(&g_data_read_event);

        //g_redirect_hid_report_events = false;

        return ams::ResultSuccess();
    }

    Result GetEventInfo(bluetooth::HidEventType *type, void *buffer, size_t size) {
        std::scoped_lock lk(g_event_info_lock);

        *type = g_current_event_type;
        std::memcpy(buffer, &g_event_info, size);

        g_data_read_event.Signal();

        return ams::ResultSuccess();
    }

    inline void HandleConnectionStateEventV1(bluetooth::HidEventInfo *event_info) {
        switch (event_info->connection.v1.status) {
            case BtdrvHidConnectionStatusOld_Opened:
                controller::AttachHandler(&event_info->connection.v1.addr);
                break;
            case BtdrvHidConnectionStatusOld_Closed:
                controller::RemoveHandler(&event_info->connection.v1.addr);
                break;
            default:
                break;
        }
    }

    inline void HandleConnectionStateEventV12(bluetooth::HidEventInfo *event_info) {
        switch (event_info->connection.v12.status) {
            case BtdrvHidConnectionStatus_Opened:
                controller::AttachHandler(&event_info->connection.v12.addr);
                break;
            case BtdrvHidConnectionStatus_Closed:
                controller::RemoveHandler(&event_info->connection.v12.addr);
                break;
            default:
                break;
        }
    }

    void HandleEvent(void) {
        {
            std::scoped_lock lk(g_event_info_lock);
            R_ABORT_UNLESS(btdrvGetHidEventInfo(&g_event_info, sizeof(bluetooth::HidEventInfo), &g_current_event_type));
        }

        switch (g_current_event_type) {
            case BtdrvHidEventType_Connection:
                hos::GetVersion() < hos::Version_12_0_0 ? HandleConnectionStateEventV1(&g_event_info) : HandleConnectionStateEventV12(&g_event_info);
                break;
            default:
                break;
        }

        g_system_event_fwd.Signal();
        g_data_read_event.Wait();

        if (g_system_event_user_fwd.GetBase()->state) {
            g_system_event_user_fwd.Signal();
        }
    }

}
