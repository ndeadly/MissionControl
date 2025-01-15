/*
 * Copyright (c) 2020-2025 ndeadly
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

namespace ams::bluetooth::hid {

    namespace {

        constinit os::SdkMutex g_event_info_lock;
        constinit bluetooth::HidEventInfo g_event_info;
        constinit bluetooth::HidEventType g_current_event_type;

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

    void SignalFakeEvent(bluetooth::HidEventType type, const void *data, size_t size) {
        g_current_event_type = type;
        std::memcpy(&g_event_info, data, size);

        g_system_event_fwd.Signal();
    }

    Result GetEventInfo(bluetooth::HidEventType *type, void *buffer, size_t size) {
        std::scoped_lock lk(g_event_info_lock);

        *type = g_current_event_type;
        std::memcpy(buffer, &g_event_info, size);

        g_data_read_event.Signal();

        R_SUCCEED();
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

    void HandleEvent() {
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
