#pragma once
#include <stratosphere.hpp>
#include <switch.h>

namespace ams::bluetooth {

    enum SubCmdType : u8 {
        SubCmd_GetControllerState   = 0x00,
        SubCmd_ManualPair           = 0x01,
        SubCmd_RequestDeviceInfo 	= 0x02,
        SubCmd_SetInputReportMode	= 0x03,
        SubCmd_TriggersElapsedTime	= 0x04,
        SubCmd_SetHciState          = 0x06,
        SubCmd_ResetPairingInfo     = 0x07,
        SubCmd_SetShipPowerState    = 0x08,
        SubCmd_SpiFlashRead			= 0x10,
        SubCmd_SpiFlashWrite		= 0x11,
        SubCmd_SpiSectorErase       = 0x12,
        SubCmd_ResetMcu             = 0x20,
        SubCmd_SetMcuConfig			= 0x21,
        SubCmd_SetMcuState          = 0x22,
        SubCmd_SetPlayerLeds 		= 0x30,
        SubCmd_GetPlayerLeds        = 0x31,
        SubCmd_SetHomeLed           = 0x38,
        SubCmd_EnableImu			= 0x40,
        SubCmd_SetImuSensitivity    = 0x41,
        SubCmd_WriteImuRegisters    = 0x42,
        SubCmd_ReadImuRegisters     = 0x43,
        SubCmd_EnableVibration		= 0x48,
        SubCmd_GetRegulatedVoltage  = 0x50,
    };

    typedef BluetoothEventType          EventType;
    typedef BluetoothHidEventType       HidEventType;
    typedef BluetoothBleEventType       BleEventType;
    //typedef BluetoothHidReportEventType HidReportEventType;

    typedef BluetoothEventData          EventData;
    typedef BluetoothHidEventData       HidEventData;
    typedef BluetoothBleEventData       BleEventData;
    //typedef BluetoothHidReportEventData HidReportEventData;
    
    typedef BluetoothAddress Address;
    typedef BluetoothPinCode PinCode;

    typedef BluetoothHidReport     HidReport;
    typedef BluetoothHidReportData HidReportData;

    typedef BluetoothHhReportType HhReportType;

    struct DeviceSettings : sf::LargeData {
        BluetoothDevicesSettings device;
    };

}
