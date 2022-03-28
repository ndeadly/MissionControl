/*
 * Copyright (c) 2020-2022 ndeadly
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
#pragma once
#include <stratosphere.hpp>
#include "btm/btm_types.hpp"

#define AMS_BTM_MITM_INTERFACE_INFO(C, H)                                                                                                                                                                                    \
    AMS_SF_METHOD_INFO(C, H, 3,  Result, GetDeviceConditionDeprecated1, (sf::Out<ams::btm::DeviceConditionV100> out),                                         (out),                hos::Version_1_0_0, hos::Version_5_0_2)  \
    AMS_SF_METHOD_INFO(C, H, 3,  Result, GetDeviceConditionDeprecated2, (sf::Out<ams::btm::DeviceConditionV510> out),                                         (out),                hos::Version_5_1_0, hos::Version_7_0_1)  \
    AMS_SF_METHOD_INFO(C, H, 3,  Result, GetDeviceConditionDeprecated3, (sf::Out<ams::btm::DeviceConditionV800> out),                                         (out),                hos::Version_8_0_0, hos::Version_8_1_1)  \
    AMS_SF_METHOD_INFO(C, H, 3,  Result, GetDeviceConditionDeprecated4, (sf::Out<ams::btm::DeviceConditionV900> out),                                         (out),                hos::Version_9_0_0, hos::Version_12_1_0) \
    AMS_SF_METHOD_INFO(C, H, 3,  Result, GetDeviceCondition,            (u32 id, const sf::OutArray<ams::btm::ConnectedDevice> &out, sf::Out<s32> total_out), (id, out, total_out), hos::Version_13_0_0)                     \
    AMS_SF_METHOD_INFO(C, H, 9,  Result, GetDeviceInfoDeprecated,       (sf::Out<ams::btm::DeviceInfoList> out),                                              (out),                hos::Version_1_0_0, hos::Version_12_1_0) \
    AMS_SF_METHOD_INFO(C, H, 9,  Result, GetDeviceInfo,                 (u32 id, const sf::OutArray<ams::btm::DeviceInfo> &out, sf::Out<s32> total_out),      (id, out, total_out), hos::Version_13_0_0)                     \

AMS_SF_DEFINE_MITM_INTERFACE(ams::mitm::btm, IBtmMitmInterface, AMS_BTM_MITM_INTERFACE_INFO, 0xB5E45686)

namespace ams::mitm::btm {

    class BtmMitmService : public sf::MitmServiceImplBase {

        public:
            using MitmServiceImplBase::MitmServiceImplBase;

        public:
            static bool ShouldMitm(const sm::MitmProcessInfo &client_info) {
                return client_info.program_id == ncm::SystemProgramId::Hid;
            }

        public:
            Result GetDeviceConditionDeprecated1(sf::Out<ams::btm::DeviceConditionV100> out);
            Result GetDeviceConditionDeprecated2(sf::Out<ams::btm::DeviceConditionV510> out);
            Result GetDeviceConditionDeprecated3(sf::Out<ams::btm::DeviceConditionV800> out);
            Result GetDeviceConditionDeprecated4(sf::Out<ams::btm::DeviceConditionV900> out);
            Result GetDeviceCondition(u32 id, const sf::OutArray<ams::btm::ConnectedDevice> &out, sf::Out<s32> total_out);
            Result GetDeviceInfoDeprecated(sf::Out<ams::btm::DeviceInfoList> out);
            Result GetDeviceInfo(u32 id, const sf::OutArray<ams::btm::DeviceInfo> &out, sf::Out<s32> total_out);
    };
    static_assert(IsIBtmMitmInterface<BtmMitmService>);

}
