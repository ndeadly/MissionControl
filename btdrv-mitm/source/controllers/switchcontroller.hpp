#pragma once
#include "bluetoothcontroller.hpp"

namespace controller {

    /*
    class SwitchController {

        public:

        private:

    };
    */

    class SwitchProController : public BluetoothController {

        public:
            static constexpr const HardwareID hardwareIds[] = { 
                {0x057e, 0x2009}   // Official Switch Pro Controller
            };

            SwitchProController(const BluetoothAddress *address) : BluetoothController(address, ControllerType_SwitchPro) {};

    };

    class JoyconController : public BluetoothController {

        public:
            static constexpr const HardwareID hardwareIds[] = { 
                {0x057e, 2006},   // Official Joycon(L) Controller
                {0x057e, 2007},   // Official Joycon(R) Controller
            };

            JoyconController(const BluetoothAddress *address) : BluetoothController(address, ControllerType_Joycon) {};

    };

}
