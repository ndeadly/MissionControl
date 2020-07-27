#include "switchcontroller.hpp"

namespace ams::controller {

    SwitchController::SwitchController(ControllerType type, const bluetooth::Address *address) 
     : m_type(type), 
       m_address(*address),
       m_charging(false),
       m_battery(BATTERY_MAX),
       m_switchController((type == ControllerType_Joycon) || (type == ControllerType_SwitchPro)) {}

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
        return ams::ResultSuccess();
    }

    const bluetooth::HidReport * SwitchController::handleIncomingReport(const bluetooth::HidReport *report) {
        return report;
    }

    const bluetooth::HidReport * SwitchController::handleOutgoingReport(const bluetooth::HidReport *report) {
        return report;
    }

}
