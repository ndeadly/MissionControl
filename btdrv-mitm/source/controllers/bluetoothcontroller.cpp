#include "bluetoothcontroller.hpp"
#include "../btdrv_mitm_logging.hpp"

namespace ams::controller {

    BluetoothController::BluetoothController(ControllerType type, const bluetooth::Address *address) : m_type(type), m_address(*address) { 
        m_switchController = (type == ControllerType_Joycon) || (type == ControllerType_SwitchPro);
    }

    const bluetooth::Address& BluetoothController::address(void) const {
        return m_address;
    }

    ControllerType BluetoothController::type(void) {
        return m_type;
    }

    bool BluetoothController::isSwitchController(void) {
        return m_switchController;
    }

    Result BluetoothController::initialize(void) {
        return 0;
    }

}
