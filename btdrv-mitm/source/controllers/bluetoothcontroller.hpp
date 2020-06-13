#pragma once
#include <memory>
#include <cstring>
#include <switch.h>

#include "virtualcontroller.hpp"

namespace controller {

    struct HardwareID {
        uint16_t vid;
        uint16_t pid;
    };

    enum ControllerType {
        ControllerType_Unknown,
        ControllerType_Joycon,
        ControllerType_SwitchPro,
        ControllerType_Wiimote,
        ControllerType_WiiUPro,
        ControllerType_Dualshock4,
        ControllerType_XboxOne,
    };

    class BluetoothController {

        public:
            const BluetoothAddress& address(void) const;
            ControllerType type(void);
            bool isSwitchController(void);

            virtual Result initialize(void);
            virtual void convertReportFormat(const HidReport *inReport, HidReport *outReport) {};

        protected:
            BluetoothController(ControllerType type, const BluetoothAddress *address);

            ControllerType m_type;
            BluetoothAddress m_address;
        
            bool m_switchController;
            std::unique_ptr<VirtualController> m_virtualController;

    };

    inline bool bdcmp(const BluetoothAddress *addr1, const BluetoothAddress *addr2) {
        return std::memcmp(addr1, addr2, sizeof(BluetoothAddress)) == 0;
    }

}
