#pragma once
#include "wiicontroller.hpp"
#include "switchcontroller.hpp"

namespace controller {

    union WiimoteReportData {
		struct {
			WiiButtonData   buttons;
			uint8_t         _unk;
		} report0x30;

        struct {
			WiiButtonData   buttons;
			uint8_t         _unk;
		} report0x31;
	};


    class WiimoteController : public WiiController {

        public:
            static constexpr const HardwareID hardwareIds[] = { 
                {0x057e, 0x0306},  // Official wiimote
            };

            WiimoteController(const BluetoothAddress *address);

            Result initialize(void);

            void convertReportFormat(const HidReport *inReport, HidReport *outReport);

        private:
            void handleInputReport0x30(const WiimoteReportData *src, SwitchReportData *dst);
            void handleInputReport0x31(const WiimoteReportData *src, SwitchReportData *dst);

    };

}
