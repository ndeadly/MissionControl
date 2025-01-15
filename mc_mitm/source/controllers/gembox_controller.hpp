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

    enum GemboxDPadDirection {
        GemboxDPad_N,
        GemboxDPad_NE,
        GemboxDPad_E,
        GemboxDPad_SE,
        GemboxDPad_S,
        GemboxDPad_SW,
        GemboxDPad_W,
        GemboxDPad_NW,
        GemboxDPad_Released = 0x0f
    };

    struct GemboxButtonData {
        u8 A     : 1;
        u8 B     : 1;
        u8       : 1;
        u8 X     : 1;
        u8 Y     : 1;
        u8       : 1;
        u8 LB    : 1;
        u8 RB    : 1;

        u8       : 3;
        u8 start : 1;
        u8       : 1;
        u8 L3    : 1;
        u8 R3    : 1;
        u8       : 0;
    } PACKED;

    struct GemboxInputReport0x02 {
        union {
            struct {
                u8      : 6;
                u8 back : 1;
                u8      : 0;
            };

            u8 buttons;
        };
    } PACKED;

    struct GemboxInputReport0x07 {
        u8 dpad;
        AnalogStick<s8> left_stick;
        AnalogStick<s8> right_stick;
        u8 left_trigger;
        u8 right_trigger;
        GemboxButtonData buttons;
    } PACKED;

    struct GemboxReportData {
        u8 id;
        union {
            GemboxInputReport0x02  input0x02;
            GemboxInputReport0x07  input0x07;
        };
    } PACKED;

    class GemboxController final : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = {
                {0x1d79, 0x0009}
            };

            GemboxController(const bluetooth::Address *address, HardwareID id)
            : EmulatedSwitchController(address, id) { }

            void ProcessInputData(const bluetooth::HidReport *report) override;

        private:
            void MapInputReport0x02(const GemboxReportData *src);
            void MapInputReport0x07(const GemboxReportData *src);

    };

}
