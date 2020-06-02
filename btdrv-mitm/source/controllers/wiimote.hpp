#pragma once
#include "wiicontroller.hpp"

namespace controller {

    class WiimoteController : public WiiController {

        public:
            static constexpr const HardwareID hardwareIds[] = { 
                {0x057e, 0x0306},  // Official wiimote
            };

            WiimoteController(const BluetoothAddress *address)  : WiiController(address, ControllerType_Wiimote) {};

        private:
        
    };

}
