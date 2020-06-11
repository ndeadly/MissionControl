#pragma once
#include <switch.h>
#include "controllers/bluetoothcontroller.hpp"


namespace ams::mitm::btdrv {

    controller::ControllerType identifyController(uint16_t vid, uint16_t pid);
    controller::BluetoothController *locateController(const BluetoothAddress *address);

    void attachDeviceHandler(const BluetoothAddress *address);
    void removeDeviceHandler(const BluetoothAddress *address);

}
