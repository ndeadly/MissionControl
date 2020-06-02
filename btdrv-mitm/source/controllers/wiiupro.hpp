#pragma once
#include "wiicontroller.hpp"

namespace controller {

    class WiiUProController : public WiiController {

        public:
            static constexpr const HardwareID hardwareIds[] = { 
                {0x057e, 0x0330},  // Official Wii U Pro Controller
            };

            WiiUProController(const BluetoothAddress *address) : WiiController(address, ControllerType_WiiUPro) {};

            Result initialize(void);

        private:
            Result sendInit1(const BluetoothAddress *address);
            Result sendInit2(const BluetoothAddress *address);

    };

}
