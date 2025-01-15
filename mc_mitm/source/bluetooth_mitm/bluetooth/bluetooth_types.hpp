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

namespace ams::bluetooth {

    using Address = ::BtdrvAddress;
    using DeviceClass = ::BtdrvClassOfDevice;
    using PinCode = ::BtdrvBluetoothPinCode;
    using AdapterProperty = ::BtdrvAdapterProperty;
    using HidReport = ::BtdrvHidReport;
    using HhReportType = ::BtdrvBluetoothHhReportType;
    using DevicesSettings = ::SetSysBluetoothDevicesSettings;

    using EventType = ::BtdrvEventType;
    using EventInfo = ::BtdrvEventInfo;

    using HidEventType = ::BtdrvHidEventType;
    using HidEventInfo = ::BtdrvHidEventInfo;

    using BleEventType = ::BtdrvBleEventType;
    using BleEventInfo = ::BtdrvBleEventInfo;

    using HidReportEventInfo = ::BtdrvHidReportEventInfo;

}
