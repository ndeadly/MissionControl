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

    enum RazerDPadDirection {
        RazerDPad_N,
        RazerDPad_NE,
        RazerDPad_E,
        RazerDPad_SE,
        RazerDPad_S,
        RazerDPad_SW,
        RazerDPad_W,
        RazerDPad_NW,
        RazerDPad_Released
    };

    struct RazerStickData {
        uint8_t x;
        uint8_t y;
    } __attribute__((packed));

    struct RazerButtonData {
        uint8_t dpad    : 4;
        uint8_t A       : 1;
        uint8_t B       : 1;
        uint8_t X       : 1;
        uint8_t Y       : 1;

        uint8_t L1           : 1;
        uint8_t R1           : 1;
        uint8_t back         : 1;
        uint8_t start        : 1;
        uint8_t L3           : 1;
        uint8_t R3           : 1;
        uint8_t              : 1;
        uint8_t home         : 1;

        uint8_t select       : 1;
        uint8_t              : 0;
    } __attribute__((packed));

    struct RazerInputReport0x01 {
        RazerStickData left_stick;
        RazerStickData right_stick;
        RazerButtonData buttons;
        uint8_t left_trigger;
        uint8_t right_trigger;
    } __attribute__((packed));

    struct RazerReportData{
        uint8_t id;
        union {
            RazerInputReport0x01 input0x01;
        };
    } __attribute__((packed));


    class RazerController : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = { 
                {0x1532, 0x0900}    // Razer Serval
            };  

            RazerController(const bluetooth::Address *address) 
                : EmulatedSwitchController(address) { };

            void UpdateControllerState(const bluetooth::HidReport *report);

        private:
            void HandleInputReport0x01(const RazerReportData *src);

    };

}
