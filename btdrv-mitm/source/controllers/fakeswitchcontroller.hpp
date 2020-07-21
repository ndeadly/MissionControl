#pragma once
#include "switchcontroller.hpp"

namespace ams::controller {

    class FakeSwitchController : public SwitchController {

        public:
            FakeSwitchController(ControllerType type, const bluetooth::Address *address) 
            : SwitchController(type, address) {};
            
            //readHidData();
            //writeHidData();

        private:

    };

}
