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

    struct GamestickInputReport0x01 {
        u8 _unk0;

        struct {
            u8      : 4;
            u8 home : 1;
            u8 back : 1;
            u8      : 0;
        } buttons;

        u8 _unk1[6];
    } PACKED;

    struct GamestickInputReport0x03 {
        u8 dpad;
        AnalogStick<u8> left_stick;
        AnalogStick<u8> right_stick;
        u8 _unk0[2];
        
        struct {
            u8 A            : 1;
            u8 B            : 1;
            u8              : 1;
            u8 X            : 1;
            u8 Y            : 1;
            u8              : 1;
            u8 L            : 1;
            u8 R            : 1;

            u8              : 3;
            u8 start        : 1;
            u8              : 1;
            u8 lstick_press : 1;
            u8 rstick_press : 1; 
            u8              : 1;
        } buttons;

    } PACKED;

    struct GamestickReportData {
        u8 id;
        union {
            GamestickInputReport0x01 input0x01;
            GamestickInputReport0x03 input0x03;
        };
    } PACKED;

    class GamestickController final : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = {
                {0x0f0d, 0x1011}
            };

            GamestickController(const bluetooth::Address *address, HardwareID id)
            : EmulatedSwitchController(address, id) { }

            void ProcessInputData(const bluetooth::HidReport *report) override;

        private:
            void MapInputReport0x01(const GamestickReportData *src);
            void MapInputReport0x03(const GamestickReportData *src);

    };

}
