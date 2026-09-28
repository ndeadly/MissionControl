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
#pragma once
#include <switch.h>
#include <stratosphere.hpp>

namespace ams::btm {

    using Profile = ::BtmProfile;

    struct DeviceConditionListV100 : public sf::LargeData {
        BtmDeviceConditionListV100 condition_list;
    };

    struct DeviceConditionListV510 : public sf::LargeData {
        BtmDeviceConditionListV510 condition_list;
    };

    struct DeviceConditionListV800 : public sf::LargeData {
        BtmDeviceConditionListV800 condition_list;
    };

    struct DeviceConditionListV900 : public sf::LargeData {
        BtmDeviceConditionListV900 condition_list;
    };

    struct DeviceCondition : public sf::LargeData {
        BtmDeviceCondition device_condition;
    };

    struct DeviceInfo : public sf::LargeData {
        BtmDeviceInfo device_info;
    };

    struct DeviceInfoList : public sf::LargeData {
        BtmDeviceInfoList info_list;
    };

}
