#pragma once
#include "bluetoothcontroller.hpp"

namespace ams::controller {

    enum WiiControllerLEDs {
		WiiControllerLEDs_P1 = 0x10,
		WiiControllerLEDs_P2 = 0x20,
		WiiControllerLEDs_P3 = 0x40,
		WiiControllerLEDs_P4 = 0x80,
	};

    struct WiiButtonData {
		uint8_t dpad_left	: 1;
		uint8_t dpad_right	: 1;
		uint8_t dpad_down	: 1;
		uint8_t dpad_up	    : 1;
		uint8_t plus		: 1;
		uint8_t				: 0;
		
		uint8_t two			: 1;
		uint8_t one 		: 1;
		uint8_t B			: 1;
		uint8_t A			: 1;
		uint8_t minus		: 1;
		uint8_t				: 2;
		uint8_t home 		: 1;
	} __attribute__ ((__packed__));

	struct WiiAccelerometerData {
		uint8_t xyz[3];
	} __attribute__ ((__packed__));

	    struct WiiInputReport0x30 {
        WiiButtonData   buttons;
    } __attribute__ ((__packed__));

    struct WiiInputReport0x31 {
        WiiButtonData           buttons;
        WiiAccelerometerData    accel;
    } __attribute__ ((__packed__));

    struct WiiInputReport0x32 {
        WiiButtonData   buttons;
        uint8_t         extension[8];
    } __attribute__ ((__packed__));

    struct WiiInputReport0x33 {
        WiiButtonData           buttons;
        WiiAccelerometerData    accel;
        uint8_t                 ir[12];
    } __attribute__ ((__packed__));

    struct WiiInputReport0x34 {
        WiiButtonData           buttons;
        uint8_t                 extension[19];
    } __attribute__ ((__packed__));

    struct WiiInputReport0x35 {
        WiiButtonData           buttons;
        WiiAccelerometerData    accel;
        uint8_t                 extension[16];
    } __attribute__ ((__packed__));

    struct WiiInputReport0x36 {
        WiiButtonData   buttons;
        uint8_t         ir[10];
        uint8_t         extension[9];
    } __attribute__ ((__packed__));

    struct WiiInputReport0x37 {
        WiiButtonData           buttons;
        WiiAccelerometerData    accel;
        uint8_t                 ir[10];
        uint8_t                 extension[6];
    } __attribute__ ((__packed__));

    struct WiiInputReport0x3d {
        uint8_t extension[21];
    } __attribute__ ((__packed__));

    struct WiiReportData {
        uint8_t id;
        union {
            WiiInputReport0x30 input0x30;
            WiiInputReport0x31 input0x31;
            WiiInputReport0x32 input0x32;
            WiiInputReport0x33 input0x33;
            WiiInputReport0x34 input0x34;
            WiiInputReport0x35 input0x35;
            WiiInputReport0x36 input0x36;
            WiiInputReport0x37 input0x37;
            WiiInputReport0x3d input0x3d;
        };
	} __attribute__ ((__packed__));

    class WiiController : public BluetoothController {

        public:
            Result initialize(void);

        protected:
            WiiController(ControllerType type, const bluetooth::Address *address) : BluetoothController(type, address) {};

            Result writeMemory(const bluetooth::Address *address, uint32_t writeaddr, const uint8_t *data, uint8_t length);
            Result setReportMode(const bluetooth::Address *address, uint8_t mode);
            Result setPlayerLeds(const bluetooth::Address *address, uint8_t mask);

    };


}
