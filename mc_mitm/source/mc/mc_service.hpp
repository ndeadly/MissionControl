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
#pragma once
#include <stratosphere.hpp>
#include "mc_types.hpp"
#include "../bluetooth_mitm/bluetooth/bluetooth_types.hpp"

#define AMS_MISSION_CONTROL_INTERFACE_INFO(C, H)                                                                                                                                      \
    AMS_SF_METHOD_INFO(C, H, 0, Result, GetVersion,            (sf::Out<u32> version),                                                                  (version)                   ) \
    AMS_SF_METHOD_INFO(C, H, 1, Result, GetBuildVersionString, (sf::Out<ams::mc::VersionString> version),                                               (version)                   ) \
    AMS_SF_METHOD_INFO(C, H, 2, Result, GetBuildDateString,    (sf::Out<ams::mc::DateString> version),                                                  (version)                   ) \
    AMS_SF_METHOD_INFO(C, H, 3, Result, GetHciHandle,          (bluetooth::Address address, sf::Out<u16> handle),                                       (address, handle)           ) \
    AMS_SF_METHOD_INFO(C, H, 4, Result, SendHciCommand,        (u16 opcode, const sf::InPointerBuffer &buffer, const sf::OutPointerBuffer &out_buffer), (opcode, buffer, out_buffer)) \
    AMS_SF_METHOD_INFO(C, H, 5, Result, DmSetConfig,           (const ams::mc::BsaSetConfig &set_config),                                               (set_config)                ) \

AMS_SF_DEFINE_INTERFACE(ams::mc, IMissionControlInterface, AMS_MISSION_CONTROL_INTERFACE_INFO, 0x30eba3d4)

namespace ams::mc {

    class MissionControlService {
        public:
            Result GetVersion(sf::Out<u32> version);
            Result GetBuildVersionString(sf::Out<ams::mc::VersionString> version);
            Result GetBuildDateString(sf::Out<ams::mc::DateString> date);
            Result GetHciHandle(bluetooth::Address address, sf::Out<u16> handle);
            Result SendHciCommand(u16 opcode, const sf::InPointerBuffer &buffer, const sf::OutPointerBuffer &out_buffer);
            Result DmSetConfig(const ams::mc::BsaSetConfig &set_config);
    };
    static_assert(IsIMissionControlInterface<MissionControlService>);

}
