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

    enum IpegaDPadDirection {
        IpegaDPad_N,
        IpegaDPad_NE,
        IpegaDPad_E,
        IpegaDPad_SE,
        IpegaDPad_S,
        IpegaDPad_SW,
        IpegaDPad_W,
        IpegaDPad_NW,
        IpegaDPad_Released = 0x88
    };

    struct IpegaButtonData {
        u8 dpad;

        u8 A            : 1;
        u8 B            : 1;
        u8 L3_g910      : 1;
        u8 X            : 1;
        u8 Y            : 1;
        u8 R3_g910      : 1;
        u8 LB           : 1;
        u8 RB           : 1;

        u8 LT           : 1;
        u8 RT           : 1;
        u8 view         : 1;
        u8 menu         : 1;
        u8              : 1;
        u8 lstick_press : 1;
        u8 rstick_press : 1;
        u8              : 0;
    } PACKED;

    struct IpegaInputReport0x02 {
        u8      : 7;
        u8 home : 1;
    } PACKED;

    struct IpegaInputReport0x07 {
        AnalogStick<u8> left_stick;
        AnalogStick<u8> right_stick;
        IpegaButtonData buttons;
        u8 right_trigger;
        u8 left_trigger;
    } PACKED;

    struct IpegaReportData {
        u8 id;
        union {
            IpegaInputReport0x02 input0x02;
            IpegaInputReport0x07 input0x07;
        };
    } PACKED;

    class IpegaController final : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = {
                {0x1949, 0x0402},
                {0x1949, 0x0403},
                {0x05ac, 0x022c}    // ipega 9017S (Another fucking Apple keyboard ID. Eventually these are going to clash)
            };

            IpegaController(const bluetooth::Address *address, HardwareID id)
            : EmulatedSwitchController(address, id) { }

            void ProcessInputData(const bluetooth::HidReport *report) override;

        private:
            void MapInputReport0x02(const IpegaReportData *src);
            void MapInputReport0x07(const IpegaReportData *src);

    };

}
