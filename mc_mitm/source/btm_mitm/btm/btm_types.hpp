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
#include <switch.h>
#include <stratosphere.hpp>

namespace ams::btm {

    using Profile = ::BtmProfile;

    struct DeviceConditionV100 : public sf::LargeData {
        BtmDeviceConditionV100 condition;
    };

    struct DeviceConditionV510 : public sf::LargeData {
        BtmDeviceConditionV510 condition;
    };

    struct DeviceConditionV800 : public sf::LargeData {
        BtmDeviceConditionV800 condition;
    };

    struct DeviceConditionV900 : public sf::LargeData {
        BtmDeviceConditionV900 condition;
    };

    struct ConnectedDevice : public sf::LargeData {
        BtmConnectedDeviceV13 condition;
    };

    struct DeviceInfo : public sf::LargeData {
        BtmDeviceInfoV13 info;
    };

    struct DeviceInfoList : public sf::LargeData {
        BtmDeviceInfoList info;
    };

}
