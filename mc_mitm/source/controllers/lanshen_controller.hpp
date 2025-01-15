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

    struct LanShenButtonData {
        u8 dpad;

        u8 A     : 1;
        u8 B     : 1;
        u8       : 1;
        u8 X     : 1;
        u8 Y     : 1;
        u8       : 1;
        u8 L1    : 1;
        u8 R1    : 1;

        u8 L2    : 1;
        u8 R2    : 1;
        u8       : 1;
        u8 start : 1;
        u8       : 1;
        u8 L3    : 1;
        u8 R3    : 1;
        u8       : 1;

    } PACKED;

    struct LanShenInputReport0x01{
        AnalogStick<u8> left_stick;
        AnalogStick<u8> right_stick;
        LanShenButtonData buttons;
        u8 _unk[4];
    } PACKED;

    struct LanShenReportData {
        u8 id;
        union {
            LanShenInputReport0x01 input0x01;
        };
    } PACKED;

    class LanShenController final : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = {
                {0x0079, 0x181c}    // LanShen X1Pro
            };

            LanShenController(const bluetooth::Address *address, HardwareID id)
            : EmulatedSwitchController(address, id) { }

            void ProcessInputData(const bluetooth::HidReport *report) override;

        private:
            void MapInputReport0x01(const LanShenReportData *src);

    };

}
