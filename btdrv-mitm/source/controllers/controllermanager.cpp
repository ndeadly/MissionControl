#include "controllermanager.hpp"
#include <memory>
#include <mutex>
#include <vector>
#include <cstring>
#include <stratosphere.hpp>

#include "switchcontroller.hpp"
#include "joycon.hpp"
#include "switchpro.hpp"
#include "wiimote.hpp"
#include "wiiupro.hpp"
#include "dualshock4.hpp"
#include "xboxone.hpp"

#include "../btdrv_mitm_logging.hpp"

namespace ams::controller {

    namespace {

        os::Mutex g_controllerLock(false);
        std::vector<std::unique_ptr<SwitchController>> g_controllers;

    }

    bool IsValidSwitchControllerName(const char *name) {
        return std::strncmp(name, "Joy-Con (L)", 		sizeof(BluetoothName)) == 0 ||
               std::strncmp(name, "Joy-Con (R)", 		sizeof(BluetoothName)) == 0 ||
               std::strncmp(name, "Pro Controller", 	sizeof(BluetoothName)) == 0 ||
               std::strncmp(name, "Lic Pro Controller", sizeof(BluetoothName)) == 0 ||
               std::strncmp(name, "NES Controller", 	sizeof(BluetoothName)) == 0 ||
               std::strncmp(name, "HVC Controller", 	sizeof(BluetoothName)) == 0 ||
               std::strncmp(name, "SNES Controller", 	sizeof(BluetoothName)) == 0 ||
               std::strncmp(name, "NintendoGamepad", 	sizeof(BluetoothName)) == 0 ;
    }

    bool IsJoyCon(const char *name) {
        return std::strncmp(name, "Joy-Con (L)", 		sizeof(BluetoothName)) == 0 ||
               std::strncmp(name, "Joy-Con (R)", 		sizeof(BluetoothName)) == 0;
    }

    ControllerType identifyController(const BluetoothDevicesSettings *device) {

        for (auto hwId : JoyconController::hardwareIds) {
            if ( (device->vid == hwId.vid) && (device->pid == hwId.pid) ) {
                return ControllerType_Joycon;
            }
        }

        for (auto hwId : SwitchProController::hardwareIds) {
            if ( (device->vid == hwId.vid) && (device->pid == hwId.pid) ) {
                return ControllerType_SwitchPro;
            }
        }

        for (auto hwId : WiiUProController::hardwareIds) {
            if ( (device->vid == hwId.vid) && (device->pid == hwId.pid) ) {
                return ControllerType_WiiUPro;
            }
        }

        for (auto hwId : WiimoteController::hardwareIds) {
            if ( (device->vid == hwId.vid) && (device->pid == hwId.pid) ) {
                return ControllerType_Wiimote;
            }
        }

        for (auto hwId : Dualshock4Controller::hardwareIds) {
            if ( (device->vid == hwId.vid) && (device->pid == hwId.pid) ) {
                return ControllerType_Dualshock4;
            }
        }

        for (auto hwId : XboxOneController::hardwareIds) {
            if ( (device->vid == hwId.vid) && (device->pid == hwId.pid) ) {
                return ControllerType_XboxOne;
            }
        }

        // Handle the case where joycons have been assigned random hardware ids when paired via rails
        if (IsJoyCon(device->name)) {
            return ControllerType_Joycon;;
        }

        return ControllerType_Unknown;
    }

    SwitchController *locateController(const bluetooth::Address *address) {
        std::scoped_lock lk(g_controllerLock);

        for (auto it = g_controllers.begin(); it < g_controllers.end(); ++it) {
                if (bdcmp(&(*it)->address(), address)) {
                    return (*it).get();
                }
        }

        return nullptr;
    }

    void attachDeviceHandler(const bluetooth::Address *address) {
        std::scoped_lock lk(g_controllerLock);

        // Retrieve information about paired device
        BluetoothDevicesSettings device;
        R_ABORT_UNLESS(btdrvGetPairedDeviceInfo(address, &device));

        switch (identifyController(&device)) {
            case ControllerType_Joycon:
                g_controllers.push_back(std::make_unique<JoyconController>(address));
                BTDRV_LOG_FMT("Attached handler for JoyCon");
                break;
            case ControllerType_SwitchPro:
                g_controllers.push_back(std::make_unique<SwitchProController>(address));
                BTDRV_LOG_FMT("Attached handler for Switch Pro Controller");
                break;
            case ControllerType_Wiimote:
                g_controllers.push_back(std::make_unique<WiimoteController>(address));
                BTDRV_LOG_FMT("Attached handler for Wiimote");
                break;
            case ControllerType_WiiUPro:
                g_controllers.push_back(std::make_unique<WiiUProController>(address));
                break;
            case ControllerType_Dualshock4:
                g_controllers.push_back(std::make_unique<Dualshock4Controller>(address));
                break;
            case ControllerType_XboxOne:
                g_controllers.push_back(std::make_unique<XboxOneController>(address));
                break;
            default:
                BTDRV_LOG_FMT("[?] Unknown controller [%04x:%04x | %s]", device.vid, device.pid, device.name);
                return;
        }

        g_controllers.back()->initialize();
    }

    void removeDeviceHandler(const bluetooth::Address *address) {
        std::scoped_lock lk(g_controllerLock);

        for (auto it = g_controllers.begin(); it < g_controllers.end(); ++it) {
            if (bdcmp(&(*it)->address(), address)) {
                g_controllers.erase(it);
                return;
            }
        }
    }

}
