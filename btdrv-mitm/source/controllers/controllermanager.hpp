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
