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

    Result GetEventInfo(bluetooth::HidEventType *type, void *buffer, size_t size) {
        std::scoped_lock lk(g_event_info_lock);

        *type = g_current_event_type;
        std::memcpy(buffer, &g_event_info, size);

        g_data_read_event.Signal();

        return ams::ResultSuccess();
    }

    void SignalFakeEvent(bluetooth::HidEventType type, const void *data, size_t size) {
        g_current_event_type = type;
        std::memcpy(&g_event_info, data, size);

        g_system_event_fwd.Signal();
    }

    void HandleConnectionStateEvent(bluetooth::HidEventInfo *event_info) {
        switch (event_info->connection_state.state) {
            case BtdrvHidConnectionState_Connected:
                controller::AttachHandler(&event_info->connection_state.address);
                break;
            case BtdrvHidConnectionState_Disconnected:
                controller::RemoveHandler(&event_info->connection_state.address);
                break;
            default:
                break;
        }
    }

    // void HandleUnknown07Event(bluetooth::HidEventInfo *event_info) {
    //     ;
    // }

    void HandleEvent(void) {
        {
            std::scoped_lock lk(g_event_info_lock);
            R_ABORT_UNLESS(btdrvGetHidEventInfo(&g_event_info, sizeof(bluetooth::HidEventInfo), &g_current_event_type));
        }

        switch (g_current_event_type) {
            case BtdrvHidEventType_ConnectionState:
                HandleConnectionStateEvent(&g_event_info);
                break;
            case BtdrvHidEventType_Unknown7:
                //HandleUnknown07Event(&g_event_info);
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
