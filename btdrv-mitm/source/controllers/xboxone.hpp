#pragma once
#include "fakeswitchcontroller.hpp"

namespace ams::controller {

    enum XboxOneDPadDirection {
        XboxOneDPad_Released,
        XboxOneDPad_N,
        XboxOneDPad_NE,
        XboxOneDPad_E,
        XboxOneDPad_SE,
        XboxOneDPad_S,
        XboxOneDPad_SW,
        XboxOneDPad_W,
        XboxOneDPad_NW
    };

    struct XboxOneStickData {
        uint16_t x;
        uint16_t y;
    } __attribute__ ((__packed__));

    struct XboxOneButtonData {
        uint8_t      dpad;

        uint8_t A            : 1;
        uint8_t B            : 1;
        uint8_t X            : 1;
        uint8_t Y            : 1;
        uint8_t LB           : 1;
        uint8_t RB           : 1;
        uint8_t view         : 1;
        uint8_t menu         : 1;

        uint8_t lstick_press : 1;
        uint8_t rstick_press : 1;
        uint8_t              : 0;
    } __attribute__ ((__packed__));

    struct XboxOneInputReport0x01 {
        XboxOneStickData    left_stick;
        XboxOneStickData    right_stick;
        uint16_t            left_trigger;
        uint16_t            right_trigger;
        XboxOneButtonData   buttons;
    } __attribute__ ((__packed__));

    struct XboxOneInputReport0x02{
        uint8_t guide   : 1;
        uint8_t         : 0; 
    } __attribute__ ((__packed__)); 

    struct XboxOneInputReport0x04{
        uint8_t capacity : 2;
        uint8_t mode     : 2;
        uint8_t charging : 1;
        uint8_t          : 2;
        uint8_t online   : 1;
    } __attribute__ ((__packed__));
 
    struct XboxOneReportData {
        uint8_t id;
        union {
            XboxOneInputReport0x01 input0x01;
            XboxOneInputReport0x02 input0x02;
            XboxOneInputReport0x04 input0x04;
        };
    } __attribute__ ((__packed__));

    class XboxOneController : public FakeSwitchController {

        public:
            static constexpr const HardwareID hardwareIds[] = { 
                {0x045e, 0x02e0}, // Official Xbox One S Controller (old FW?)
                {0x045e, 0x02fd}  // Official Xbox One S Controller
            };

            XboxOneController(const bluetooth::Address *address) 
                : FakeSwitchController(ControllerType_XboxOne, address) { };

            Result initialize(void);
            void convertReportFormat(const bluetooth::HidReport *inReport, bluetooth::HidReport *outReport);

        private:
            void handleInputReport0x01(const XboxOneReportData *src, SwitchReportData *dst);
            void handleInputReport0x02(const XboxOneReportData *src, SwitchReportData *dst);
            void handleInputReport0x04(const XboxOneReportData *src, SwitchReportData *dst);

    };

}
