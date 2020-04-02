#pragma once

#include <switch.h>
#include "hidgamepad.hpp"

namespace mc::controller {

    enum Dualshock4ControllerVariant {
        Dualshock4ControllerVariant_V1,
        Dualshock4ControllerVariant_V2,
        Dualshock4ControllerVariant_Unknown
    };

    enum Dualshock4DPadDirection {
        Dualshock4DPad_N,
        Dualshock4DPad_NE,
        Dualshock4DPad_E,
        Dualshock4DPad_SE,
        Dualshock4DPad_S,
        Dualshock4DPad_SW,
        Dualshock4DPad_W,
        Dualshock4DPad_NW,
        Dualshock4DPad_Released
    };

    struct Dualshock4StickData {
        uint8_t x;
        uint8_t y;
    };

    struct Dualshock4ButtonData {
        uint8_t dpad       : 4;
        uint8_t square     : 1;
        uint8_t cross      : 1;
        uint8_t circle     : 1;
        uint8_t triangle   : 1;
        
        uint8_t L1         : 1;
        uint8_t R1         : 1;
        uint8_t L2         : 1;
        uint8_t R2         : 1;
        uint8_t share      : 1;
        uint8_t options    : 1;
        uint8_t L3         : 1;
        uint8_t R3         : 1;
        
        uint8_t ps         : 1;
        uint8_t tpad       : 1;
        uint8_t counter    : 6;
    };

    union Dualshock4ReportData {
        struct {
            Dualshock4StickData     left_stick;
            Dualshock4StickData     right_stick;
            Dualshock4ButtonData    buttons;
            uint8_t                 left_trigger;
            uint8_t                 right_trigger;
        } report0x01;

        struct {
            Dualshock4StickData     left_stick;
            Dualshock4StickData     right_stick;
            Dualshock4ButtonData    buttons;
            uint8_t                 left_trigger;
            uint8_t                 right_trigger;
            uint16_t timestamp;
            uint8_t battery;
            uint16_t vel_x;
            uint16_t vel_y;
            uint16_t vel_z;
            uint16_t acc_x;
            uint16_t acc_y;
            uint16_t acc_z;
            uint32_t _unk0;

            uint8_t battery_level    : 4;
            uint8_t usb              : 1;
            uint8_t mic              : 1;
            uint8_t phone            : 1;
            uint8_t                  : 0;

            uint16_t _unk2;
            uint8_t tpad_packets;
            uint8_t packet_counter;
        } report0x11;
    };

    class Dualshock4Controller : public HidGamepad {

        public:
            static constexpr const HardwareID hardwareIds[] = { 
                {0x054c, 0x05c4},  // Official Dualshock4 v1
                {0x054c, 0x09cc}   // Official Dualshock4 v2
            };

            Dualshock4Controller(HidInterfaceType iface);

            Result receiveReport(const HidReport *report);

        private:
            void mapStickValues(JoystickPosition *dst, const Dualshock4StickData *src);
            void handleInputReport0x01(const Dualshock4ReportData *data);
            void handleInputReport0x11(const Dualshock4ReportData *data); 
    };

}
