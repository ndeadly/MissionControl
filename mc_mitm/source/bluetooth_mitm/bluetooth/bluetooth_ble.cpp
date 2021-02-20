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
#include "bluetooth_ble.hpp"
#include "../btdrv_mitm_flags.hpp"
#include <mutex>
#include <cstring>

namespace ams::bluetooth::ble {

    namespace {

        os::Mutex    g_event_data_lock(false);
        uint8_t      g_event_data_buffer[0x400];
        BleEventType g_current_event_type;

        os::SystemEventType g_system_event;
        os::SystemEventType g_system_event_fwd;
        os::SystemEventType g_system_event_user_fwd;
        os::EventType       g_data_read_event;

        os::Event g_init_event(os::EventClearMode_ManualClear);

    }

    bool IsInitialized() {
        return g_init_event.TryWait();
    }

    void WaitInitialized(void) {
        g_init_event.Wait();
    }

    os::SystemEventType *GetSystemEvent(void) {
        return &g_system_event;
    }

    os::SystemEventType *GetForwardEvent(void) {
        return &g_system_event_fwd;
    }

    os::SystemEventType *GetUserForwardEvent(void) {
        return &g_system_event_user_fwd;
    }

    Result Initialize(Handle event_handle) {
        os::AttachReadableHandleToSystemEvent(&g_system_event, event_handle, false, os::EventClearMode_ManualClear);

        R_TRY(os::CreateSystemEvent(&g_system_event_fwd, os::EventClearMode_AutoClear, true));
        R_TRY(os::CreateSystemEvent(&g_system_event_user_fwd, os::EventClearMode_AutoClear, true));
        os::InitializeEvent(&g_data_read_event, false, os::EventClearMode_AutoClear);

        g_init_event.Signal();

        return ams::ResultSuccess();
    }

    void Finalize(void) {
        os::FinalizeEvent(&g_data_read_event);
        os::DestroySystemEvent(&g_system_event_user_fwd);
        os::DestroySystemEvent(&g_system_event_fwd);
    }

    Result GetEventInfo(ncm::ProgramId program_id, BleEventType *type, uint8_t* buffer, size_t size) {
        std::scoped_lock lk(g_event_data_lock); 

        *type = g_current_event_type;
        std::memcpy(buffer, g_event_data_buffer, size);

        os::SignalEvent(&g_data_read_event);
        
        return ams::ResultSuccess();
    }

    void HandleEvent(void) {
        {
            std::scoped_lock lk(g_event_data_lock); 
            R_ABORT_UNLESS(btdrvGetBleManagedEventInfo(g_event_data_buffer, sizeof(g_event_data_buffer), &g_current_event_type));
        }

        if (!g_redirect_ble_events) {
            os::SignalSystemEvent(&g_system_event_fwd);
            os::WaitEvent(&g_data_read_event);
        }

        if (g_system_event_user_fwd.state) {
            os::SignalSystemEvent(&g_system_event_user_fwd);
        }
    }

}
