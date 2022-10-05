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

    enum LanShenDPadDirection {
        LanShenDPad_N,
        LanShenDPad_NE,
        LanShenDPad_E,
        LanShenDPad_SE,
        LanShenDPad_S,
        LanShenDPad_SW,
        LanShenDPad_W,
        LanShenDPad_NW,
        LanShenDPad_Released = 0x0f
    };

    struct LanShenStickData {
        uint8_t x;
        uint8_t y;
    } __attribute__ ((__packed__));

    struct LanShenButtonData {
        uint8_t dpad;

        uint8_t A       : 1;
        uint8_t B       : 1;
        uint8_t         : 1;
        uint8_t X       : 1;
        uint8_t Y       : 1;
        uint8_t         : 1;
        uint8_t L1      : 1;
        uint8_t R1      : 1;

        uint8_t L2      : 1;
        uint8_t R2      : 1;
        uint8_t         : 1;
        uint8_t start   : 1;
        uint8_t         : 1;
        uint8_t L3      : 1;
        uint8_t R3      : 1;
        uint8_t         : 1;

    } __attribute__ ((__packed__));

    struct LanShenInputReport0x01{
        LanShenStickData left_stick;
        LanShenStickData right_stick;
        LanShenButtonData buttons;
        uint8_t _unk[4];
    } __attribute__ ((__packed__));

    struct LanShenReportData {
        uint8_t id;
        union {
            LanShenInputReport0x01 input0x01;
        };
    } __attribute__((packed));

    class LanShenController : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = {
                {0x0079, 0x181c}    // LanShen X1Pro
            };

            LanShenController(const bluetooth::Address *address, HardwareID id)
            : EmulatedSwitchController(address, id) { }

            bool SupportsSetTsiCommand() { return false; }

            void ProcessInputData(const bluetooth::HidReport *report) override;

        private:
            void MapInputReport0x01(const LanShenReportData *src);

    };

}
