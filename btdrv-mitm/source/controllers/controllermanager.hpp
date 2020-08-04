#pragma once
#include <switch.h>
#include "switchcontroller.hpp"

#define BTM_COD_MAJOR_PERIPHERAL    0x05
#define BTM_COD_MINOR_GAMEPAD       0x08
#define BTM_COD_MINOR_JOYSTICK      0x04

namespace ams::controller {

    const constexpr char* proControllerName = "Pro Controller";

    ControllerType identifyController(uint16_t vid, uint16_t pid);

    bool IsValidSwitchControllerName(const char *name);
    bool IsJoyCon(const char *name);

    SwitchController *locateController(const bluetooth::Address *address);

    void attachDeviceHandler(const bluetooth::Address *address);
    void removeDeviceHandler(const bluetooth::Address *address);

    inline bool IsController(const bluetooth::DeviceClass *cod) {
        return ( (((uint8_t *)cod)[1] & 0x0f) == BTM_COD_MAJOR_PERIPHERAL) &&
               ( ((((uint8_t *)cod)[2] & 0x0f) == BTM_COD_MINOR_GAMEPAD) || 
               ( (((uint8_t *)cod)[2] & 0x0f) == BTM_COD_MINOR_JOYSTICK) );
    }

}
