#pragma once
#include "bluetoothcontroller.hpp"

namespace controller {

    enum WiiControllerLEDs {
		WiiControllerLEDs_P1 = 0x10,
		WiiControllerLEDs_P2 = 0x20,
		WiiControllerLEDs_P3 = 0x40,
		WiiControllerLEDs_P4 = 0x80,
	};

    struct WiiButtonData {
		uint8_t dpad_left	: 1;
		uint8_t dpad_right	: 1;
		uint8_t dpad_down	: 1;
		uint8_t dpad_up	    : 1;
		uint8_t plus		: 1;
		uint8_t				: 0;
		
		uint8_t two			: 1;
		uint8_t one 		: 1;
		uint8_t B			: 1;
		uint8_t A			: 1;
		uint8_t minus		: 1;
		uint8_t				: 2;
		uint8_t home 		: 1;
	};

    class WiiController : public BluetoothController {

        public:
            Result initialize(void);

        protected:
            WiiController(const BluetoothAddress *address, ControllerType type) : BluetoothController(address, type) {};

            Result writeMemory(const BluetoothAddress *address, uint32_t writeaddr, const uint8_t *data, uint8_t length);
            Result setReportMode(const BluetoothAddress *address, uint8_t mode);
            Result setPlayerLeds(const BluetoothAddress *address, uint8_t mask);

        private:
            //void mapStickValues(JoystickPosition *dst, const Dualshock4StickData *src);
            //void handleInputReport0x01(const Dualshock4ReportData *data);
            //void handleInputReport0x11(const Dualshock4ReportData *data); 
    };


}
