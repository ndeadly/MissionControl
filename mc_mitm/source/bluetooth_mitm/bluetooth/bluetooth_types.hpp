/*
 * Copyright (c) 2020-2021 ndeadly
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
   
    typedef BtdrvAddress Address;
    typedef BtdrvDeviceClass DeviceClass;
    typedef BtdrvBluetoothPinCode PinCode;
    typedef BtdrvAdapterProperty AdapterProperty;
    typedef BtdrvHidReport HidReport;
    typedef BtdrvBluetoothHhReportType HhReportType;
    typedef SetSysBluetoothDevicesSettings DevicesSettings;

    typedef BtdrvBluetoothSspVariant SspVariant;
    typedef BtdrvBluetoothTransport Transport;
    typedef BtdrvBluetoothDiscoveryState DiscoveryState;
    typedef BtdrvBluetoothBondState BondState;
    typedef BtdrvEventType EventType;
    typedef BtdrvEventInfo EventInfo;

    typedef BtdrvHidEventType HidEventType;
    typedef BtdrvHidEventInfo HidEventInfo;

    typedef BtdrvBleEventType BleEventType;
    typedef BtdrvBleEventInfo BleEventInfo;

    typedef BtdrvHidConnectionState HidConnectionState;
    typedef BtdrvHidReportData HidReportData;
  
}
