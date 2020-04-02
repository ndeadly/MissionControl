#pragma once

#include <switch.h>
#include "hidgamepad.hpp"

namespace mc::controller {

	struct WiimoteButtonData {
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

	union WiimoteReportData {
		struct {
			WiimoteButtonData   buttons;
			uint8_t             _unk;
		} report0x30;
	};

	class WiimoteController : public HidGamepad {

		public:
			static constexpr const HardwareID hardwareIds[] = { 
				{0x057e, 0x0306},  // Official wiimote
			};

			WiimoteController(HidInterfaceType iface) : HidGamepad(iface) {};

			Result receiveReport(const HidReport *report);

		private:
			void handleInputReport0x30(const WiimoteReportData *data);
			
	};

}
