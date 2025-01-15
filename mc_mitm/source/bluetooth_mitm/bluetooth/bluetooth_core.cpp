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
#include "bluetooth_core.hpp"
#include "../btdrv_ext.h"
#include "../btdrv_mitm_flags.hpp"
#include "../../controllers/controller_management.hpp"

namespace ams::bluetooth::core {

    namespace {

        constinit os::SdkMutex g_event_info_lock;
        constinit bluetooth::EventInfo g_event_info;
        constinit bluetooth::EventType g_current_event_type;

        os::SystemEvent g_system_event;
        os::SystemEvent g_system_event_fwd(os::EventClearMode_AutoClear, true);
        os::SystemEvent g_system_event_user_fwd(os::EventClearMode_AutoClear, true);

        os::Event g_init_event(os::EventClearMode_ManualClear);
        os::Event g_enable_event(os::EventClearMode_ManualClear);
        os::Event g_custom_data_event(os::EventClearMode_AutoClear);
        os::Event g_data_read_event(os::EventClearMode_AutoClear);

        bluetooth::Address ReverseBluetoothAddress(bluetooth::Address address) {
            u64 tmp;
            std::memcpy(&tmp, &address, sizeof(address));
            tmp = util::SwapEndian(tmp) >> 16;
            return *reinterpret_cast<bluetooth::Address *>(&tmp);
        }

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

    void SignalEnabled() {
        g_enable_event.Signal();
    }

    void WaitEnabled() {
        g_enable_event.Wait();
    }

    os::Event *GetCustomDataEvent() {
        return &g_custom_data_event;
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

    void SignalFakeEvent(bluetooth::EventType type, const void *data, size_t size) {
        g_current_event_type = type;
        std::memcpy(&g_event_info, data, size);

        g_system_event_fwd.Signal();
    }

    inline void ModifyEventInfov1(bluetooth::EventInfo *event_info, BtdrvEventType event_type) {
        switch (event_type) {
            case BtdrvEventTypeOld_InquiryDevice:
                if (controller::IsAllowedDeviceClass(&event_info->inquiry_device.v1.class_of_device) && !controller::IsOfficialSwitchControllerName(event_info->inquiry_device.v1.name)) {
                    std::strncpy(event_info->inquiry_device.v1.name, controller::ProControllerName, sizeof(event_info->inquiry_device.v1.name) - 1);
                }
                break;
            case BtdrvEventTypeOld_PairingPinCodeRequest:
                if (!controller::IsOfficialSwitchControllerName(event_info->pairing_pin_code_request.name)) {
                    std::strncpy(event_info->pairing_pin_code_request.name, controller::ProControllerName, sizeof(event_info->pairing_pin_code_request.name) - 1);
                }
                break;
            case BtdrvEventTypeOld_SspRequest:
                if (!controller::IsOfficialSwitchControllerName(event_info->ssp_request.v1.name)) {
                    std::strncpy(event_info->ssp_request.v1.name, controller::ProControllerName, sizeof(event_info->ssp_request.v1.name) - 1);
                }
                break;
            default:
                break;
        }
    }

    inline void ModifyEventInfov12(bluetooth::EventInfo *event_info, BtdrvEventType event_type) {
        switch (event_type) {
            case BtdrvEventType_InquiryDevice:
                if (controller::IsAllowedDeviceClass(&event_info->inquiry_device.v12.class_of_device) && !controller::IsOfficialSwitchControllerName(event_info->inquiry_device.v12.name)) {
                    std::strncpy(event_info->inquiry_device.v12.name, controller::ProControllerName, sizeof(event_info->inquiry_device.v12.name) - 1);
                }
                break;
            case BtdrvEventType_PairingPinCodeRequest:
                if (!controller::IsOfficialSwitchControllerName(event_info->pairing_pin_code_request.name)) {
                    std::strncpy(event_info->pairing_pin_code_request.name, controller::ProControllerName, sizeof(event_info->pairing_pin_code_request.name) - 1);
                }
                break;
            case BtdrvEventType_SspRequest:
                if (!controller::IsOfficialSwitchControllerName(event_info->ssp_request.v12.name)) {
                    std::strncpy(event_info->ssp_request.v12.name, controller::ProControllerName, sizeof(event_info->ssp_request.v12.name) - 1);
                }
                break;
            default:
                break;
        }
    }

    Result GetEventInfo(ncm::ProgramId program_id, bluetooth::EventType *type, void *buffer, size_t size) {
        std::scoped_lock lk(g_event_info_lock);

        *type = g_current_event_type;
        std::memcpy(buffer, &g_event_info, size);

        if (program_id == ncm::SystemProgramId::Btm) {
            auto event_info = reinterpret_cast<bluetooth::EventInfo *>(buffer);

            if (hos::GetVersion() < hos::Version_12_0_0) {
                ModifyEventInfov1(event_info, g_current_event_type);
            } else {
                ModifyEventInfov12(event_info, g_current_event_type);
            }
        }

        g_data_read_event.Signal();

        R_SUCCEED();
    }

    inline void HandlePinCodeRequestEventV1(bluetooth::EventInfo *event_info) {
        // Default pin used by bluetooth service
        bluetooth::PinCode pin = {"0000"};
        u8 pin_length = std::strlen(pin.code);

        // Reverse host address as pin code for wii devices
        if (std::strncmp(event_info->pairing_pin_code_request.name, controller::WiiControllerPrefix, std::strlen(controller::WiiControllerPrefix)) == 0) {
            // Fetch host adapter address
            bluetooth::Address host_address;
            R_ABORT_UNLESS(btdrvLegacyGetAdapterProperty(BtdrvBluetoothPropertyType_Address, &host_address, sizeof(bluetooth::Address)));

            // Set reversed bluetooth address as pin code
            *reinterpret_cast<bluetooth::Address *>(&pin) = ReverseBluetoothAddress(host_address);
            pin_length = sizeof(bluetooth::Address);
        }

        R_ABORT_UNLESS(btdrvLegacyRespondToPinRequest(event_info->pairing_pin_code_request.addr, false, &pin, pin_length));
    }

    inline void HandlePinCodeRequestEventV12(bluetooth::EventInfo *event_info) {
        // Default pin used by bluetooth service
        BtdrvPinCode pin = {"0000", 4};

        // Reverse host address as pin code for wii devices
        if (std::strncmp(event_info->pairing_pin_code_request.name, controller::WiiControllerPrefix, std::strlen(controller::WiiControllerPrefix)) == 0) {
            // Fetch host adapter address
            BtdrvAdapterProperty property;
            R_ABORT_UNLESS(btdrvGetAdapterProperty(BtdrvAdapterPropertyType_Address, &property));

            bluetooth::Address host_address = *reinterpret_cast<bluetooth::Address *>(property.data);

            // Set reversed bluetooth address as pin code
            *reinterpret_cast<bluetooth::Address *>(&pin.code) = ReverseBluetoothAddress(host_address);
            pin.length = sizeof(bluetooth::Address);
        }

        R_ABORT_UNLESS(btdrvRespondToPinRequest(event_info->pairing_pin_code_request.addr, &pin));
    }

    void HandleEvent() {
        {
            std::scoped_lock lk(g_event_info_lock);
            R_ABORT_UNLESS(btdrvGetEventInfo(&g_event_info, sizeof(bluetooth::EventInfo), &g_current_event_type));
        }

        // Process custom event and return
        if (g_current_event_type == BtdrvEventType_MissionControlCustomEvent) {
            g_custom_data_event.Signal();
            g_data_read_event.Wait();
            return;
        }

        if (!g_redirect_core_events) {
            if ((hos::GetVersion() < hos::Version_12_0_0) && (g_current_event_type == BtdrvEventTypeOld_PairingPinCodeRequest)) {
                HandlePinCodeRequestEventV1(&g_event_info);
            } else if ((hos::GetVersion() >= hos::Version_12_0_0) && (g_current_event_type == BtdrvEventType_PairingPinCodeRequest)) {
                HandlePinCodeRequestEventV12(&g_event_info);
            } else {
                g_system_event_fwd.Signal();
                g_data_read_event.Wait();
            }
        }

        if (g_system_event_user_fwd.GetBase()->state) {
            g_system_event_user_fwd.Signal();
        }
    }

}
