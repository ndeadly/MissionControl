#pragma once

#include <cstring>
#include <switch.h>

#define BTM_COD_MAJOR_PERIPHERAL            0x05
#define BTM_COD_MINOR_GAMEPAD               0x08
#define BTM_COD_MINOR_JOYSTICK              0x04

inline bool bdcmp(const BluetoothAddress *addr1, const BluetoothAddress *addr2) {
    //return (*(uint64_t *)addr1 & 0xffffffffffff0000) == (*(uint64_t *)addr2 & 0xffffffffffff0000);
    return std::memcmp(addr1, addr2, sizeof(BluetoothAddress)) == 0;
}

inline bool isController(const BluetoothDeviceClass *cod) {
	return ( (((uint8_t *)cod)[1] & 0x0f) == BTM_COD_MAJOR_PERIPHERAL ) &&
	       ( ((((uint8_t *)cod)[2] & 0x0f) == BTM_COD_MINOR_GAMEPAD) || 
             ((((uint8_t *)cod)[2] & 0x0f) == BTM_COD_MINOR_JOYSTICK) );
}
