/*
 * Copyright (c) 2020-2023 ndeadly
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

    enum BetopDPadDirection {
        BetopDPad_N,
        BetopDPad_NE,
        BetopDPad_E,
        BetopDPad_SE,
        BetopDPad_S,
        BetopDPad_SW,
        BetopDPad_W,
        BetopDPad_NW,
        BetopDPad_Released = 0x0f
    };

    struct BetopStickData {
        u8 x;
        u8 y;
    } PACKED;

    struct BetopButtonData {
        u8 dpad;

        u8 A      : 1;
        u8 B      : 1;
        u8        : 1;
        u8 X      : 1;
        u8 Y      : 1;
        u8        : 1;
        u8 L1     : 1;
        u8 R1     : 1;

        u8 L2     : 1;
        u8 R2     : 1;
        u8 select : 1;
        u8 start  : 1;
        u8 home   : 1;
        u8 L3     : 1;
        u8 R3     : 1;
        u8        : 0;
    } PACKED;

    struct BetopInputReport0x03 {
        u8 unk0;
        BetopButtonData buttons;
        BetopStickData left_stick;
        BetopStickData right_stick;
        u8 left_trigger;
        u8 right_trigger;
        u8 unk1;
    } PACKED;

    struct BetopReportData {
        u8 id;
        union {
            BetopInputReport0x03 input0x03;
        };
    } PACKED;

    class BetopController final : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = {
                {0x20bc, 0x5501}    // Betop 2585N2
            };

            BetopController(const bluetooth::Address *address, HardwareID id)
            : EmulatedSwitchController(address, id) { }

            void ProcessInputData(const bluetooth::HidReport *report) override;

        private:
            void MapInputReport0x03(const BetopReportData *src);

    };

}
