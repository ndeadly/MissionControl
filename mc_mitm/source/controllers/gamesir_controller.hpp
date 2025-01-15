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

    enum GamesirDpadDirection2 {
        GamesirDpad2_N,
        GamesirDpad2_NE,
        GamesirDpad2_E,
        GamesirDpad2_SE,
        GamesirDpad2_S,
        GamesirDpad2_SW,
        GamesirDpad2_W,
        GamesirDpad2_NW,
        GamesirDpad2_Released = 0x0f,
    };

    struct GamesirButtonData {
        u8 A      : 1;
        u8 B      : 1;
        u8        : 1;
        u8 X      : 1;
        u8 Y      : 1;
        u8        : 1;
        u8 LB     : 1;
        u8 RB     : 1;

        u8 LT     : 1;
        u8 RT     : 1;
        u8 select : 1;
        u8 start  : 1;
        u8 home   : 1;  // Only present in report 0x03
        u8 L3     : 1;
        u8 R3     : 1;
        u8        : 0;

        u8 dpad;
    } PACKED;

    struct GamesirReport0x03 {
        GamesirButtonData buttons;
        AnalogStick<u8> left_stick;
        AnalogStick<u8> right_stick;
        u8 left_trigger;
        u8 right_trigger;
        u8 _unk[2];
    } PACKED;

    struct GamesirReport0x12 {
        u8      : 3;
        u8 home : 1;
        u8      : 0;

        u8 _unk[2];
    } PACKED;

    struct GamesirReport0xc4 {
        AnalogStick<u8> left_stick;
        AnalogStick<u8> right_stick;
        u8 left_trigger;
        u8 right_trigger;
        GamesirButtonData buttons;
        u8 _unk;
    } PACKED;

    struct GamesirReportData {
        u8 id;
        union {
            GamesirReport0x03 input0x03;
            GamesirReport0x12 input0x12;
            GamesirReport0xc4 input0xc4;
        };
    } PACKED;

    class GamesirController final : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = {
                {0xffff, 0x046e},   // Gamesir G3s
                {0x05ac, 0x022d},   // Gamesir G3s (Alternate mode. Lol, this is actually the ID of an Apple wireless keyboard)
                {0xffff, 0x046f},   // Gamesir G4s
                {0xffff, 0x0450},   // Gamesir T1s
                {0x05ac, 0x056b}    // Gamesir T2a
            };

            GamesirController(const bluetooth::Address *address, HardwareID id)
            : EmulatedSwitchController(address, id) { }

            void ProcessInputData(const bluetooth::HidReport *report) override;

        private:
            void MapInputReport0x03(const GamesirReportData *src);
            void MapInputReport0x12(const GamesirReportData *src);
            void MapInputReport0xc4(const GamesirReportData *src);

    };

}
