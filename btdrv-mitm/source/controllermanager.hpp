#pragma once
#include <switch.h>
#include "controllers/switchcontroller.hpp"

#define BTM_COD_MAJOR_PERIPHERAL    0x05
#define BTM_COD_MINOR_GAMEPAD       0x08
#define BTM_COD_MINOR_JOYSTICK      0x04

namespace ams::mitm::btdrv {

    controller::ControllerType identifyController(uint16_t vid, uint16_t pid);
    controller::SwitchController *locateController(const bluetooth::Address *address);

    bool IsValidSwitchControllerName(const char *name);

    void attachDeviceHandler(const bluetooth::Address *address);
    void removeDeviceHandler(const bluetooth::Address *address);

    inline bool IsController(const BluetoothDeviceClass *cod) {
        return ( (((uint8_t *)cod)[1] & 0x0f) == BTM_COD_MAJOR_PERIPHERAL) &&
               ( ((((uint8_t *)cod)[2] & 0x0f) == BTM_COD_MINOR_GAMEPAD) || 
               ( (((uint8_t *)cod)[2] & 0x0f) == BTM_COD_MINOR_JOYSTICK) );
    }

}
