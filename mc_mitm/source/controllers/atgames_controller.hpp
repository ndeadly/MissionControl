/*
 * Copyright (c) 2020-2022 ndeadly
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

    enum AtGamesDPadDirection {
        AtGamesDPad_N,
        AtGamesDPad_NE,
        AtGamesDPad_E,
        AtGamesDPad_SE,
        AtGamesDPad_S,
        AtGamesDPad_SW,
        AtGamesDPad_W,
        AtGamesDPad_NW,
        AtGamesDPad_Released = 0x08
    };

    struct AtGamesStickData {
        uint8_t x;
        uint8_t y;
    } __attribute__((packed));

    struct AtGamesInputReport0x01 {
        uint8_t rewind          : 1;
        uint8_t nudge_front     : 1;
        uint8_t                 : 2;
        uint8_t nudge_left      : 1;
        uint8_t flipper_right   : 1;
        uint8_t                 : 1;
        uint8_t play            : 1;

        uint8_t                 : 1;
        uint8_t home_twirl      : 1;
        uint8_t flipper_left    : 1;
        uint8_t nudge_right     : 1;
        uint8_t                 : 0;

        uint8_t unk1[2];
        uint8_t dpad;
        AtGamesStickData left_stick;
        AtGamesStickData right_stick; // Only right stick y-axis is used for plunger
        uint8_t unk2;

    } __attribute__((packed));

    struct AtGamesReportData {
        uint8_t id;
        union {
            AtGamesInputReport0x01 input0x01;
        };
    } __attribute__((packed));

    class AtGamesController : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = {
                {0x1d6b, 0x0246},   // AtGames Legends Pinball
            };

            AtGamesController(const bluetooth::Address *address, HardwareID id)
            : EmulatedSwitchController(address, id) { }

            void ProcessInputData(const bluetooth::HidReport *report) override;

        private:
            void MapInputReport0x01(const AtGamesReportData *src);

    };

}
