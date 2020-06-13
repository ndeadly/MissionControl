#include <memory>
#include <functional>
#include <queue>
#include <vector>
#include <vapours.hpp>

#include "controllermanager.hpp"
#include "controllers/switchcontroller.hpp"
#include "controllers/wiimote.hpp"
#include "controllers/wiiupro.hpp"
#include "controllers/dualshock4.hpp"
#include "controllers/xboxone.hpp"

#include "btdrv_mitm_logging.hpp"

namespace ams::mitm::btdrv {

    namespace {

        std::priority_queue<int, std::vector<int>, std::greater<int>> g_uniqueIds;
        std::vector<std::unique_ptr<controller::BluetoothController>> g_controllers;

    }

    void initUniqueIds(void) {
        for (int n: {1, 2, 3, 4, 5, 6, 7, 8}) {
            g_uniqueIds.push(n);
        }
    }

    bool uniqueIdAvailable(void) {
        return !g_uniqueIds.empty();
    }

    int acquireUniqueId(void) {
        if (uniqueIdAvailable()) {
            auto id = g_uniqueIds.top();
            g_uniqueIds.pop();
            return id;
        }

        return -1;
    }

    void releaseUniqueId(int id) {
        if (id > 0 && id <= 4)
            g_uniqueIds.push(id);
    }


    controller::ControllerType identifyController(uint16_t vid, uint16_t pid) {

        for (auto hwId : controller::JoyconController::hardwareIds) {
            if ( (vid == hwId.vid) && (pid == hwId.pid) ) {
                return controller::ControllerType_Joycon;
            }
        }

        for (auto hwId : controller::SwitchProController::hardwareIds) {
            if ( (vid == hwId.vid) && (pid == hwId.pid) ) {
                return controller::ControllerType_SwitchPro;
            }
        }

        for (auto hwId : controller::WiiUProController::hardwareIds) {
            if ( (vid == hwId.vid) && (pid == hwId.pid) ) {
                return controller::ControllerType_WiiUPro;
            }
        }

        for (auto hwId : controller::WiimoteController::hardwareIds) {
            if ( (vid == hwId.vid) && (pid == hwId.pid) ) {
                return controller::ControllerType_Wiimote;
            }
        }

        for (auto hwId : controller::Dualshock4Controller::hardwareIds) {
            if ( (vid == hwId.vid) && (pid == hwId.pid) ) {
                return controller::ControllerType_Dualshock4;
            }
        }

        for (auto hwId : controller::XboxOneController::hardwareIds) {
            if ( (vid == hwId.vid) && (pid == hwId.pid) ) {
                return controller::ControllerType_XboxOne;
            }
        }

        return controller::ControllerType_Unknown;
    }


    controller::BluetoothController *locateController(const BluetoothAddress *address) {

        for (auto it = g_controllers.begin(); it < g_controllers.end(); ++it) {
                if (controller::bdcmp(&(*it)->address(), address)) {
                    return (*it).get();
                }
        }

        return nullptr;
    }


    void attachDeviceHandler(const BluetoothAddress *address) {
        // Retrieve information about paired device
        BluetoothDevicesSettings device;
        R_ABORT_UNLESS(btdrvGetPairedDeviceInfo(address, &device));

        //BTDRV_LOG_FMT(" vid/pid: %04x:%04x", device.vid, device.pid);

        switch (identifyController(device.vid, device.pid)) {
            case controller::ControllerType_Joycon:
                BTDRV_LOG_FMT(" Joycon controller");
                g_controllers.push_back(std::make_unique<controller::JoyconController>(address));
                break;
            case controller::ControllerType_SwitchPro:
                BTDRV_LOG_FMT(" Switch pro controller");
                g_controllers.push_back(std::make_unique<controller::SwitchProController>(address));
                break;
            case controller::ControllerType_Wiimote:
                BTDRV_LOG_FMT(" Wiimote controller");
                g_controllers.push_back(std::make_unique<controller::WiimoteController>(address));
                break;
            case controller::ControllerType_WiiUPro:
                BTDRV_LOG_FMT(" Wii U pro controller");
                g_controllers.push_back(std::make_unique<controller::WiiUProController>(address));
                break;
            case controller::ControllerType_Dualshock4:
                BTDRV_LOG_FMT(" Dualshock4 controller");
                g_controllers.push_back(std::make_unique<controller::Dualshock4Controller>(address));
                break;
            case controller::ControllerType_XboxOne:
                BTDRV_LOG_FMT(" Xbox one controller");
                g_controllers.push_back(std::make_unique<controller::XboxOneController>(address));
                break;
            default:
                BTDRV_LOG_FMT(" Unknown controller");
                // Disconnect unknown controller
                btdrvCloseHidConnection(address);
                btdrvRemoveBond(address);
                return;
        }

        g_controllers.back()->initialize();
    }


    void removeDeviceHandler(const BluetoothAddress *address) {

        for (auto it = g_controllers.begin(); it < g_controllers.end(); ++it) {
            if (controller::bdcmp(&(*it)->address(), address)) {
                g_controllers.erase(it);
                return;
            }
        }
    }


}
