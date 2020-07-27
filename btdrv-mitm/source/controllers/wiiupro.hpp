#pragma once
#include "wiicontroller.hpp"

namespace ams::controller {

    struct WiiUProButtonData {
        uint8_t             : 1;
        uint8_t R           : 1;
        uint8_t plus        : 1;
        uint8_t home        : 1;
        uint8_t minus       : 1;
        uint8_t L           : 1;
        uint8_t dpad_down   : 1;
        uint8_t dpad_right  : 1;

        uint8_t dpad_up     : 1;
        uint8_t dpad_left   : 1;
        uint8_t ZR          : 1;
        uint8_t X           : 1; 
        uint8_t A           : 1;
        uint8_t Y           : 1;
        uint8_t B           : 1;
        uint8_t ZL          : 1;

        uint8_t rstick_press : 1;
        uint8_t lstick_press : 1;
        uint8_t : 0;
    } __attribute__ ((__packed__));

    struct WiiUProExtensionData {
        uint16_t left_stick_x;
        uint16_t right_stick_x;
        uint16_t left_stick_y;
        uint16_t right_stick_y;
        WiiUProButtonData buttons;
    } __attribute__ ((__packed__));

    class WiiUProController : public WiiController {

        public:
            static constexpr const HardwareID hardwareIds[] = { 
                {0x057e, 0x0330},  // Official Wii U Pro Controller
            };

            WiiUProController(const bluetooth::Address *address);

            void convertReportFormat(const bluetooth::HidReport *inReport, bluetooth::HidReport *outReport);

            Result initialize(void);

        private:
            Result sendInit1(void);
            Result sendInit2(void);

            void handleInputReport0x20(const WiiReportData *src, SwitchReportData *dst);
            void handleInputReport0x34(const WiiReportData *src, SwitchReportData *dst);

    };

}
