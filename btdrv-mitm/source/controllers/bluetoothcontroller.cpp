#include "bluetoothcontroller.hpp"
#include "../btdrv_mitm_logging.hpp"

namespace controller {

    BluetoothController::BluetoothController(const BluetoothAddress *address, ControllerType type) : m_address(*address), m_type(type) { 
        m_switchController = (type == ControllerType_Joycon) || (type == ControllerType_SwitchPro);
    }

    const BluetoothAddress& BluetoothController::address(void) const {
        return m_address;
    }

    ControllerType BluetoothController::type(void) {
        return m_type;
    }

    bool BluetoothController::isSwitchController(void) {
        return m_switchController;
    }

    Result BluetoothController::initialize(void) {
        BTDRV_LOG_FMT("BluetoothController::initialize");
        return 0;
    }

}
