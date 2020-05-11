#pragma once

#include <switch.h>
#include "gamepad/hidgamepad.hpp"

namespace mc::controller {

    union JoyconStickData {
        struct __attribute__ ((__packed__)) {
            uint16_t     x : 12;
            uint16_t       : 0;
            uint8_t        : 8;
        };

        struct __attribute__ ((__packed__)) {
            uint8_t        : 8;
            uint16_t       : 4;
            uint16_t     y : 12;
        };
    };

    struct JoyconButtonData {
        uint8_t Y              : 1;
        uint8_t X              : 1;
        uint8_t B              : 1;
        uint8_t A              : 1;
        uint8_t                : 2; // SR, SL (Right Joy)
        uint8_t R              : 1;
        uint8_t ZR             : 1;

        uint8_t minus          : 1;
        uint8_t plus           : 1;
        uint8_t rstick_press   : 1;
        uint8_t lstick_press   : 1;
        uint8_t home           : 1;
        uint8_t capture        : 1;
        uint8_t                : 0;

        uint8_t dpad_down      : 1;
        uint8_t dpad_up        : 1;
        uint8_t dpad_right     : 1;
        uint8_t dpad_left      : 1;
        uint8_t                : 2; // SR, SL (Left Joy)
        uint8_t L              : 1;
        uint8_t ZL             : 1;
    };

    union JoyconReportData {
        struct {
            uint8_t             conn_info      : 4;
            uint8_t             battery        : 4;
            uint8_t             timer;
            JoyconButtonData buttons;
            JoyconStickData  left_stick;
            JoyconStickData  right_stick;
        } report0x30;
    };

    class JoyconController : public HidGamepad {

        public:
            static constexpr const HardwareID hardwareIds[] = { 
                {0x057e, 2006},   // Official Joycon(L) Controller
                {0x057e, 2007},   // Official Joycon(R) Controller
            };

            JoyconController(HidInterfaceType iface) : HidGamepad(iface) {};

            Result receiveReport(const HidReport *report);
            
        private:
            void mapStickValues(JoystickPosition *dst, const JoyconStickData *src); 
            void handleInputReport0x30(const JoyconReportData *data);
            
    };

}
