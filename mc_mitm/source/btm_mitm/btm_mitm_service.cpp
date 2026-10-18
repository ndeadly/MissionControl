/*
 * Copyright (c) 2020-2026 ndeadly
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
#include "btm_mitm_service.hpp"
#include "btm_shim.h"
#include "btm_device_condition.hpp"
#include "../controllers/controller_management.hpp"

namespace ams::mitm::btm {

    namespace {

        void RenameConnectedDevices(BtmDeviceConditionLegacy *devices, size_t count) {
            for (unsigned int i = 0; i < count; ++i) {
                auto device = &devices[i];
                if ((device->profile == BtmProfile_Hid) && !controller::IsOfficialSwitchControllerName(device->name)) {
                    std::strncpy(device->name, controller::LicensedProControllerName, sizeof(device->name) - 1);
                }
            }
        }

        template <typename T>
        void AppendVirtualDevices(T *device_condition, size_t max_size, sf::Out<s32> &total_out) {
            bluetooth::Address virtual_devices[4];
            const size_t virtual_count = controller::GetVirtualDevices(virtual_devices, util::size(virtual_devices));

            s32 total = total_out.GetValue();
            const s32 existing_count = total;
            for (size_t i = 0; (i < virtual_count) && (static_cast<size_t>(total) < max_size); ++i) {
                bool already_present = false;
                for (s32 j = 0; j < existing_count; ++j) {
                    if (std::memcmp(&device_condition[j].address, &virtual_devices[i], sizeof(bluetooth::Address)) == 0) {
                        already_present = true;
                        break;
                    }
                }
                if (already_present) {
                    continue;
                }

                auto device = &device_condition[total++];
                std::memset(device, 0, sizeof(*device));
                device->address = virtual_devices[i];
                device->profile = BtmProfile_Hid;
                device->profile_condition.hid.sniff_mode = BtmSniffMode_15ms;
                device->profile_condition.hid.slot_mode  = BtmSlotMode_2;
                device->profile_condition.hid.vid        = 0x057e;
                device->profile_condition.hid.pid        = 0x2009;
                std::strncpy(device->name, controller::ProControllerName, sizeof(device->name) - 1);
            }
            total_out.SetValue(total);
        }

    }

    Result BtmMitmService::AcquireDeviceConditionEvent(sf::OutCopyHandle out_handle) {
        if (!condition::IsInitialized()) {
            // Forward to the real function to obtain the system event handle
            os::NativeHandle handle = os::InvalidNativeHandle;
            R_TRY(btmAcquireDeviceConditionEventFwd(m_forward_service.get(), &handle));

            // Attach the real system event handle to our own event
            condition::GetSystemEvent()->AttachReadableHandle(handle, false, os::EventClearMode_AutoClear);

            // Return our forwarder event handle to the caller instead
            out_handle.SetValue(condition::GetForwardEvent()->GetReadableHandle(), false);

            condition::SignalInitialized();
        } else {
            // hid is the only client, so it gets the event it already has rather than a second one nothing
            // would signal
            out_handle.SetValue(condition::GetForwardEvent()->GetReadableHandle(), false);
        }

        R_SUCCEED();
    }

    Result BtmMitmService::GetDeviceCondition(ams::btm::Profile profile, const sf::OutArray<ams::btm::DeviceCondition> &out, sf::Out<s32> total_out) {
        auto device_condition = reinterpret_cast<BtmDeviceCondition *>(out.GetPointer());
        R_TRY(btmGetDeviceConditionFwd(m_forward_service.get(), profile, device_condition, out.GetSize(), total_out.GetPointer()));

        for (int i = 0; i < total_out.GetValue(); ++i) {
            auto device = &device_condition[i];
            if ((device->profile == BtmProfile_Hid) && !controller::IsOfficialSwitchControllerName(device->name)) {
                std::strncpy(device->name, controller::LicensedProControllerName, sizeof(device->name) - 1);
            }
        }

        // Append controllers btm doesn't know about (Bluetooth LE).
        if ((profile == BtmProfile_Hid) || (profile == BtmProfile_None)) {
            AppendVirtualDevices(device_condition, out.GetSize(), total_out);
        }

        R_SUCCEED();
    }

    Result BtmMitmService::GetDeviceInfo(ams::btm::Profile profile, const sf::OutArray<ams::btm::DeviceInfo> &out, sf::Out<s32> total_out) {
        auto device_info = reinterpret_cast<BtmDeviceInfo *>(out.GetPointer());
        R_TRY(btmGetDeviceInfoFwd(m_forward_service.get(), profile, device_info, out.GetSize(), total_out.GetPointer()));

        for (int i = 0; i < total_out.GetValue(); ++i) {
            auto device = &device_info[i];
            if ((device->profile == BtmProfile_Hid) && !controller::IsOfficialSwitchControllerName(device->name)) {
                std::strncpy(device->name, controller::LicensedProControllerName, sizeof(device->name) - 1);
            }
        }

        R_SUCCEED();
    }

    Result BtmMitmService::GetShortenedDeviceInfo(ams::btm::Profile profile, const sf::OutArray<ams::btm::ShortenedDeviceInfo> &out, sf::Out<s32> total_out) {
        auto device_info = reinterpret_cast<BtmShortenedDeviceInfo *>(out.GetPointer());
        R_TRY(btmGetShortenedDeviceInfoFwd(m_forward_service.get(), profile, device_info, out.GetSize(), total_out.GetPointer()));

        for (int i = 0; i < total_out.GetValue(); ++i) {
            auto device = &device_info[i];
            if ((device->profile == BtmProfile_Hid) && !controller::IsOfficialSwitchControllerName(device->name)) {
                std::strncpy(device->name, controller::LicensedProControllerName, sizeof(device->name) - 1);
            }
        }
        
        R_SUCCEED();
    }

    Result BtmMitmService::GetShortenedDeviceCondition(ams::btm::Profile profile, const sf::OutArray<ams::btm::ShortenedDeviceCondition> &out, sf::Out<s32> total_out) {
        auto device_condition = reinterpret_cast<BtmShortenedDeviceCondition *>(out.GetPointer());
        R_TRY(btmGetShortenedDeviceConditionFwd(m_forward_service.get(), profile, device_condition, out.GetSize(), total_out.GetPointer()));

        for (int i = 0; i < total_out.GetValue(); ++i) {
            auto device = &device_condition[i];
            if ((device->profile == BtmProfile_Hid) && !controller::IsOfficialSwitchControllerName(device->name)) {
                std::strncpy(device->name, controller::LicensedProControllerName, sizeof(device->name) - 1);
            }
        }

        if ((profile == BtmProfile_Hid) || (profile == BtmProfile_None)) {
            AppendVirtualDevices(device_condition, out.GetSize(), total_out);
        }

        R_SUCCEED();
    }

    /* Deprecated */

    /* 1.0.0 - 5.0.2 */
    Result BtmMitmService::GetDeviceConditionDeprecated1(sf::Out<ams::btm::DeviceConditionListV100> out) {
        auto device_condition_list = reinterpret_cast<BtmDeviceConditionListV100 *>(out.GetPointer());
        R_TRY(btmGetDeviceConditionDeprecated1Fwd(m_forward_service.get(), device_condition_list));
        RenameConnectedDevices(device_condition_list->devices, device_condition_list->device_count);

        R_SUCCEED();
    }

    /* 5.1.0 - 7.0.1 */
    Result BtmMitmService::GetDeviceConditionDeprecated2(sf::Out<ams::btm::DeviceConditionListV510> out) {
        auto device_condition_list = reinterpret_cast<BtmDeviceConditionListV510 *>(out.GetPointer());
        R_TRY(btmGetDeviceConditionDeprecated2Fwd(m_forward_service.get(), device_condition_list));
        RenameConnectedDevices(device_condition_list->devices, device_condition_list->device_count);

        R_SUCCEED();
    }

    /* 8.0.0 - 8.1.1 */
    Result BtmMitmService::GetDeviceConditionDeprecated3(sf::Out<ams::btm::DeviceConditionListV800> out) {
        auto device_condition_list = reinterpret_cast<BtmDeviceConditionListV800 *>(out.GetPointer());
        R_TRY(btmGetDeviceConditionDeprecated3Fwd(m_forward_service.get(), device_condition_list));
        RenameConnectedDevices(device_condition_list->devices, device_condition_list->device_count);

        R_SUCCEED();
    }

    /* 9.0.0 - 12.1.0 */
    Result BtmMitmService::GetDeviceConditionDeprecated4(sf::Out<ams::btm::DeviceConditionListV900> out) {
        auto device_condition_list = reinterpret_cast<BtmDeviceConditionListV900 *>(out.GetPointer());
        R_TRY(btmGetDeviceConditionDeprecated4Fwd(m_forward_service.get(), device_condition_list));
        RenameConnectedDevices(device_condition_list->devices, device_condition_list->device_count);

        R_SUCCEED();
    }

    /* 1.0.0 - 12.1.0 */
    Result BtmMitmService::GetDeviceInfoDeprecated(sf::Out<ams::btm::DeviceInfoList> out) {
        auto device_info_list = reinterpret_cast<BtmDeviceInfoList *>(out.GetPointer());
        R_TRY(btmGetDeviceInfoDeprecatedFwd(m_forward_service.get(), device_info_list));

        for (unsigned int i = 0; i < device_info_list->device_count; ++i) {
            auto device = &device_info_list->devices[i];
            if ((device->profile == BtmProfile_Hid) && !controller::IsOfficialSwitchControllerName(device->name.name)) {
                std::strncpy(device->name.name, controller::LicensedProControllerName, sizeof(device->name) - 1);
            }
        }

        R_SUCCEED();
    }

}
