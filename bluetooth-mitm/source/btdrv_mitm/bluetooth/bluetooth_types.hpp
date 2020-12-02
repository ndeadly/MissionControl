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
#include <switch.h>
#include <stratosphere.hpp>

namespace ams::bluetooth {
   
    typedef char Name[0xf9];
    typedef BtdrvAddress Address;
    typedef BtdrvBluetoothPinCode PinCode;
    typedef BtdrvAdapterProperty AdapterProperty;
    typedef BtdrvHidReport HidReport;
    typedef BtdrvBluetoothHhReportType HhReportType;
    typedef SetSysBluetoothDevicesSettings DevicesSettings;

    typedef BtdrvEventType EventType;
    typedef BtdrvHidEventType HidEventType;
    typedef BtdrvBleEventType BleEventType;

    struct DeviceClass {
        u8 cod[0x3];
    };

    struct HidReportData {
        union {
            // Pre 9.0.0
            struct {
                u16     size;
                u8      _unk0;
                Address address;
                u8      _unk1[3];
            };
            // 9.0.0+
            struct {
                u8      _unk0[5];
                Address address;
                u8      _unk1;
            } v2;
        };
        
        HidReport report;
    };

    enum SspVariant {
        BluetoothSspVariant_PasskeyConfirmation,
        BluetoothSspVariant_PasskeyEntry,
        BluetoothSspVariant_Consent,
        BluetoothSspVariant_PasskeyNotification
    };

    enum Transport {
        BluetoothTransport_Auto,
        BluetoothTransport_BREDR,
        BluetoothTransport_LE
    };

    enum DiscoveryState {
        BluetoothDiscoveryState_Stopped,
        BluetoothDiscoveryState_Started
    };

    enum BondState {
        BluetoothBondState_None,
        BluetoothBondState_Bonding,
        BluetoothBondState_Bonded
    };

    enum Status {
        BluetoothStatus_Success,
        BluetoothStatus_Fail,
        BluetoothStatus_NotReady,
        BluetoothStatus_NoMemory,
        BluetoothStatus_Busy,
        BluetoothStatus_Done,
        BluetoothStatus_Unsupported,
        BluetoothStatus_ParameterInvalid,
        BluetoothStatus_Unhandled,
        BluetoothStatus_AuthenticationFailure,
        BluetoothStatus_RemoteDeviceDown,
        BluetoothStatus_AuthenticationRejected,
        BluetoothStatus_JniEnvironmentError,
        BluetoothStatus_JniThreadAttachError,
        BluetoothStatus_WakelockError
    };

    enum HidConnectionState {
        BluetoothHidConnectionState_Connected                    = 0,
        BluetoothHidConnectionState_Connecting,
        BluetoothHidConnectionState_Disconnected,
        BluetoothHidConnectionState_Disconnecting,
        BluetoothHidConnectionState_FailedMouseFromHost,
        BluetoothHidConnectionState_FailedKeyboardFromHost,
        BluetoothHidConnectionState_FailedTooManyDevices,
        BluetoothHidConnectionState_FailedNoBluetoothHidDriver,
        BluetoothHidConnectionState_FailedGeneric,
        BluetoothHidConnectionState_Unknown
    };

    enum HidStatus {
        BluetoothHidStatus_Ok                            = 0,
        BluetoothHidStatus_HandshakeHidNotReady,
        BluetoothHidStatus_HandshakeInvalidReportId,
        BluetoothHidStatus_HandshakeTransactionNotSpt,
        BluetoothHidStatus_HandshakeInvalidParameter,
        BluetoothHidStatus_HandshakeError,
        BluetoothHidStatus_Error,
        BluetoothHidStatus_ErrorSdp,
        BluetoothHidStatus_ErrorProtocol,
        BluetoothHidStatus_ErrorDatabaseFull,
        BluetoothHidStatus_ErrorDeviceTypeUnsupported,
        BluetoothHidStatus_ErrorNoResources,
        BluetoothHidStatus_ErrorAuthenicationFailed,
        BluetoothHidStatus_ErrorHdl
    };

    union EventData {
        u8 raw[0x480];

        struct __attribute__ ((__packed__)) {
            Name        name;
            Address     address;
            u8          uuid[0x10];
            DeviceClass cod;
            /* + more items we don't care about */
            u8  _unk0;
            u8  _unk1[0x252];
            u32 _unk2;
        } device_found;
        
        struct {
            DiscoveryState state;
        } discovery_state;
        
        struct {
            Address     address;
            Name        name;
            DeviceClass cod;
        } pin_reply;
        
        struct {
            Address     address;
            Name        name;
            DeviceClass cod;
            SspVariant  variant;
            u32         passkey;
        } ssp_reply;
        
        union {
            struct {
                Address     address;
                Status      status;
                BondState   state;
            };
            struct {
                Status      status;
                Address     address;
                BondState   state;
            } v2;
        } bond_state;
    };

    union HidEventData {
        u8 raw[0x480];

        struct {
            Address            address;
            HidConnectionState state;
        } connection_state;

        struct {
            Address         address;
            HidStatus       status;
            u32 	        report_length;
            HidReportData   report_data;
        } get_report;

        union {
            struct {
                Address address;
                u32     _unk0;
                u32     _unk1;                 
            };
            struct {
                u32     _unk0;
                u32     _unk1;
                Address address;                 
            } v2;
        } unknown07;
    };

}
