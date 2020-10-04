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

    struct OuyaStickData {
        uint16_t x;
        uint16_t y;
    } __attribute__ ((__packed__));

    struct OuyaButtonData {
        uint8_t O             : 1;
        uint8_t U             : 1;
        uint8_t Y             : 1;
        uint8_t A             : 1;
        uint8_t LB            : 1;
        uint8_t RB            : 1;
        uint8_t LS            : 1;
        uint8_t RS            : 1;

        uint8_t dpad_up       : 1;
        uint8_t dpad_down     : 1;
        uint8_t dpad_left     : 1;
        uint8_t dpad_right    : 1;
        uint8_t LT            : 1;
        uint8_t RT            : 1;
        uint8_t center_press  : 1;
        uint8_t center_hold   : 1;
    } __attribute__ ((__packed__));;

    struct OuyaInputReport0x03 {
        uint8_t battery;
        uint8_t _unk[6];
    } __attribute__((packed));

    struct OuyaInputReport0x07 {
        OuyaStickData   left_stick;
        OuyaStickData   right_stick;
        uint16_t        left_trigger;
        uint16_t        right_trigger;
        OuyaButtonData  buttons;
    } __attribute__((packed));

    struct OuyaReportData {
        uint8_t id;
        union {
            OuyaInputReport0x03  input0x03;
            OuyaInputReport0x07  input0x07;
        };
    } __attribute__((packed));

    class OuyaController : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = { 
                {0x2836, 0x0001}
            };  

            OuyaController(const bluetooth::Address *address) 
                : EmulatedSwitchController(address) { };

            void UpdateControllerState(const bluetooth::HidReport *report);

        private:
            void HandleInputReport0x03(const OuyaReportData *src);
            void HandleInputReport0x07(const OuyaReportData *src);

    };

}
