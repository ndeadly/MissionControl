/*
 * Copyright (C) 2020 ndeadly
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "btm_mitm_service.hpp"
#include "btm_shim.h"
#include "../controllers/controller_management.hpp"
#include <cstring>

namespace ams::mitm::btm {

    namespace {

        void RenameConnectedDevices(BtmConnectedDevice devices[], size_t count) {
            for (unsigned int i = 0; i < count; ++i) {
                auto device = &devices[i];
                if (!controller::IsOfficialSwitchControllerName(device->name)) {
                    std::strncpy(device->name, controller::pro_controller_name, sizeof(device->name) - 1);
                }
            }
        }

    }

    Result BtmMitmService::GetDeviceConditionDeprecated1(sf::Out<DeviceConditionV100> out) {
        auto device_condition = reinterpret_cast<BtmDeviceConditionV100 *>(out.GetPointer());
        R_TRY(btmGetDeviceConditionDeprecated1Fwd(this->forward_service.get(), device_condition));
        RenameConnectedDevices(device_condition->devices, device_condition->connected_count);
        return ams::ResultSuccess();
    }

    Result BtmMitmService::GetDeviceConditionDeprecated2(sf::Out<DeviceConditionV510> out) {
        auto device_condition = reinterpret_cast<BtmDeviceConditionV510 *>(out.GetPointer());
        R_TRY(btmGetDeviceConditionDeprecated2Fwd(this->forward_service.get(), device_condition));
        RenameConnectedDevices(device_condition->devices, device_condition->connected_count);
        return ams::ResultSuccess();
    }

    Result BtmMitmService::GetDeviceConditionDeprecated3(sf::Out<DeviceConditionV800> out) {
        auto device_condition = reinterpret_cast<BtmDeviceConditionV800 *>(out.GetPointer());
        R_TRY(btmGetDeviceConditionDeprecated1Fwd(this->forward_service.get(), device_condition));
        RenameConnectedDevices(device_condition->devices, device_condition->connected_count);
        return ams::ResultSuccess();
    }

    Result BtmMitmService::GetDeviceCondition(sf::Out<btm::DeviceCondition> out) {
        auto device_condition = reinterpret_cast<BtmDeviceConditionV900 *>(out.GetPointer());
        R_TRY(btmGetDeviceConditionFwd(this->forward_service.get(), device_condition));
        RenameConnectedDevices(device_condition->devices, device_condition->connected_count);
        return ams::ResultSuccess();
    }

    Result BtmMitmService::GetDeviceInfo(sf::Out<btm::DeviceInfoList> out) {
        auto device_info = reinterpret_cast<BtmDeviceInfoList *>(out.GetPointer());
        R_TRY(btmGetDeviceInfoFwd(this->forward_service.get(), device_info));

        for (unsigned int i = 0; i < device_info->total_entries; ++i) {
            auto device = &device_info->devices[i];
            if (!controller::IsOfficialSwitchControllerName(device->name)) {
                std::strncpy(device->name, controller::pro_controller_name, sizeof(device->name) - 1);
            }
        }

        return ams::ResultSuccess();
    }

}
