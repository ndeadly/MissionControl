#pragma once
#include "switchcontroller.hpp"

namespace ams::controller {

    class JoyconController : public SwitchController {

        public:
            static constexpr const HardwareID hardwareIds[] = { 
                {0x057e, 0x2006},   // Official Joycon(L) Controller
                {0x057e, 0x2007},   // Official Joycon(R) Controller
            };

            JoyconController(const bluetooth::Address *address) 
                : SwitchController(ControllerType_Joycon, address) {};

    };

}
