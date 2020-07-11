#pragma once
#include "fakeswitchcontroller.hpp"

namespace ams::controller {

    struct Dualshock4LedColour {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };

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
    } __attribute__((packed));

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
    } __attribute__((packed));

    struct Dualshock4OutputReport0x11  {
        struct {
            uint8_t data[75];
        };
        uint32_t crc;
    } __attribute__((packed));

    struct Dualshock4InputReport0x01 {
        Dualshock4StickData     left_stick;
        Dualshock4StickData     right_stick;
        Dualshock4ButtonData    buttons;
        uint8_t                 left_trigger;
        uint8_t                 right_trigger;
    } __attribute__((packed));

    /* This report is sent once the GET REPORT FEATURE 0x02 is received */
    struct Dualshock4InputReport0x11 {
        uint8_t _unk[2];
        Dualshock4StickData     left_stick;
        Dualshock4StickData     right_stick;
        Dualshock4ButtonData    buttons;
        uint8_t                 left_trigger;
        uint8_t                 right_trigger;
        uint16_t                timestamp;
        uint8_t                 battery;
        uint16_t                vel_x;
        uint16_t                vel_y;
        uint16_t                vel_z;
        uint16_t                acc_x;
        uint16_t                acc_y;
        uint16_t                acc_z;
        uint32_t                _unk0;

        uint8_t battery_level    : 4;
        uint8_t usb              : 1;
        uint8_t mic              : 1;
        uint8_t phone            : 1;
        uint8_t                  : 0;

        uint16_t _unk2;
        uint8_t tpad_packets;
        uint8_t packet_counter;
    } __attribute__((packed));

    struct Dualshock4ReportData {
        uint8_t id;
        union {
            Dualshock4InputReport0x01 input0x01;
            Dualshock4InputReport0x11 input0x11;
        };
    } __attribute__((packed));

    class Dualshock4Controller : public FakeSwitchController {

        public:
            static constexpr const HardwareID hardwareIds[] = { 
                {0x054c, 0x05c4},  // Official Dualshock4 v1
                {0x054c, 0x09cc}   // Official Dualshock4 v2
            };

            Dualshock4Controller(const bluetooth::Address *address);
            
            Result initialize(void);
            
            void convertReportFormat(const bluetooth::HidReport *inReport, bluetooth::HidReport *outReport);

        private:
            void handleInputReport0x01(const Dualshock4ReportData *src, SwitchReportData *dst);
            void handleInputReport0x11(const Dualshock4ReportData *src, SwitchReportData *dst);
    };

}
