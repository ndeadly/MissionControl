#pragma once
#include <switch.h>
#include "switchcontroller.hpp"

#define BTM_COD_MAJOR_PERIPHERAL    0x05
#define BTM_COD_MINOR_GAMEPAD       0x08
#define BTM_COD_MINOR_JOYSTICK      0x04

namespace ams::controller {

    const constexpr char* proControllerName = "Pro Controller";

    ControllerType identifyController(const BluetoothDevicesSettings *device);

    bool IsValidSwitchControllerName(const char *name);
    bool IsJoyCon(const char *name);

    SwitchController *locateHandler(const bluetooth::Address *address);
    void attachHandler(const bluetooth::Address *address);
    void removeHandler(const bluetooth::Address *address);

    inline bool IsController(const bluetooth::DeviceClass *cod) {
        return ( (((uint8_t *)cod)[1] & 0x0f) == BTM_COD_MAJOR_PERIPHERAL) &&
               ( ((((uint8_t *)cod)[2] & 0x0f) == BTM_COD_MINOR_GAMEPAD) || 
               ( (((uint8_t *)cod)[2] & 0x0f) == BTM_COD_MINOR_JOYSTICK) );
    }

}
