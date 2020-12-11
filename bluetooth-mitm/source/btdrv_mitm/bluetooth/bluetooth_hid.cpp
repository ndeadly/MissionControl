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

        os::Mutex    g_event_info_lock(false);
        uint8_t      g_event_info_buffer[0x480];
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
        std::scoped_lock lk(g_event_info_lock);

        *type = g_current_event_type;
        std::memcpy(buffer, g_event_info_buffer, size);

        os::SignalEvent(&g_data_read_event);

        return ams::ResultSuccess();
    }

    void handleConnectionStateEvent(bluetooth::HidEventInfo *event_info) {
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

    void handleUnknown07Event(bluetooth::HidEventInfo *event_info) {
        // Fix for xbox one disconnection. Don't know what this value is for, but it appears to be 0 for other controllers
        if (hos::GetVersion() < hos::Version_9_0_0)
            event_info->type7.v1.unk_xC = 0;
        else
            event_info->type7.v9.unk_x4 = 0;
    }

    void HandleEvent(void) {
        {
            std::scoped_lock lk(g_event_info_lock);
            R_ABORT_UNLESS(btdrvGetHidEventInfo(g_event_info_buffer, sizeof(g_event_info_buffer), &g_current_event_type));
        }

        auto event_info = reinterpret_cast<bluetooth::HidEventInfo *>(g_event_info_buffer);

        switch (g_current_event_type) {

            case BtdrvHidEventType_ConnectionState:
                handleConnectionStateEvent(event_info);
                break;
            case BtdrvHidEventType_Unknown7:
                handleUnknown07Event(event_info);
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
