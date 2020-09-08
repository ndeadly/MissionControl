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
#include <cstring>

namespace ams::mitm::btm {

    Result BtmMitmService::GetDeviceCondition(sf::Out<btm::DeviceCondition> out) {
        auto device_condition = reinterpret_cast<BtmDeviceCondition *>(out.GetPointer());
        R_TRY(btmGetDeviceConditionFwd(this->forward_service.get(), device_condition));

        for (unsigned int i = 0; i < device_condition->connected_count; ++i) {
            auto device = &device_condition->devices[i];
            if (!IsOfficialSwitchControllerName(device->name, sizeof(device->name))) {
                std::strncpy(device->name, controller::pro_controller_name, sizeof(device->name) - 1);
            }
        }

        return ams::ResultSuccess();
    }

    Result BtmMitmService::GetDeviceInfo(sf::Out<btm::DeviceInfo> out) {
        auto device_info = reinterpret_cast<BtmDeviceInfo *>(out.GetPointer());
        R_TRY(btmGetDeviceInfoFwd(this->forward_service.get(), device_info));

        for (unsigned int i = 0; i < device_info->count; ++i) {
            auto device = &device_info->devices[i];
            if (!IsOfficialSwitchControllerName(device->name, sizeof(device->name))) {
                std::strncpy(device->name, controller::pro_controller_name, sizeof(device->name) - 1);
            }
        }

        return ams::ResultSuccess();
    }

}
