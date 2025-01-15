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

    enum RazerDPadDirection {
        RazerDPad_N,
        RazerDPad_NE,
        RazerDPad_E,
        RazerDPad_SE,
        RazerDPad_S,
        RazerDPad_SW,
        RazerDPad_W,
        RazerDPad_NW,
        RazerDPad_Released
    };

    struct RazerButtonData {
        u8 dpad   : 4;
        u8 A      : 1;
        u8 B      : 1;
        u8 X      : 1;
        u8 Y      : 1;

        u8 L1     : 1;
        u8 R1     : 1;
        u8 back   : 1;
        u8 start  : 1;
        u8 L3     : 1;
        u8 R3     : 1;
        u8        : 1;
        u8 home   : 1;

        u8 select : 1;
        u8        : 0;
    } PACKED;

    struct RazerInputReport0x01 {
        AnalogStick<u8> left_stick;
        AnalogStick<u8> right_stick;
        RazerButtonData buttons;
        u8 left_trigger;
        u8 right_trigger;
    } PACKED;

    struct RazerReportData{
        u8 id;
        union {
            RazerInputReport0x01 input0x01;
        };
    } PACKED;

    class RazerController final : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = {
                {0x1532, 0x0900}    // Razer Serval
            };

            RazerController(const bluetooth::Address *address, HardwareID id)
            : EmulatedSwitchController(address, id) { }

            void ProcessInputData(const bluetooth::HidReport *report) override;

        private:
            void MapInputReport0x01(const RazerReportData *src);

    };

}
