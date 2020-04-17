#pragma once

#include <switch.h>
#include "gamepad/controllers/wiimote.hpp"
#include "gamepad/hidgamepad.hpp"

namespace mc::controller {

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
    };

    union WiiUProReportData {
        struct {
            WiimoteButtonData core_buttons;

            /*
            uint8_t left_stick_x    : 6;
            uint8_t right_stick_x2  : 2;

            uint8_t left_stick_y    : 6;
            uint8_t right_stick_x1  : 2;

            uint8_t right_stick_x0  : 1;
            uint8_t left_trigger_1  : 2;
            uint8_t right_stick_y   : 5;

            uint8_t left_trigger_0  : 3;
            uint8_t right_trigger   : 5;
            */
            uint16_t left_stick_x;
            uint16_t right_stick_x;
            uint16_t left_stick_y;
            uint16_t right_stick_y;
            WiiUProButtonData buttons;
        } report0x34;
    };

    class WiiUProController : public HidGamepad {

        public:
            static constexpr const HardwareID hardwareIds[] = { 
                {0x057e, 0x0330},  // Official Wii U Pro Controller
            };

            WiiUProController(HidInterfaceType iface);

            Result receiveReport(const HidReport *report);

        private:
            void mapStickValues(JoystickPosition *dst, uint16_t x, uint16_t y);
            void handleInputReport0x20(const WiiUProReportData *data);  
            void handleInputReport0x34(const WiiUProReportData *data);
            
    };

}
