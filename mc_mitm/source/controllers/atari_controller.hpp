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

    enum AtariDPadDirection {
        AtariDPad_Released,
        AtariDPad_N,
        AtariDPad_NE,
        AtariDPad_E,
        AtariDPad_SE,
        AtariDPad_S,
        AtariDPad_SW,
        AtariDPad_W,
        AtariDPad_NW,
    };

    struct AtariButtonData {
        u8 A    : 1;
        u8 B    : 1;
        u8 X    : 1;
        u8 Y    : 1;
        u8 LB   : 1;
        u8 RB   : 1;
        u8 L3   : 1;
        u8 R3   : 1;

        u8 back : 1;
        u8 menu : 1;
        u8 home : 1;
        u8      : 1;
        u8 dpad : 4;
    } PACKED;

    struct AtariInputReport0x01 {
        AtariButtonData buttons;
        AnalogStick<s16> left_stick;
        AnalogStick<s16> right_stick;
        u16 left_trigger;
        u16 right_trigger;
    } PACKED;

    struct AtariInputReport0x02 {

    } PACKED;

    struct AtariReportData {
        u8 id;
        union {
            AtariInputReport0x01 input0x01;
            AtariInputReport0x02 input0x02;
        };
    } PACKED;

    class AtariController final : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = {
                {0x3250, 0x1002}    // Atari VCS Wireless Modern Controller
            };

            AtariController(const bluetooth::Address *address, HardwareID id)
            : EmulatedSwitchController(address, id) { }

            void ProcessInputData(const bluetooth::HidReport *report) override;

        private:
            void MapInputReport0x01(const AtariReportData *src);
            void MapInputReport0x02(const AtariReportData *src);

    };

}
