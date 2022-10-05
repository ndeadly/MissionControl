/*
 * Copyright (c) 2020-2022 ndeadly
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
#include "bluetooth_ble.hpp"
#include "../btdrv_mitm_flags.hpp"

namespace ams::bluetooth::ble {

    namespace {

        os::Mutex g_event_data_lock(false);
        bluetooth::BleEventInfo g_event_info;
        bluetooth::BleEventType g_current_event_type;

        os::SystemEvent g_system_event;
        os::SystemEvent g_system_event_fwd(os::EventClearMode_AutoClear, true);
        os::SystemEvent g_system_event_user_fwd(os::EventClearMode_AutoClear, true);

        os::Event g_init_event(os::EventClearMode_ManualClear);
        os::Event g_data_read_event(os::EventClearMode_AutoClear);
    }

    bool IsInitialized() {
        return g_init_event.TryWait();
    }

    void SignalInitialized() {
        g_init_event.Signal();
    }

    void WaitInitialized() {
        g_init_event.Wait();
    }

    os::SystemEvent *GetSystemEvent() {
        return &g_system_event;
    }

    os::SystemEvent *GetForwardEvent() {
        return &g_system_event_fwd;
    }

    os::SystemEvent *GetUserForwardEvent() {
        return &g_system_event_user_fwd;
    }

    Result GetEventInfo(bluetooth::BleEventType *type, void *buffer, size_t size) {
        std::scoped_lock lk(g_event_data_lock); 

        *type = g_current_event_type;
        std::memcpy(buffer, &g_event_info, size);

        g_data_read_event.Signal();
        
        return ams::ResultSuccess();
    }

    void HandleEvent() {
        {
            std::scoped_lock lk(g_event_data_lock); 
            R_ABORT_UNLESS(btdrvGetBleManagedEventInfo(&g_event_info, sizeof(bluetooth::BleEventInfo), &g_current_event_type));
        }

        if (!g_redirect_ble_events) {
            g_system_event_fwd.Signal();
            g_data_read_event.Wait();
        }

        if (g_system_event_user_fwd.GetBase()->state) {
            g_system_event_user_fwd.Signal();
        }
    }

}
