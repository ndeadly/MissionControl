#pragma once

#include <memory>
#include <vector>
#include <switch.h>
#include "gamepad/bluetoothdatabase.hpp"
#include "gamepad/controllers.hpp"

namespace mc::controller {

    class ControllerManager {

        public:
            ControllerManager();
            ~ControllerManager();
            
            static ControllerType identify(uint16_t vid, uint16_t pid);
            static ControllerType identify(const HardwareID *hwId);
            static ControllerType identify(const BluetoothDevice *device);

            Result registerBluetoothControllers(void);
            Result attachBluetoothController(const BluetoothAddress *address);
            Result removeBluetoothController(const BluetoothAddress *address);
            Result receiveBluetoothReport(const BluetoothAddress *address, const HidReport *report);

            //Result attachUsbController();
            //Result removeUsbController();
            //Result receiveUsbReport(, const HidReport *report);

            void removeControllers(void);

        private:
            std::unique_ptr<BluetoothDatabase>          m_database;
            std::vector<std::unique_ptr<HidGamepad>> m_controllers;

    };

}
