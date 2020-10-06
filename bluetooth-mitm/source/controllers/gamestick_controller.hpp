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

    enum GamestickDPadDirection {
        GamestickDPad_N,
        GamestickDPad_NE,
        GamestickDPad_E,
        GamestickDPad_SE,
        GamestickDPad_S,
        GamestickDPad_SW,
        GamestickDPad_W,
        GamestickDPad_NW,
        GamestickDPad_Released = 0x0f
    };

    struct GamestickStickData {
        uint8_t x;
        uint8_t y;
    } __attribute__((packed));

    struct GamestickInputReport0x01 {
        uint8_t _unk0;

        struct {
            uint8_t         : 4;
            uint8_t home    : 1;
            uint8_t back    : 1;
            uint8_t         : 0;
        } buttons;

        uint8_t _unk1[6];
    } __attribute__((packed));

    struct GamestickInputReport0x03 {
        uint8_t dpad;
        GamestickStickData left_stick;
        GamestickStickData right_stick;
        uint8_t _unk0[2];
        
        struct {
            uint8_t A   : 1;
            uint8_t B   : 1;
            uint8_t     : 1;
            uint8_t X   : 1;
            uint8_t Y   : 1;
            uint8_t     : 1;
            uint8_t L   : 1;
            uint8_t R   : 1;

            uint8_t              : 3;
            uint8_t start        : 1;
            uint8_t              : 1;
            uint8_t lstick_press : 1;
            uint8_t rstick_press : 1; 
            uint8_t              : 1;
        } buttons;

    } __attribute__((packed));

    struct GamestickReportData {
        uint8_t id;
        union {
            GamestickInputReport0x01 input0x01;
            GamestickInputReport0x03 input0x03;
        };
    } __attribute__((packed));

    class GamestickController : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = { 
                {0x0f0d, 0x1011}
            };  

            GamestickController(const bluetooth::Address *address) 
                : EmulatedSwitchController(address) { };

            void UpdateControllerState(const bluetooth::HidReport *report);

        private:
            void HandleInputReport0x01(const GamestickReportData *src);
            void HandleInputReport0x03(const GamestickReportData *src);

           
    };

}
