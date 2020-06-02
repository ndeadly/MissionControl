#pragma once
#include "bluetoothcontroller.hpp"

namespace controller {

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
    };

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
    };
 
    union XboxOneReportData {
        struct {
            XboxOneStickData    left_stick;
            XboxOneStickData    right_stick;
            uint16_t            left_trigger;
            uint16_t            right_trigger;
            XboxOneButtonData   buttons;
        } report0x01;

        struct {
            uint8_t guide    : 1;
        } report0x02;
    };

    class XboxOneController : public BluetoothController {

        public:
            static constexpr const HardwareID hardwareIds[] = { 
                {0x045e, 0x02e0}, // Official Xbox One S Controller (old FW?)
                {0x045e, 0x02fd}  // Official Xbox One S Controller
            };

            XboxOneController(const BluetoothAddress *address) : BluetoothController(address, ControllerType_XboxOne) {};

            void convertReportFormat(HidReport *report);

        private:
            void mapStickValues(JoystickPosition *dst, const XboxOneStickData *src); 
            void handleInputReport0x01(const XboxOneReportData *data);
            void handleInputReport0x02(const XboxOneReportData *data);
            
    };

}
