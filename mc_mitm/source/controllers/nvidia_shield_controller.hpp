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

    enum NvidiaShieldDPadDirection {
        NvidiaShieldDPad_N,
        NvidiaShieldDPad_NE,
        NvidiaShieldDPad_E,
        NvidiaShieldDPad_SE,
        NvidiaShieldDPad_S,
        NvidiaShieldDPad_SW,
        NvidiaShieldDPad_W,
        NvidiaShieldDPad_NW,
        NvidiaShieldDPad_Released = 0x80
    };

    struct NvidiaShieldButtonData {
        u8 A     : 1;
        u8 B     : 1;
        u8 X     : 1;
        u8 Y     : 1;
        u8 LB    : 1;
        u8 RB    : 1;
        u8 L3    : 1;
        u8 R3    : 1;

        u8 start : 1;
        u8       : 0;
    } PACKED;

    struct NvidiaShieldInputReport0x01 {
        u8 _unk0;  // maybe a counter?
        u8 dpad;
        NvidiaShieldButtonData buttons;
        u16 left_trigger;
        u16 right_trigger;
        AnalogStick<u16> left_stick;
        AnalogStick<u16> right_stick;
        u8 home : 1;
        u8 back : 1;
        u8      : 0;
    } PACKED;

    struct NvidiaShieldInputReport0x03 {
        u8 _unk[15];
    } PACKED;

    struct NvidiaShieldReportData{
        u8 id;
        union {
            NvidiaShieldInputReport0x01 input0x01;
            NvidiaShieldInputReport0x03 input0x03;
        };
    } PACKED;

    class NvidiaShieldController final : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = {
                {0x0955, 0x7214}    // Nvidia Shield Controller (2017) v1.04
            };

            NvidiaShieldController(const bluetooth::Address *address, HardwareID id)
            : EmulatedSwitchController(address, id) { }

            void ProcessInputData(const bluetooth::HidReport *report) override;

        private:
            void MapInputReport0x01(const NvidiaShieldReportData *src);
            void MapInputReport0x03(const NvidiaShieldReportData *src);

    };

}
