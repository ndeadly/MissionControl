/*
 * Copyright (c) 2020-2025 ndeadly
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

    struct AtGamesInputReport0x01 {
        u8 rewind        : 1;
        u8 nudge_front   : 1;
        u8 b_button      : 1;
        u8 y_button      : 1;
        u8 nudge_left    : 1;
        u8 flipper_right : 1;
        u8 x_button      : 1;
        u8 play          : 1;

        u8 a_button      : 1;
        u8 home_twirl    : 1;
        u8 flipper_left  : 1;
        u8 nudge_right   : 1;
        u8 z_button      : 1;
        u8 c_button      : 1;
        u8               : 0;

        u8 unk1[2];
        u8 dpad;
        AnalogStick<u8> left_stick;
        AnalogStick<u8> right_stick; // Only right stick y-axis is used for plunger
        u8 unk2;

    } PACKED;

    struct AtGamesReportData {
        u8 id;
        union {
            AtGamesInputReport0x01 input0x01;
        };
    } PACKED;

    class AtGamesController final : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = {
                {0x1d6b, 0x0246},   // AtGames Legends Pinball
            };

            AtGamesController(const bluetooth::Address *address, HardwareID id)
            : EmulatedSwitchController(address, id), m_arcadepanel(false) { }

            void ProcessInputData(const bluetooth::HidReport *report) override;

        private:
            void MapInputReport0x01(const AtGamesReportData *src);
        
            bool m_arcadepanel;

    };

}
