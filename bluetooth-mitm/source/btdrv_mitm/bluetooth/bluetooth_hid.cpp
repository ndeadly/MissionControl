/*
 * Copyright (c) 2020 ndeadly
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
#include <atomic>
#include <mutex>
#include <cstring>

namespace ams::bluetooth::hid {

    namespace {

        std::atomic<bool> g_is_initialized(false);

        os::Mutex    g_event_data_lock(false);
        uint8_t      g_event_data_buffer[0x480];
        HidEventType g_current_event_type;

        os::SystemEventType g_system_event;
        os::SystemEventType g_system_event_fwd;
        os::SystemEventType g_system_event_user_fwd;
        os::EventType       g_data_read_event;

    }

    bool IsInitialized(void) {
        return g_is_initialized;
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

        g_is_initialized = true;

        return ams::ResultSuccess();
    }

    void Finalize(void) {
        os::FinalizeEvent(&g_data_read_event);
        os::DestroySystemEvent(&g_system_event_user_fwd);
        os::DestroySystemEvent(&g_system_event_fwd); 

        g_is_initialized = false;           
    }

    Result GetEventInfo(ncm::ProgramId program_id, HidEventType *type, uint8_t* buffer, size_t size) {
        std::scoped_lock lk(g_event_data_lock);

        *type = g_current_event_type;
        std::memcpy(buffer, g_event_data_buffer, size);

        os::SignalEvent(&g_data_read_event);

        return ams::ResultSuccess();
    }

    void handleConnectionStateEvent(HidEventData *event_data) {
        switch (event_data->connectionState.state) {
            case HidConnectionState_Connected:
                controller::AttachHandler(&event_data->connectionState.address);
                break;
            case HidConnectionState_Disconnected:
                controller::RemoveHandler(&event_data->connectionState.address);
                break;
            default:
                break;
        }
    }

    void handleUnknown07Event(HidEventData *event_data) {
        // Fix for xbox one disconnection. Don't know what this value is for, but it appears to be 0 for other controllers
        if (hos::GetVersion() < hos::Version_9_0_0)
            event_data->unknown07._unk1 = 0;
        else
            event_data->unknown07.v2._unk1 = 0;
    }

    void HandleEvent(void) {
        {
            std::scoped_lock lk(g_event_data_lock);
            R_ABORT_UNLESS(btdrvGetHidEventInfo(&g_current_event_type, g_event_data_buffer, sizeof(g_event_data_buffer)));
        }

        auto event_data = reinterpret_cast<HidEventData *>(g_event_data_buffer);

        switch (g_current_event_type) {

            case HidEvent_ConnectionState:
                handleConnectionStateEvent(event_data);
                break;
            case HidEvent_Unknown07:
                handleUnknown07Event(event_data);
                break;
            default:
                break;
        }

        os::SignalSystemEvent(&g_system_event_fwd);
        os::WaitEvent(&g_data_read_event);

        if (g_system_event_user_fwd.state)
            os::SignalSystemEvent(&g_system_event_user_fwd);
    }

}
