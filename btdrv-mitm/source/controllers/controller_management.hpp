/*
 * Copyright (c) 2020 ndeadly
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
#include "switch_controller.hpp"
#include "wii_controller.hpp"
#include "dualshock4_controller.hpp"
#include "xbox_one_controller.hpp"

namespace ams::controller {

    const constexpr char* pro_controller_name = "Pro Controller";
    const constexpr char* wii_controller_prefix = "Nintendo RVL";

    enum ControllerType {
        ControllerType_Switch,
        ControllerType_Wii,
        ControllerType_Dualshock4,
        ControllerType_XboxOne,
        ControllerType_Unknown,
    };

    ControllerType Identify(const BluetoothDevicesSettings *device);
    bool IsGamepad(const bluetooth::DeviceClass *cod);
    bool IsJoyCon(const char *name);
    bool IsOfficialSwitchControllerName(const char *name);
    
    void AttachHandler(const bluetooth::Address *address);
    void RemoveHandler(const bluetooth::Address *address);
    SwitchController *LocateHandler(const bluetooth::Address *address);

}
