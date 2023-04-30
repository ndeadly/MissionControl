/*
 * Copyright (c) 2020-2023 ndeadly
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
#include <string>

#include "switch_controller.hpp"
#include "wii_controller.hpp"
#include "dualshock3_controller.hpp"
#include "dualshock4_controller.hpp"
#include "dualsense_controller.hpp"
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
#include "mad_catz_controller.hpp"
#include "mocute_controller.hpp"
#include "razer_controller.hpp"
#include "icade_controller.hpp"
#include "lanshen_controller.hpp"
#include "atgames_controller.hpp"
#include "hyperkin_controller.hpp"
#include "betop_controller.hpp"

namespace ams::controller {

    const constexpr char* pro_controller_name = "Pro Controller";
    const constexpr char* wii_controller_prefix = "Nintendo RVL";

    enum ControllerType {
        ControllerType_Switch,
        ControllerType_Wii,
        ControllerType_Dualshock3,
        ControllerType_Dualshock4,
        ControllerType_Dualsense,
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
        ControllerType_MadCatz,
        ControllerType_Mocute,
        ControllerType_Razer,
        ControllerType_ICade,
        ControllerType_LanShen,
        ControllerType_AtGames,
        ControllerType_Hyperkin,
        ControllerType_Betop,
        ControllerType_Unknown,
    };

    class UnknownController : public EmulatedSwitchController {
        public:
            UnknownController(const bluetooth::Address *address, HardwareID id)
            : EmulatedSwitchController(address, id) { }
    };

    ControllerType Identify(const bluetooth::DevicesSettings *device);
    bool IsAllowedDeviceClass(const bluetooth::DeviceClass *cod);
    bool IsOfficialSwitchControllerName(const std::string& name);

    void AttachHandler(const bluetooth::Address *address);
    void RemoveHandler(const bluetooth::Address *address);
    std::shared_ptr<SwitchController> LocateHandler(const bluetooth::Address *address);

}
