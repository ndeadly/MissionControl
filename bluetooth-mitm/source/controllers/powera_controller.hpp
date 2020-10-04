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

    enum PowerADPadDirection {
        PowerADPad_N,
        PowerADPad_NE,
        PowerADPad_E,
        PowerADPad_SE,
        PowerADPad_S,
        PowerADPad_SW,
        PowerADPad_W,
        PowerADPad_NW,
        PowerADPad_Released = 0x0f
    };

    struct PowerAStickData {
        uint8_t x;
        uint8_t y;
    } __attribute__((packed));

    struct PowerAButtonData {
        uint8_t dpad    : 4;
        uint8_t A       : 1;
        uint8_t B       : 1;
        uint8_t X       : 1;
        uint8_t Y       : 1;

        uint8_t L1      : 1;
        uint8_t R1      : 1;
        uint8_t select  : 1;
        uint8_t start   : 1;
        uint8_t L3      : 1;
        uint8_t R3      : 1;
        uint8_t         : 0;
    } __attribute__((packed));

    struct PowerAInputReport0x03 {
        PowerAStickData left_stick;
        PowerAStickData right_stick;
        PowerAButtonData buttons;
        uint8_t L2;
        uint8_t R2;
        uint8_t battery;
        uint8_t _unk;
    } __attribute__((packed));

    struct PowerAReportData{
        uint8_t id;
        union {
            PowerAInputReport0x03 input0x03;
        };
    } __attribute__((packed));

    class PowerAController : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = { 
                {0x20d6, 0x89e5},   // Moga Hero Controller
                {0x20d6, 0x6271}    // Moga Pro 2 Controller
            };  

            PowerAController(const bluetooth::Address *address) 
                : EmulatedSwitchController(address) { };

            void UpdateControllerState(const bluetooth::HidReport *report);

        private:
            void HandleInputReport0x03(const PowerAReportData *src);

    };

}
