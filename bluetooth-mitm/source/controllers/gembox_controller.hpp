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

    enum GemboxDPadDirection {
        GemboxDPad_N,
        GemboxDPad_NE,
        GemboxDPad_E,
        GemboxDPad_SE,
        GemboxDPad_S,
        GemboxDPad_SW,
        GemboxDPad_W,
        GemboxDPad_NW,
        GemboxDPad_Released = 0x0f
    };

    struct GemboxStickData {
        uint8_t x;
        uint8_t y;
    } __attribute__ ((__packed__));

    struct GemboxButtonData {
        uint8_t A       : 1;
        uint8_t B       : 1;
        uint8_t         : 1;
        uint8_t X       : 1;
        uint8_t Y       : 1;
        uint8_t         : 1;
        uint8_t LB      : 1;
        uint8_t RB      : 1;

        uint8_t         : 3;
        uint8_t start   : 1;
        uint8_t         : 1;
        uint8_t L3      : 1;
        uint8_t R3      : 1;
        uint8_t         : 0;
    } __attribute__ ((__packed__));

    struct GemboxInputReport0x02 {
        union {
            struct {
                uint8_t         : 6;
                uint8_t back    : 1;
                uint8_t         : 0;
            };

            uint8_t buttons;
        };
    } __attribute__((packed));

    struct GemboxInputReport0x07 {
        uint8_t             dpad;
        GemboxStickData     left_stick;
        GemboxStickData     right_stick;
        uint8_t             left_trigger;
        uint8_t             right_trigger;
        GemboxButtonData    buttons;
    } __attribute__((packed));

    struct GemboxReportData {
        uint8_t id;
        union {
            GemboxInputReport0x02  input0x02;
            GemboxInputReport0x07  input0x07;
        };
    } __attribute__((packed));

    class GemboxController : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = { 
                {0x1d79, 0x0009}
            };  

            GemboxController(const bluetooth::Address *address) 
                : EmulatedSwitchController(address) { };

            void UpdateControllerState(const bluetooth::HidReport *report);

        private:
            void HandleInputReport0x02(const GemboxReportData *src);
            void HandleInputReport0x07(const GemboxReportData *src);

    };

}
