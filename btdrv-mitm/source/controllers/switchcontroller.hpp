#pragma once
#include "bluetoothcontroller.hpp"

namespace controller {

    enum BatteryLevel {
        BatteryLevel_Empty,
        BatteryLevel_Critical,
        BatteryLevel_Low,
        BatteryLevel_Medium,
        BatteryLevel_Full
    };

    union SwitchStickData {
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
    };

    struct Switch6AxisData {
        uint16_t    accel_x;
        uint16_t    accel_y;
        uint16_t    accel_z;
        uint16_t    gyro_1;
        uint16_t    gyro_2;
        uint16_t    gyro_3;
    };

    union SwitchReportData {
        struct {
            uint8_t             conn_info      : 4;
            uint8_t             battery        : 4;
            uint8_t             timer;
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

        } report0x21;

        struct {
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
        } report0x30;
    };

    class SwitchProController : public BluetoothController {

        public:
            static constexpr const HardwareID hardwareIds[] = { 
                {0x057e, 0x2009}   // Official Switch Pro Controller
            };

            SwitchProController(const BluetoothAddress *address) : BluetoothController(address, ControllerType_SwitchPro) {};

    };

    class JoyconController : public BluetoothController {

        public:
            static constexpr const HardwareID hardwareIds[] = { 
                {0x057e, 0x2006},   // Official Joycon(L) Controller
                {0x057e, 0x2007},   // Official Joycon(R) Controller
            };

            JoyconController(const BluetoothAddress *address) : BluetoothController(address, ControllerType_Joycon) {};

    };

}
