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
#pragma once
#include <stratosphere.hpp>
#include "btm/btm_types.hpp"

namespace ams::mitm::btm {

    namespace {

        #define AMS_BTM_MITM_INTERFACE_INFO(C, H) \
            AMS_SF_METHOD_INFO(C, H, 3,  Result, GetDeviceConditionDeprecated1, (sf::Out<DeviceConditionV100>), hos::Version_1_0_0, hos::Version_5_0_2) \
            AMS_SF_METHOD_INFO(C, H, 3,  Result, GetDeviceConditionDeprecated2, (sf::Out<DeviceConditionV510>), hos::Version_5_1_0, hos::Version_7_0_1) \
            AMS_SF_METHOD_INFO(C, H, 3,  Result, GetDeviceConditionDeprecated3, (sf::Out<DeviceConditionV800>), hos::Version_8_0_0, hos::Version_8_1_1) \
            AMS_SF_METHOD_INFO(C, H, 3,  Result, GetDeviceCondition,            (sf::Out<DeviceCondition>),     hos::Version_9_0_0) \
            AMS_SF_METHOD_INFO(C, H, 9,  Result, GetDeviceInfo,                 (sf::Out<DeviceInfo>)) \

        AMS_SF_DEFINE_MITM_INTERFACE(IBtmMitmInterface, AMS_BTM_MITM_INTERFACE_INFO)

    }

    class BtmMitmService : public sf::MitmServiceImplBase   {

        public:
            using MitmServiceImplBase::MitmServiceImplBase;

        public:
            static bool ShouldMitm(const sm::MitmProcessInfo &client_info) {
                return client_info.program_id == ncm::SystemProgramId::Hid;
            }

        public:
            Result GetDeviceConditionDeprecated1(sf::Out<DeviceConditionV100> out);
            Result GetDeviceConditionDeprecated2(sf::Out<DeviceConditionV510> out);
            Result GetDeviceConditionDeprecated3(sf::Out<DeviceConditionV800> out);
            Result GetDeviceCondition(sf::Out<btm::DeviceCondition> out);
            Result GetDeviceInfo(sf::Out<btm::DeviceInfo> out);
    };
    static_assert(IsIBtmMitmInterface<BtmMitmService>);

}
