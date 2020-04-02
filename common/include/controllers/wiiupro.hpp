#pragma once

#include <switch.h>
#include "hidgamepad.hpp"

namespace mc::controller {

    class WiiUProController : public HidGamepad {

        public:
            static constexpr const HardwareID hardwareIds[] = { 
                {0x057e, 0x0330},  // Official Wii U Pro Controller
            };

            WiiUProController(HidInterfaceType iface) : HidGamepad(iface) {};

            Result receiveReport(const HidReport *report);

        private:
            
    };

}
