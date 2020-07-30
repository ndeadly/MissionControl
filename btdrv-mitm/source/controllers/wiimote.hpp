#pragma once
#include "wiicontroller.hpp"

namespace ams::controller {

    class WiimoteController : public WiiController {

        public:
            static constexpr const HardwareID hardwareIds[] = { 
                {0x057e, 0x0306},  // Official wiimote
            };

            WiimoteController(const bluetooth::Address *address)
                : WiiController(ControllerType_Wiimote, address) { };

            Result initialize(void);
    };

}
