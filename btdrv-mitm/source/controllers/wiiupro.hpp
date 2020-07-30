#pragma once
#include "wiicontroller.hpp"

namespace ams::controller {

    class WiiUProController : public WiiController {

        public:
            static constexpr const HardwareID hardwareIds[] = { 
                {0x057e, 0x0330},  // Official Wii U Pro Controller
            };

            WiiUProController(const bluetooth::Address *address) 
                : WiiController(ControllerType_WiiUPro, address) { };
    };

}
