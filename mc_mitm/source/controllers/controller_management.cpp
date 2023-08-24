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
#include "controller_management.hpp"
#include <stratosphere.hpp>
#include "../utils.hpp"

namespace ams::controller {

    namespace {

        const std::string official_npad_names[] = {
            "NintendoGamepad",
            "Joy-Con (L)",
            "Joy-Con (R)",
            "Pro Controller",
            "Lic Pro Controller",
            "NES Controller",
            "HVC Controller",
            "SNES Controller",
            "N64 Controller",
            "MD/Gen Control Pad",
            "Lic2 Pro Controller",
        };

        constexpr auto cod_major_peripheral = 0x05;
        constexpr auto cod_minor_gamepad    = 0x08;
        constexpr auto cod_minor_joystick   = 0x04;
        constexpr auto cod_minor_keyboard   = 0x40;

        os::Mutex g_controller_lock(false);
        std::vector<std::shared_ptr<SwitchController>> g_controllers;

    }

    ControllerType Identify(const bluetooth::DevicesSettings *device) {

        for (auto hwId : SwitchController::hardware_ids) {
            if ( (device->vid == hwId.vid) && (device->pid == hwId.pid) ) {
                return ControllerType_Switch;
            }
        }

        // Additionally check controller name against known official Nintendo controllers, as some controllers (eg. JoyCons paired via rails) don't report the correct vid/pid
        if (IsOfficialSwitchControllerName(hos::GetVersion() < hos::Version_13_0_0 ? device->name.name : device->name2))
            return ControllerType_Switch;

        for (auto hwId : WiiController::hardware_ids) {
            if ( (device->vid == hwId.vid) && (device->pid == hwId.pid) ) {
                return ControllerType_Wii;
            }
        }

        for (auto hwId : Dualshock3Controller::hardware_ids) {
            if ( (device->vid == hwId.vid) && (device->pid == hwId.pid) ) {
                return ControllerType_Dualshock3;
            }
        }

        for (auto hwId : Dualshock4Controller::hardware_ids) {
            if ( (device->vid == hwId.vid) && (device->pid == hwId.pid) ) {
                return ControllerType_Dualshock4;
            }
        }

        for (auto hwId : DualsenseController::hardware_ids) {
            if ( (device->vid == hwId.vid) && (device->pid == hwId.pid) ) {
                return ControllerType_Dualsense;
            }
        }

        for (auto hwId : XboxOneController::hardware_ids) {
            if ( (device->vid == hwId.vid) && (device->pid == hwId.pid) ) {
                return ControllerType_XboxOne;
            }
        }

        for (auto hwId : OuyaController::hardware_ids) {
            if ( (device->vid == hwId.vid) && (device->pid == hwId.pid) ) {
                return ControllerType_Ouya;
            }
        }

        for (auto hwId : GamestickController::hardware_ids) {
            if ( (device->vid == hwId.vid) && (device->pid == hwId.pid) ) {
                return ControllerType_Gamestick;
            }
        }

        for (auto hwId : GemboxController::hardware_ids) {
            if ( (device->vid == hwId.vid) && (device->pid == hwId.pid) ) {
                return ControllerType_Gembox;
            }
        }

        for (auto hwId : IpegaController::hardware_ids) {
            if ( (device->vid == hwId.vid) && (device->pid == hwId.pid) ) {
                return ControllerType_Ipega;
            }
        }

        for (auto hwId : XiaomiController::hardware_ids) {
            if ( (device->vid == hwId.vid) && (device->pid == hwId.pid) ) {
                return ControllerType_Xiaomi;
            }
        }

        for (auto hwId : GamesirController::hardware_ids) {
            if ( (device->vid == hwId.vid) && (device->pid == hwId.pid) ) {
                return ControllerType_Gamesir;
            }
        }

        for (auto hwId : SteelseriesController::hardware_ids) {
            if ( (device->vid == hwId.vid) && (device->pid == hwId.pid) ) {
                return ControllerType_Steelseries;
            }
        }

        for (auto hwId : NvidiaShieldController::hardware_ids) {
            if ( (device->vid == hwId.vid) && (device->pid == hwId.pid) ) {
                return ControllerType_NvidiaShield;
          }
        }

        for (auto hwId : EightBitDoController::hardware_ids) {
            if ( (device->vid == hwId.vid) && (device->pid == hwId.pid) ) {
                return ControllerType_8BitDo;
            }
        }

        for (auto hwId : PowerAController::hardware_ids) {
            if ( (device->vid == hwId.vid) && (device->pid == hwId.pid) ) {
                return ControllerType_PowerA;
            }
        }

        for (auto hwId : MadCatzController::hardware_ids) {
            if ( (device->vid == hwId.vid) && (device->pid == hwId.pid) ) {
                return ControllerType_MadCatz;
            }
        }

        for (auto hwId : MocuteController::hardware_ids) {
            if ( (device->vid == hwId.vid) && (device->pid == hwId.pid) ) {
                return ControllerType_Mocute;
            }
        }

        for (auto hwId : RazerController::hardware_ids) {
            if ( (device->vid == hwId.vid) && (device->pid == hwId.pid) ) {
                return ControllerType_Razer;
            }
        }

        for (auto hwId : ICadeController::hardware_ids) {
            if ( (device->vid == hwId.vid) && (device->pid == hwId.pid) ) {
                return ControllerType_ICade;
            }
        }

        for (auto hwId : LanShenController::hardware_ids) {
            if ( (device->vid == hwId.vid) && (device->pid == hwId.pid) ) {
                return ControllerType_LanShen;
            }
        }

        for (auto hwId : AtGamesController::hardware_ids) {
            if ( (device->vid == hwId.vid) && (device->pid == hwId.pid) ) {
                return ControllerType_AtGames;
            }
        }

        for (auto hwId : HyperkinController::hardware_ids) {
            if ( (device->vid == hwId.vid) && (device->pid == hwId.pid) ) {
                return ControllerType_Hyperkin;
            }
        }

        for (auto hwId : BetopController::hardware_ids) {
            if ( (device->vid == hwId.vid) && (device->pid == hwId.pid) ) {
                return ControllerType_Betop;
            }
        }

        return ControllerType_Unknown;
    }

    bool IsAllowedDeviceClass(const bluetooth::DeviceClass *cod) {
        return ((cod->class_of_device[1] & 0x0f) == cod_major_peripheral) &&
               (((cod->class_of_device[2] & 0x0f) == cod_minor_gamepad) || ((cod->class_of_device[2] & 0x0f) == cod_minor_joystick) || ((cod->class_of_device[2] & 0x40) == cod_minor_keyboard));
    }

    bool IsOfficialSwitchControllerName(const std::string& name) {
        for (auto n : official_npad_names) {
            if (name.rfind(n, 0) == 0)
                return true;
        }

        return false;
    }

    void AttachHandler(const bluetooth::Address *address) {
        bluetooth::DevicesSettings device_settings;
        R_ABORT_UNLESS(btdrvGetPairedDeviceInfo(*address, &device_settings));

        HardwareID id = { device_settings.vid, device_settings.pid };

        std::shared_ptr<SwitchController> controller;

        switch (Identify(&device_settings)) {
            case ControllerType_Switch:
                controller = std::make_shared<SwitchController>(address, id);
                break;
            case ControllerType_Wii:
                controller = std::make_shared<WiiController>(address, id);
                break;
            case ControllerType_Dualshock3:
                controller = std::make_shared<Dualshock3Controller>(address, id);
                break;
            case ControllerType_Dualshock4:
                controller = std::make_shared<Dualshock4Controller>(address, id);
                break;
            case ControllerType_Dualsense:
                controller = std::make_shared<DualsenseController>(address, id);
                break;
            case ControllerType_XboxOne:
                controller = std::make_shared<XboxOneController>(address, id);
                break;
            case ControllerType_Ouya:
                controller = std::make_shared<OuyaController>(address, id);
                break;
            case ControllerType_Gamestick:
                controller = std::make_shared<GamestickController>(address, id);
                break;
            case ControllerType_Gembox:
                controller = std::make_shared<GemboxController>(address, id);
                break;
            case ControllerType_Ipega:
                controller = std::make_shared<IpegaController>(address, id);
                break;
            case ControllerType_Xiaomi:
                controller = std::make_shared<XiaomiController>(address, id);
                break;
            case ControllerType_Gamesir:
                controller = std::make_shared<GamesirController>(address, id);
                break;
            case ControllerType_Steelseries:
                controller = std::make_shared<SteelseriesController>(address, id);
                break;
            case ControllerType_NvidiaShield:
                controller = std::make_shared<NvidiaShieldController>(address, id);
                break;
            case ControllerType_8BitDo:
                controller = std::make_shared<EightBitDoController>(address, id);
                break;
            case ControllerType_PowerA:
                controller = std::make_shared<PowerAController>(address, id);
                break;
            case ControllerType_MadCatz:
                controller = std::make_shared<MadCatzController>(address, id);
                break;
            case ControllerType_Mocute:
                controller = std::make_shared<MocuteController>(address, id);
                break;
            case ControllerType_Razer:
                controller = std::make_shared<RazerController>(address, id);
                break;
            case ControllerType_ICade:
                controller = std::make_shared<ICadeController>(address, id);
                break;
            case ControllerType_LanShen:
                controller = std::make_shared<LanShenController>(address, id);
                break;
            case ControllerType_AtGames:
                controller = std::make_shared<AtGamesController>(address, id);
                break;
            case ControllerType_Hyperkin:
                controller = std::make_shared<HyperkinController>(address, id);
                break;
            case ControllerType_Betop:
                controller = std::make_shared<BetopController>(address, id);
                break;
            default:
                controller = std::make_shared<UnknownController>(address, id);
                break;
        }

        {
            std::scoped_lock lk(g_controller_lock);
            g_controllers.push_back(controller);
        }

        if (R_FAILED(controller->Initialize())) {
            // Try to disconnect the controller
            btdrvCloseHidConnection(controller->Address());
        }
    }

    void RemoveHandler(const bluetooth::Address *address) {
        std::scoped_lock lk(g_controller_lock);

        for (auto it = g_controllers.begin(); it < g_controllers.end(); ++it) {
            if (utils::BluetoothAddressCompare(&(*it)->Address(), address)) {
                g_controllers.erase(it);
                return;
            }
        }
    }

    std::shared_ptr<SwitchController> LocateHandler(const bluetooth::Address *address) {
        std::scoped_lock lk(g_controller_lock);

        for (auto it = g_controllers.begin(); it < g_controllers.end(); ++it) {
            if (utils::BluetoothAddressCompare(&(*it)->Address(), address)) {
                return (*it);
            }
        }

        return nullptr;
    }

}
