#pragma once
#include "bluetoothcontroller.hpp"
#include "switchcontroller.hpp"

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

    struct XboxOneReport0x01 {
        XboxOneStickData    left_stick;
        XboxOneStickData    right_stick;
        uint16_t            left_trigger;
        uint16_t            right_trigger;
        XboxOneButtonData   buttons;
    } __attribute__ ((__packed__));

    struct XboxOneReport0x02{
        uint8_t guide   : 1;
        uint8_t         : 0; 
    } __attribute__ ((__packed__)); 

    struct XboxOneReport0x04{
        uint8_t capacity : 2;
        uint8_t mode     : 2;
        uint8_t charging : 1;
        uint8_t          : 2;
        uint8_t online   : 1;
    } __attribute__ ((__packed__));
 
    struct XboxOneReportData {
        uint8_t id;
        union {
            XboxOneReport0x01 report0x01;
            XboxOneReport0x02 report0x02;
            XboxOneReport0x04 report0x04;
        };
    } __attribute__ ((__packed__));

    class XboxOneController : public BluetoothController {

        public:
            static constexpr const HardwareID hardwareIds[] = { 
                {0x045e, 0x02e0}, // Official Xbox One S Controller (old FW?)
                {0x045e, 0x02fd}  // Official Xbox One S Controller
            };

            XboxOneController(const bluetooth::Address *address);

            void convertReportFormat(const bluetooth::HidReport *inReport, bluetooth::HidReport *outReport);

        private:
            void handleInputReport0x01(const XboxOneReportData *src, SwitchReportData *dst);
            void handleInputReport0x02(const XboxOneReportData *src, SwitchReportData *dst);
            void handleInputReport0x04(const XboxOneReportData *src, SwitchReportData *dst);

    };

}
