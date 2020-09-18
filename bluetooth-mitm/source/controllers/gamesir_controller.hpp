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

    enum GamesirDpadDirection {
        GamesirDpad_Released,
        GamesirDpad_N,
        GamesirDpad_NE,
        GamesirDpad_E,
        GamesirDpad_SE,
        GamesirDpad_S,
        GamesirDpad_SW,
        GamesirDpad_W,
        GamesirDpad_NW,
    };

    struct GamesirStickData {
        uint8_t x;
        uint8_t y;
    } __attribute__((packed));

    struct GamesirButtonData {
        uint8_t A       : 1;
        uint8_t B       : 1;
        uint8_t         : 1;
        uint8_t X       : 1;
        uint8_t Y       : 1;
        uint8_t         : 1;
        uint8_t LB      : 1;
        uint8_t RB      : 1;

        uint8_t LT      : 1;
        uint8_t RT      : 1;
        uint8_t select  : 1;
        uint8_t start   : 1;
        uint8_t         : 1;
        uint8_t L3      : 1;
        uint8_t R3      : 1;
        uint8_t         : 0;

        uint8_t dpad;
    } __attribute__((packed));

    struct GamesirReport0x12 {
        uint8_t         : 3;
        uint8_t home    : 1;
        uint8_t         : 0;

        uint8_t _unk[2];
    } __attribute__((packed));

    struct GamesirReport0xc4 {
        GamesirStickData    left_stick;
        GamesirStickData    right_stick;
        uint8_t             left_trigger;
        uint8_t             right_trigger;
        GamesirButtonData   buttons;
        uint8_t             _unk;
    } __attribute__((packed));

    struct GamesirReportData {
        uint8_t id;
        union {
            GamesirReport0x12  input0x12;
            GamesirReport0xc4  input0xc4;
        };
    } __attribute__((packed));

    class GamesirController : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = { 
                {0x05ac, 0x022d},   // Gamesir-G3s (Lol, this is actually the ID of an Apple wireless keyboard)
                {0xffff, 0x046f}    // Gamesir-G4s
            };  

            GamesirController(const bluetooth::Address *address) 
                : EmulatedSwitchController(address) { };

            void ConvertReportFormat(const bluetooth::HidReport *in_report, bluetooth::HidReport *out_report);

        private:
            void HandleInputReport0x12(const GamesirReportData *src, SwitchReportData *dst);
            void HandleInputReport0xc4(const GamesirReportData *src, SwitchReportData *dst);

    };

}
