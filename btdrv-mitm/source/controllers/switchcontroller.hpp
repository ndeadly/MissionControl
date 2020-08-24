#pragma once
#include "../bluetooth/bluetooth_types.hpp"

#define UINT12_MAX 0xfff
#define STICK_ZERO 0x800
#define BATTERY_MAX 8

namespace ams::controller {

    enum ControllerType {
        ControllerType_Unknown,
        ControllerType_Joycon,
        ControllerType_SwitchPro,
        ControllerType_Wiimote,
        ControllerType_WiiUPro,
        ControllerType_Dualshock4,
        ControllerType_XboxOne,
    };

    struct HardwareID {
        uint16_t vid;
        uint16_t pid;
    };
        
    struct SwitchStickData {
        uint8_t xy[3];
    } __attribute__ ((__packed__));

    struct SwitchButtonData {
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
    } __attribute__ ((__packed__));

    struct Switch6AxisData {
        uint16_t    accel_x;
        uint16_t    accel_y;
        uint16_t    accel_z;
        uint16_t    gyro_1;
        uint16_t    gyro_2;
        uint16_t    gyro_3;
    } __attribute__ ((__packed__));

    struct SwitchOutputReport0x01;
    struct SwitchOutputReport0x03;
    struct SwitchOutputReport0x10;
    struct SwitchOutputReport0x11;
    struct SwitchOutputReport0x12;

    struct SwitchInputReport0x21 {
        uint8_t             timer;
        uint8_t             conn_info      : 4;
        uint8_t             battery        : 4;
        SwitchButtonData    buttons;
        SwitchStickData     left_stick;
        SwitchStickData     right_stick;
        uint8_t             vibrator;
        
        struct {
            uint8_t         ack;
            uint8_t         id;
            uint8_t         reply;
            uint8_t         data[0x22];
        } subcmd;
    } __attribute__ ((__packed__));

    struct SwitchInputReport0x23;

    struct SwitchInputReport0x30 {
        uint8_t             timer;
        uint8_t             conn_info      : 4;
        uint8_t             battery        : 4;
        SwitchButtonData    buttons;
        SwitchStickData     left_stick;
        SwitchStickData     right_stick;
        uint8_t             vibrator;

        Switch6AxisData     imu_0ms;
        Switch6AxisData     imu_5ms;
        Switch6AxisData     imu_10ms;
    } __attribute__ ((__packed__));

    struct SwitchInputReport0x31;
    struct SwitchInputReport0x32;
    struct SwitchInputReport0x33;
    struct SwitchInputReport0x3f;

    struct SwitchReportData {
        uint8_t id;
        union {
            //SwitchOutputReport0x01 output0x01;
            //SwitchOutputReport0x03 output0x03;
            //SwitchOutputReport0x10 output0x10;
            //SwitchOutputReport0x11 output0x11;
            //SwitchOutputReport0x12 output0x12;
            SwitchInputReport0x21  input0x21;
            SwitchInputReport0x30  input0x30;
            //SwitchInputReport0x31  input0x31;
            //SwitchInputReport0x32  input0x32;
            //SwitchInputReport0x33  input0x33;
            //SwitchInputReport0x3f  input0x3f;

        };
    } __attribute__ ((__packed__));

    class SwitchController {

        public: 
            const bluetooth::Address& address(void) const { return m_address; };
            ControllerType type(void) { return m_type; };

            virtual Result initialize(void) { return ams::ResultSuccess(); };
            
            virtual Result handleIncomingReport(const bluetooth::HidReport *report);
            virtual const bluetooth::HidReport * handleOutgoingReport(const bluetooth::HidReport *report);

        protected:
            SwitchController(ControllerType type, const bluetooth::Address *address)
                : m_type(type)
                , m_address(*address) { };

            ControllerType m_type;
            bluetooth::Address m_address;
    };

}
