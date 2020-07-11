#pragma once
#include "wiicontroller.hpp"

namespace ams::controller {



    class WiimoteController : public WiiController {

        public:
            static constexpr const HardwareID hardwareIds[] = { 
                {0x057e, 0x0306},  // Official wiimote
            };

            WiimoteController(const bluetooth::Address *address);

            Result initialize(void);

            void convertReportFormat(const bluetooth::HidReport *inReport, bluetooth::HidReport *outReport);

        private:
            void handleInputReport0x30(const WiiReportData *src, SwitchReportData *dst);
            void handleInputReport0x31(const WiiReportData *src, SwitchReportData *dst);

    };

}
