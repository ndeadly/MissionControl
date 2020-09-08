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
#include "btm_types.hpp"

namespace ams::mitm::btm {

    namespace {

        #define AMS_BTM_MITM_INTERFACE_INFO(C, H) \
            AMS_SF_METHOD_INFO(C, H, 3,  Result, GetDeviceCondition,    (sf::Out<btm::DeviceCondition>)) \
            AMS_SF_METHOD_INFO(C, H, 9,  Result, GetDeviceInfo,         (sf::Out<btm::DeviceInfo>)) \

        AMS_SF_DEFINE_MITM_INTERFACE(IBtmMitmInterface, AMS_BTM_MITM_INTERFACE_INFO)

    }

    class BtmMitmService : public sf::MitmServiceImplBase   {

        public:
            using MitmServiceImplBase::MitmServiceImplBase;

        public:
            static bool ShouldMitm(const sm::MitmProcessInfo &client_info) {
                return true;
            }

        public:
            Result GetDeviceCondition(sf::Out<btm::DeviceCondition> out);
            Result GetDeviceInfo(sf::Out<btm::DeviceInfo> out);
    };
    static_assert(IsIBtmMitmInterface<BtmMitmService>);

}
