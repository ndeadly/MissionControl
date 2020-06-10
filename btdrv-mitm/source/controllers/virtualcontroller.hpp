#pragma once

#include <switch.h>

namespace controller {

    struct SwitchProGamepadState {

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

        JoystickPosition left_stick;
        JoystickPosition right_stick;
    };

	class VirtualController {

		public:
			virtual ~VirtualController() {};

			virtual Result connect(void) = 0;
			virtual Result disconnect(void) = 0;
			virtual Result setState(const SwitchProGamepadState* state) = 0;
			
	};

}