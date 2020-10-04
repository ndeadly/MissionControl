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
#include "ouya_controller.hpp"
#include "gamestick_controller.hpp"
#include "gembox_controller.hpp"
#include "ipega_controller.hpp"
#include "xiaomi_controller.hpp"
#include "gamesir_controller.hpp"
#include "steelseries_controller.hpp"
#include "nvidia_shield_controller.hpp"
#include "8bitdo_controller.hpp"
#include "powera_controller.hpp"

namespace ams::controller {

    const constexpr char* pro_controller_name = "Pro Controller";
    const constexpr char* wii_controller_prefix = "Nintendo RVL";

    enum ControllerType {
        ControllerType_Switch,
        ControllerType_Wii,
        ControllerType_Dualshock4,
        ControllerType_XboxOne,
        ControllerType_Ouya,
        ControllerType_Gamestick,
        ControllerType_Gembox,
        ControllerType_Ipega,
        ControllerType_Xiaomi,
        ControllerType_Gamesir,
        ControllerType_Steelseries,
        ControllerType_NvidiaShield,
        ControllerType_8BitDo,
        ControllerType_PowerA,
        ControllerType_Unknown,
    };

    class UnknownController : public EmulatedSwitchController{
        public:
            UnknownController(const bluetooth::Address *address) 
            : EmulatedSwitchController(address) { 
                m_colours.buttons = {0xff, 0x00, 0x00};
            };
    };

    ControllerType Identify(const BluetoothDevicesSettings *device);
    bool IsAllowedDevice(const bluetooth::DeviceClass *cod);
    bool IsOfficialSwitchControllerName(const char *name, size_t size);
    
    void AttachHandler(const bluetooth::Address *address);
    void RemoveHandler(const bluetooth::Address *address);
    SwitchController *LocateHandler(const bluetooth::Address *address);

}
