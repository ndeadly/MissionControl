#include "switchcontroller.hpp"

namespace ams::controller {

    SwitchController::SwitchController(ControllerType type, const bluetooth::Address *address) 
     : m_type(type), 
       m_address(*address),
       m_battery(8) { 
        m_switchController = (type == ControllerType_Joycon) || (type == ControllerType_SwitchPro);
    }

    const bluetooth::Address& SwitchController::address(void) const {
        return m_address;
    }

    ControllerType SwitchController::type(void) {
        return m_type;
    }

    bool SwitchController::isSwitchController(void) {
        return m_switchController;
    }

    Result SwitchController::initialize(void) {
        return 0;
    }

}
