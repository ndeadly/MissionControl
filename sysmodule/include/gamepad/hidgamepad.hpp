#pragma once

#include <memory>
#include <cmath>
#include <switch.h>

#include "gamepad/bluetoothinterface.hpp"
#include "gamepad/abstractedpad.hpp"
#include "gamepad/hdls.hpp"

namespace mc::controller {

    enum ControllerType {
        //ControllerType_Joycon,
        ControllerType_SwitchPro,
        ControllerType_WiiUPro,
        ControllerType_Wiimote,
        ControllerType_Dualshock4,
        ControllerType_XboxOne,
        ControllerType_Unknown
    };

    struct HardwareID {
        uint16_t vid;
        uint16_t pid;
    };

    /* Convert an unsigned joystick value of arbitrary precision to int32 used by libnx */
    inline int32_t unsigned_to_signed(uint32_t x, uint8_t nbits) {
		return  x * UINT16_MAX / (powf(2, nbits) - 1) + INT16_MIN;
	}

    class HidGamepad {

        public:
            friend class ControllerManager;

            HidGamepad(HidInterfaceType type);
            virtual ~HidGamepad() {};

            //HidInterfaceType interfaceType(void) { return m_interface->type(); }

            virtual Result receiveReport(const HidReport *report) = 0;

            void setInnerDeadzone(float percentage);
            void setOuterDeadzone(float percentage);
        
        protected:
            uint16_t m_innerDeadzone;
            uint16_t m_outerDeadzone;

            std::unique_ptr<BluetoothInterface> m_btInterface;
            std::unique_ptr<VirtualController>	m_virtual;

            SwitchProGamepadState m_state;
        
    };

}
