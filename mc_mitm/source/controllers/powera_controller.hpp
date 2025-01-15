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

    enum PowerADPadDirection {
        PowerADPad_N,
        PowerADPad_NE,
        PowerADPad_E,
        PowerADPad_SE,
        PowerADPad_S,
        PowerADPad_SW,
        PowerADPad_W,
        PowerADPad_NW,
        PowerADPad_Released = 0x0f
    };

    struct PowerAButtonData {
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
        u8        : 0;
    } PACKED;

    struct PowerAInputReport0x03 {
        AnalogStick<u8> left_stick;
        AnalogStick<u8> right_stick;
        PowerAButtonData buttons;
        u8 L2;
        u8 R2;
        u8 battery;
        u8 _unk;
    } PACKED;

    struct PowerAReportData{
        u8 id;
        union {
            PowerAInputReport0x03 input0x03;
        };
    } PACKED;

    class PowerAController final : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = {
                {0x20d6, 0x89e5},   // Moga Hero Controller
                {0x20d6, 0x0dad},   // Moga Pro Controller
                {0x20d6, 0x6271}    // Moga Pro 2 Controller
            };

            PowerAController(const bluetooth::Address *address, HardwareID id)
            : EmulatedSwitchController(address, id) { }

            void ProcessInputData(const bluetooth::HidReport *report) override;

        private:
            void MapInputReport0x03(const PowerAReportData *src);

    };

}
