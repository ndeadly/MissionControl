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
#include "bluetooth_core.hpp"
#include "../btdrv_mitm_flags.hpp"
#include "../../controllers/controller_management.hpp"
#include <mutex>
#include <cstring>

namespace ams::bluetooth::core {

    namespace {

        os::Mutex g_event_info_lock(false);
        bluetooth::EventInfo g_event_info;
        bluetooth::EventType g_current_event_type;

        os::SystemEvent g_system_event;
        os::SystemEvent g_system_event_fwd(os::EventClearMode_AutoClear, true);
        os::SystemEvent g_system_event_user_fwd(os::EventClearMode_AutoClear, true);

        os::Event g_init_event(os::EventClearMode_ManualClear);
        os::Event g_enable_event(os::EventClearMode_ManualClear);
        os::Event g_data_read_event(os::EventClearMode_AutoClear);

    }

    bool IsInitialized() {
        return g_init_event.TryWait();
    }

    void WaitInitialized(void) {
        g_init_event.Wait();
    }

    void SignalEnabled(void) {
        g_enable_event.Signal();
    }

    void WaitEnabled(void) {
        g_enable_event.Wait();
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

    Result Initialize(Handle event_handle) {
        g_system_event.AttachReadableHandle(event_handle, false, os::EventClearMode_ManualClear);
        g_init_event.Signal();

        return ams::ResultSuccess();
    }

    void Finalize(void) {
        ;
    }

    Result GetEventInfo(bluetooth::EventType *type, uint8_t* buffer, size_t size) {
        std::scoped_lock lk(g_event_info_lock);

        *type = g_current_event_type;
        std::memcpy(buffer, &g_event_info, size);

        auto event_info = reinterpret_cast<bluetooth::EventInfo *>(buffer);

        switch (g_current_event_type) {
            case BtdrvEventType_DeviceFound:
                if (controller::IsAllowedDeviceClass(&event_info->device_found.cod) && !controller::IsOfficialSwitchControllerName(event_info->device_found.name)) {
                    std::strncpy(event_info->device_found.name, controller::pro_controller_name, sizeof(event_info->device_found.name) - 1);
                }
                break;
            case BtdrvEventType_PinRequest:
                if (!controller::IsOfficialSwitchControllerName(event_info->pin_reply.name)) {
                    std::strncpy(event_info->pin_reply.name, controller::pro_controller_name, sizeof(event_info->pin_reply.name) - 1);
                }
                break;
            case BtdrvEventType_SspRequest:
                if (!controller::IsOfficialSwitchControllerName(event_info->ssp_reply.name)) {
                    std::strncpy(event_info->ssp_reply.name, controller::pro_controller_name, sizeof(event_info->ssp_reply.name) - 1);
                }
                break;
            default:
                break;
        }

        g_data_read_event.Signal();

        return ams::ResultSuccess();
    }

    void HandleEvent(void) {
        {
            std::scoped_lock lk(g_event_info_lock);
            R_ABORT_UNLESS(btdrvGetEventInfo(&g_event_info, sizeof(bluetooth::EventInfo), &g_current_event_type));
        }

        if (!g_redirect_core_events) {
            if (g_current_event_type == BtdrvEventType_PinRequest) {
                // Default pin used by bluetooth service
                bluetooth::PinCode pin = {"0000"};
                uint8_t pin_length = std::strlen(pin.code);

                // Reverse host address as pin code for wii devices
                if (std::strncmp(g_event_info.pin_reply.name, controller::wii_controller_prefix, std::strlen(controller::wii_controller_prefix)) == 0) {
                    // Fetch host adapter address
                    bluetooth::Address host_address;
                    R_ABORT_UNLESS(btdrvGetAdapterProperty(BtdrvBluetoothPropertyType_Address, &host_address, sizeof(bluetooth::Address)));
                    // Reverse host address
                    *reinterpret_cast<uint64_t *>(&pin) = util::SwapBytes(*reinterpret_cast<uint64_t *>(&host_address)) >> 16;
                    pin_length = sizeof(bluetooth::Address);
                }

                // Fuck BTM, we're sending the pin response ourselves if it won't.
                R_ABORT_UNLESS(btdrvRespondToPinRequest(g_event_info.pin_reply.address, false, &pin, pin_length));
            }
            else {
                g_system_event_fwd.Signal();
                g_data_read_event.Wait();
            }
        }

        if (g_system_event_user_fwd.GetBase()->state) {
            g_system_event_user_fwd.Signal();
        }
    }

}
