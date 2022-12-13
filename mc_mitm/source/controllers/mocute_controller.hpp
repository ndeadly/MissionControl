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

    enum MocuteDPadDirection {
        MocuteDPad_N,
        MocuteDPad_NE,
        MocuteDPad_E,
        MocuteDPad_SE,
        MocuteDPad_S,
        MocuteDPad_SW,
        MocuteDPad_W,
        MocuteDPad_NW,
        MocuteDPad_Released = 0x0f
    };

    enum MocuteDPadDirection2 {
        MocuteDPad2_Released = 0x00,
        MocuteDPad2_N,
        MocuteDPad2_NE,
        MocuteDPad2_E,
        MocuteDPad2_SE,
        MocuteDPad2_S,
        MocuteDPad2_SW,
        MocuteDPad2_W,
        MocuteDPad2_NW
    };

    struct MocuteStickData {
        u8 x;
        u8 y;
    } __attribute__ ((__packed__));

    struct MocuteButtonData {
        u8 dpad   : 4;
        u8 A      : 1;
        u8 B      : 1;
        u8 X      : 1;
        u8 Y      : 1;

        u8 L1     : 1;
        u8 R1     : 1;
        u8 select : 1;
        u8 start  : 1;
        u8 L3     : 1;
        u8 R3     : 1;
        u8 L2     : 1;
        u8 R2     : 1;
    } __attribute__ ((__packed__));

    struct MocuteInputReport0x01 {
        MocuteStickData left_stick;
        MocuteStickData right_stick;
        MocuteButtonData buttons;
        u8 left_trigger;
        u8 right_trigger;
    } __attribute__ ((__packed__));

    struct MocuteReportData {
        u8 id;
        union {
            MocuteInputReport0x01 input0x01;
        };
    } PACKED;

    class MocuteController final : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = {
                {0xffff, 0x0000},   // Mocute 050 Controller
                {0x04e8, 0x046e}    // Mocute 050 Controller
            };

            MocuteController(const bluetooth::Address *address, HardwareID id)
            : EmulatedSwitchController(address, id) { }

            void ProcessInputData(const bluetooth::HidReport *report) override;

        private:
            void MapInputReport(const MocuteReportData *src);

    };

}
