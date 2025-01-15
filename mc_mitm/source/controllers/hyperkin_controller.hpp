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

    enum HyperkinDPadDirection {
        HyperkinDPad_Released,
        HyperkinDPad_N,
        HyperkinDPad_NE,
        HyperkinDPad_E,
        HyperkinDPad_SE,
        HyperkinDPad_S,
        HyperkinDPad_SW,
        HyperkinDPad_W,
        HyperkinDPad_NW
    };

    struct HyperkinButtonData {
        u8 B      : 1;
        u8 A      : 1;
        u8 Y      : 1;
        u8 X      : 1;
        u8 L      : 1;
        u8 R      : 1;
        u8        : 0;
        
        u8 select : 1;
        u8 start  : 1;
        u8        : 0;

        u8 dpad;
    } PACKED;

    struct HyperkinInputReport0x3f{
        HyperkinButtonData buttons;
        AnalogStick<u16> left_stick;
        AnalogStick<u16> right_stick;
        u8 unk;
    } PACKED;

    struct HyperkinReportData{
        u8 id;
        union {
            HyperkinInputReport0x3f input0x3f;
        };
    } PACKED;

    class HyperkinController final : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = {
                {0x2e24, 0x200a}    // Hyperkin Scout
            };

            HyperkinController(const bluetooth::Address *address, HardwareID id)
            : EmulatedSwitchController(address, id) { }

            void ProcessInputData(const bluetooth::HidReport *report) override;

        private:
            void MapInputReport0x3f(const HyperkinReportData *src);

    };

}
