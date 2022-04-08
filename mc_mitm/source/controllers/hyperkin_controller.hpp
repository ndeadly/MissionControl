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

    struct HyperkinStickData {
        uint16_t x;
        uint16_t y;
    } __attribute__ ((__packed__));

    struct HyperkinButtonData {
        uint8_t B       : 1;
        uint8_t A       : 1;
        uint8_t Y       : 1;
        uint8_t X       : 1;
        uint8_t L       : 1;
        uint8_t R       : 1;
        uint8_t         : 0;
        
        uint8_t select  : 1;
        uint8_t start   : 1;
        uint8_t         : 0;

        uint8_t dpad;
    } __attribute__((packed));

    struct HyperkinInputReport0x3f{
        HyperkinButtonData buttons;
        HyperkinStickData left_stick;
        HyperkinStickData right_stick;
        uint8_t unk;
    } __attribute__ ((__packed__));

    struct HyperkinReportData{
        uint8_t id;
        union {
            HyperkinInputReport0x3f input0x3f;
        };
    } __attribute__((packed));

    class HyperkinController : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = { 
                {0x2e24, 0x200a}    // Hyperkin Scout
            };  

            HyperkinController(const bluetooth::Address *address, HardwareID id) 
            : EmulatedSwitchController(address, id) { }

            bool SupportsSetTsiCommand() { return false; }

            void ProcessInputData(const bluetooth::HidReport *report) override;

        private:
            void MapInputReport0x3f(const HyperkinReportData *src);

    };

}
