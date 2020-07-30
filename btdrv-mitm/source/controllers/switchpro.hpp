#pragma once
#include "switchcontroller.hpp"

namespace ams::controller {

    class SwitchProController : public SwitchController {

        public:
            static constexpr const HardwareID hardwareIds[] = { 
                {0x057e, 0x2009}   // Official Switch Pro Controller
            };

            SwitchProController(const bluetooth::Address *address) 
                : SwitchController(ControllerType_SwitchPro, address) { };

    };

}
