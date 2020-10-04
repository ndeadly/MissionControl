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
#include "bluetooth_core.hpp"
#include "../btdrv_mitm_flags.hpp"
#include "../../controllers/controller_management.hpp"
#include <atomic>
#include <mutex>
#include <cstring>

namespace ams::bluetooth::core {

    namespace {

        std::atomic<bool> g_is_initialized(false);

        os::Mutex g_event_data_lock(false);
        uint8_t   g_event_data_buffer[0x400];
        EventType g_current_event_type;

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

    Result GetEventInfo(ncm::ProgramId program_id, EventType *type, uint8_t* buffer, size_t size) {
        std::scoped_lock lk(g_event_data_lock);

        *type = g_current_event_type;
        std::memcpy(buffer, g_event_data_buffer, size);

        auto event_data = reinterpret_cast<EventData *>(buffer);
        if (program_id == ncm::SystemProgramId::Btm) {
            switch (g_current_event_type) {
                case BluetoothEvent_DeviceFound:
                    if (controller::IsAllowedDevice(&event_data->deviceFound.cod) && !controller::IsOfficialSwitchControllerName(event_data->deviceFound.name, sizeof(BluetoothName))) {
                        std::strncpy(event_data->deviceFound.name, controller::pro_controller_name, sizeof(BluetoothName) - 1);
                    }
                    break;
                case BluetoothEvent_PinRequest:
                    if (!controller::IsOfficialSwitchControllerName(event_data->pinReply.name, sizeof(BluetoothName))) {
                        std::strncpy(event_data->pinReply.name, controller::pro_controller_name, sizeof(BluetoothName) - 1);
                    }
                    break;
                case BluetoothEvent_SspRequest:
                    if (!controller::IsOfficialSwitchControllerName(event_data->sspReply.name, sizeof(BluetoothName))) {
                        std::strncpy(event_data->sspReply.name, controller::pro_controller_name, sizeof(BluetoothName) - 1);
                    }
                    break;
                default:
                    break;
            }
        }

        os::SignalEvent(&g_data_read_event);

        return ams::ResultSuccess();
    }

    void HandleEvent(void) {
        {
            std::scoped_lock lk(g_event_data_lock);
            R_ABORT_UNLESS(btdrvGetEventInfo(&g_current_event_type, g_event_data_buffer, sizeof(g_event_data_buffer)));
        }

        if (!g_redirect_core_events) {
            if (g_current_event_type == BluetoothEvent_PinRequest) {
                auto event_data = reinterpret_cast<EventData *>(g_event_data_buffer);

                bluetooth::PinCode pin_code = {0x30, 0x30, 0x30, 0x30};
                uint8_t pin_length = sizeof(uint32_t);;

                // Reverse host address as pin code for wii devices
                if (std::strncmp(event_data->pinReply.name, controller::wii_controller_prefix, std::strlen(controller::wii_controller_prefix)) == 0) {
                    // Fetch host adapter properties
                    BluetoothAdapterProperty properties;
                    R_ABORT_UNLESS(btdrvGetAdapterProperties(&properties));
                    // Reverse host address
                    *reinterpret_cast<uint64_t *>(&pin_code) = util::SwapBytes(*reinterpret_cast<uint64_t *>(&properties.address)) >> 16;
                    pin_length = sizeof(bluetooth::Address);
                }

                // Fuck BTM, we're sending the pin response ourselves if it won't.
                R_ABORT_UNLESS(btdrvRespondToPinRequest(&event_data->pinReply.address, false, &pin_code, pin_length));
            }
            else {
                os::SignalSystemEvent(&g_system_event_fwd);
                os::WaitEvent(&g_data_read_event);
            }
        }

        if (g_system_event_user_fwd.state)
            os::SignalSystemEvent(&g_system_event_user_fwd);
    }

}
