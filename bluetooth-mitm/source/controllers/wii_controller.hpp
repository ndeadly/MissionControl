/*
 * Copyright (c) 2020 ndeadly
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#include "emulated_switch_controller.hpp"

namespace ams::controller {

    enum WiiControllerLEDs {
		WiiControllerLEDs_P1 = 0x10,
		WiiControllerLEDs_P2 = 0x20,
		WiiControllerLEDs_P3 = 0x40,
		WiiControllerLEDs_P4 = 0x80,
	};

	enum WiiExtensionController {
		WiiExtensionController_None,
		WiiExtensionController_Nunchuck,
		WiiExtensionController_Classic,
		WiiExtensionController_ClassicPro,
		WiiExtensionController_WiiUPro,
		WiiExtensionController_TaTaCon,
		WiiExtensionController_Unsupported,
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
		uint8_t x;
		uint8_t y;
		uint8_t z;
	} __attribute__ ((__packed__));

	struct WiiClassicControllerButtonData {
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
	} __attribute__ ((__packed__));

	struct WiiNunchuckExtensionData {
		uint8_t stick_x;
		uint8_t stick_y;
		uint8_t accel_x_92;
		uint8_t accel_y_92;
		uint8_t accel_z_92;

		uint8_t Z : 1;
		uint8_t C : 1;
		uint8_t accel_x_10 : 2;
		uint8_t accel_y_10 : 2;
		uint8_t accel_z_10 : 2;	
	} __attribute__ ((__packed__));

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
    } __attribute__ ((__packed__));

    struct WiiUProExtensionData {
        uint16_t left_stick_x;
        uint16_t right_stick_x;
        uint16_t left_stick_y;
        uint16_t right_stick_y;
        WiiUProButtonData buttons;
    } __attribute__ ((__packed__));

	struct TaTaConExtensionData {
		uint8_t _unk0[5];

		uint8_t 			: 3;
		uint8_t R_rim		: 1;
		uint8_t R_center	: 1;
		uint8_t L_rim		: 1;
		uint8_t L_center	: 1;
		uint8_t 			: 0;
	} __attribute__ ((__packed__));

	struct WiiOutputReport0x10 {
		uint8_t rumble;
	} __attribute__ ((__packed__));

	struct WiiOutputReport0x11 {
		uint8_t leds;
	} __attribute__ ((__packed__));

	struct WiiOutputReport0x12 {
		uint8_t _unk;
        uint8_t report_mode;
	} __attribute__ ((__packed__));

	struct WiiOutputReport0x14 {
		uint8_t : 5;
		uint8_t speaker_enable : 1;
		uint8_t : 0;
	} __attribute__ ((__packed__));

	struct WiiOutputReport0x15 {
		uint8_t _unk;
	} __attribute__ ((__packed__));

	struct WiiOutputReport0x16 {
		uint32_t address;
		uint8_t  size;
		uint8_t  data[16];
	} __attribute__ ((__packed__));

	struct WiiOutputReport0x17 {
		uint32_t address;
		uint16_t size;
	} __attribute__ ((__packed__));

	struct WiiOutputReport0x18 {
		uint8_t size;
		uint8_t speaker_data[20];
	} __attribute__ ((__packed__));

	struct WiiOutputReport0x19 {
		uint8_t : 5;
		uint8_t speaker_mute : 1;
		uint8_t : 0;
	} __attribute__ ((__packed__));

	struct WiiInputReport0x20 {
        WiiButtonData   buttons;
		uint8_t			battery_critical : 1;
		uint8_t			extension_connected : 1;
		uint8_t			speaker_enabled : 1;
		uint8_t			ir_enabled : 1;
		uint8_t			led_state : 4;
		uint8_t			_pad[2];
		uint8_t			battery;
    } __attribute__ ((__packed__));

	struct WiiInputReport0x21 {
		WiiButtonData buttons;
		uint8_t 	  size  : 4;
		uint8_t		  error : 4;
		uint16_t	  address;
		uint8_t		  data[16];
	} __attribute__ ((__packed__));

	struct WiiInputReport0x22 {
		WiiButtonData   buttons;
		uint8_t			report_id;
		uint8_t			error;
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
			WiiOutputReport0x10 output0x10;
			WiiOutputReport0x11 output0x11;
			WiiOutputReport0x12 output0x12;
			WiiOutputReport0x14 output0x14;
			WiiOutputReport0x15 output0x15;
			WiiOutputReport0x16 output0x16;
			WiiOutputReport0x17 output0x17;
			WiiOutputReport0x18 output0x18;
			WiiOutputReport0x19 output0x19;
			WiiInputReport0x20  input0x20;
			WiiInputReport0x21  input0x21;
			WiiInputReport0x22  input0x22;
            WiiInputReport0x30  input0x30;
            WiiInputReport0x31  input0x31;
            WiiInputReport0x32  input0x32;
            WiiInputReport0x33  input0x33;
            WiiInputReport0x34  input0x34;
            WiiInputReport0x35  input0x35;
            WiiInputReport0x36  input0x36;
            WiiInputReport0x37  input0x37;
            WiiInputReport0x3d  input0x3d;
        };
	} __attribute__ ((__packed__));

    class WiiController : public EmulatedSwitchController {

		public:
		    static constexpr const HardwareID hardware_ids[] = { 
                {0x057e, 0x0306},  // Official Wiimote
                {0x057e, 0x0330},  // Official Wii U Pro Controller
            };

			WiiController(const bluetooth::Address *address)    
				: EmulatedSwitchController(address)
				, m_extension(WiiExtensionController_None) { };

			Result Initialize(void);
			void UpdateControllerState(const bluetooth::HidReport *report);

        protected:
			void HandleInputReport0x20(const WiiReportData *src);
			void HandleInputReport0x21(const WiiReportData *src);
			void HandleInputReport0x22(const WiiReportData *src);
            void HandleInputReport0x30(const WiiReportData *src);
            void HandleInputReport0x31(const WiiReportData *src);
            void HandleInputReport0x32(const WiiReportData *src);
            void HandleInputReport0x34(const WiiReportData *src);

            void MapButtonsHorizontalOrientation(const WiiButtonData *buttons);
            void MapButtonsVerticalOrientation(const WiiButtonData *buttons);

			void MapExtensionBytes(const uint8_t ext[]);
			void MapNunchuckExtension(const uint8_t ext[]);
			void MapClassicControllerExtension(const uint8_t ext[]);
			void MapWiiUProControllerExtension(const uint8_t ext[]);
			void MapTaTaConExtension(const uint8_t ext[]);

			Result WriteMemory(uint32_t write_addr, const uint8_t *data, uint8_t size);
			Result ReadMemory(uint32_t read_addr, uint16_t size);

            Result SetReportMode(uint8_t mode);
			Result QueryStatus(void);

			Result SetPlayerLed(uint8_t led_mask);

			WiiExtensionController m_extension;
    };

}
