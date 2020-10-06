/*
 * Copyright (C) 2020 ndeadly
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#include "emulated_switch_controller.hpp"

namespace ams::controller {

    enum XiaomiDPadDirection {
        XiaomiDPad_N,
        XiaomiDPad_NE,
        XiaomiDPad_E,
        XiaomiDPad_SE,
        XiaomiDPad_S,
        XiaomiDPad_SW,
        XiaomiDPad_W,
        XiaomiDPad_NW,
        XiaomiDPad_Released = 0x0f
    };

    struct XiaomiStickData {
        uint8_t x;
        uint8_t y;
    } __attribute__((packed));

    struct XiaomiButtonData {
        uint8_t A            : 1;
        uint8_t B            : 1;
        uint8_t              : 1;
        uint8_t X            : 1;
        uint8_t Y            : 1;
        uint8_t              : 1;
        uint8_t L1           : 1;
        uint8_t R1           : 1;

        uint8_t L2           : 1;
        uint8_t R2           : 1;
        uint8_t back         : 1;
        uint8_t menu         : 1;
        uint8_t              : 1;
        uint8_t lstick_press : 1;
        uint8_t rstick_press : 1;
        uint8_t              : 0;

        uint8_t _unk;
        
        uint8_t dpad;
    } __attribute__((packed));

    struct XiaomiInputReport0x04 {
        XiaomiButtonData buttons;
        XiaomiStickData  left_stick;
        XiaomiStickData  right_stick;
        uint8_t  _unk0[2];
        uint8_t  left_trigger;
        uint8_t  right_trigger;
        uint16_t accel_x;
        uint16_t accel_y;
        uint16_t accel_z;
        uint8_t  battery;
        uint8_t  home   : 1;
        uint8_t         : 0;
    } __attribute__((packed));

    struct XiaomiReportData{
        uint8_t id;
        union {
            XiaomiInputReport0x04 input0x04;
        };
    } __attribute__((packed));

    class XiaomiController : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = { 
                {0x2717, 0x3144}    // Xiaomi Mi Controller
            };  

            XiaomiController(const bluetooth::Address *address) 
                : EmulatedSwitchController(address) { };

            Result Initialize(void);

            void UpdateControllerState(const bluetooth::HidReport *report);

        private:
            void HandleInputReport0x04(const XiaomiReportData *src);

    };

}
