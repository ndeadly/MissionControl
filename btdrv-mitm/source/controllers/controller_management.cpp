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
#include "controller_management.hpp"
#include <stratosphere.hpp>
#include <memory>
#include <mutex>
#include <vector>
#include <cstring>

namespace ams::controller {

    namespace {

        constexpr auto cod_major_peripheral  = 0x05;
        constexpr auto cod_minor_gamepad     = 0x08;
        constexpr auto cod_minor_joystick    = 0x04;

        os::Mutex g_controller_lock(false);
        std::vector<std::unique_ptr<SwitchController>> g_controllers;

        inline bool bdcmp(const bluetooth::Address *addr1, const bluetooth::Address *addr2) {
            return std::memcmp(addr1, addr2, sizeof(bluetooth::Address)) == 0;
        }

    }

    ControllerType Identify(const BluetoothDevicesSettings *device) {
        for (auto hwId : SwitchController::hardware_ids) {
            if ( (device->vid == hwId.vid) && (device->pid == hwId.pid) ) {
                return ControllerType_Switch;
            }
        }

        for (auto hwId : WiiController::hardware_ids) {
            if ( (device->vid == hwId.vid) && (device->pid == hwId.pid) ) {
                return ControllerType_Wii;
            }
        }

        for (auto hwId : Dualshock4Controller::hardware_ids) {
            if ( (device->vid == hwId.vid) && (device->pid == hwId.pid) ) {
                return ControllerType_Dualshock4;
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

        // Handle the case where joycons have been assigned random hardware ids when paired via rails
        if (IsJoyCon(device->name)) {
            return ControllerType_Switch;;
        }

        return ControllerType_Unknown;
    }

    bool IsGamepad(const bluetooth::DeviceClass *cod) {
        return ((cod->cod[1] & 0x0f) == cod_major_peripheral) &&
               (((cod->cod[2] & 0x0f) == cod_minor_gamepad) || ((cod->cod[2] & 0x0f) == cod_minor_joystick));
    }

    bool IsJoyCon(const char *name) {
        return std::strncmp(name, "Joy-Con (L)", 		sizeof(BluetoothName)) == 0 ||
               std::strncmp(name, "Joy-Con (R)", 		sizeof(BluetoothName)) == 0;
    }

    bool IsOfficialSwitchControllerName(const char *name) {
        return std::strncmp(name, "Joy-Con (L)", 		sizeof(BluetoothName)) == 0 ||
               std::strncmp(name, "Joy-Con (R)", 		sizeof(BluetoothName)) == 0 ||
               std::strncmp(name, "Pro Controller", 	sizeof(BluetoothName)) == 0 ||
               std::strncmp(name, "Lic Pro Controller", sizeof(BluetoothName)) == 0 ||
               std::strncmp(name, "NES Controller", 	sizeof(BluetoothName)) == 0 ||
               std::strncmp(name, "HVC Controller", 	sizeof(BluetoothName)) == 0 ||
               std::strncmp(name, "SNES Controller", 	sizeof(BluetoothName)) == 0 ||
               std::strncmp(name, "NintendoGamepad", 	sizeof(BluetoothName)) == 0 ;
    }

    void AttachHandler(const bluetooth::Address *address) {
        std::scoped_lock lk(g_controller_lock);

        BluetoothDevicesSettings device;
        R_ABORT_UNLESS(btdrvGetPairedDeviceInfo(address, &device));

        switch (Identify(&device)) {
            case ControllerType_Switch:
                g_controllers.push_back(std::make_unique<SwitchController>(address));
                break;
            case ControllerType_Wii:
                g_controllers.push_back(std::make_unique<WiiController>(address));
                break;
            case ControllerType_Dualshock4:
                g_controllers.push_back(std::make_unique<Dualshock4Controller>(address));
                break;
            case ControllerType_XboxOne:
                g_controllers.push_back(std::make_unique<XboxOneController>(address));
                break;
            case ControllerType_Ouya:
                g_controllers.push_back(std::make_unique<OuyaController>(address));
                break;
            case ControllerType_Gamestick:
                g_controllers.push_back(std::make_unique<GamestickController>(address));
                break;
            case ControllerType_Gembox:
                g_controllers.push_back(std::make_unique<GemboxController>(address));
                break;
            default:
                return;
        }

        g_controllers.back()->Initialize();
    }

    void RemoveHandler(const bluetooth::Address *address) {
        std::scoped_lock lk(g_controller_lock);

        for (auto it = g_controllers.begin(); it < g_controllers.end(); ++it) {
            if (bdcmp(&(*it)->Address(), address)) {
                g_controllers.erase(it);
                return;
            }
        }
    }

    SwitchController *LocateHandler(const bluetooth::Address *address) {
        std::scoped_lock lk(g_controller_lock);

        for (auto it = g_controllers.begin(); it < g_controllers.end(); ++it) {
                if (bdcmp(&(*it)->Address(), address)) {
                    return (*it).get();
                }
        }

        return nullptr;
    }

}
