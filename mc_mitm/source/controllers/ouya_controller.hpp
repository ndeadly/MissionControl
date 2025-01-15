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

    struct OuyaButtonData {
        u8 O            : 1;
        u8 U            : 1;
        u8 Y            : 1;
        u8 A            : 1;
        u8 LB           : 1;
        u8 RB           : 1;
        u8 LS           : 1;
        u8 RS           : 1;

        u8 dpad_up      : 1;
        u8 dpad_down    : 1;
        u8 dpad_left    : 1;
        u8 dpad_right   : 1;
        u8 LT           : 1;
        u8 RT           : 1;
        u8 center_press : 1;
        u8 center_hold  : 1;
    } PACKED;

    struct OuyaInputReport0x03 {
        u8 battery;
        u8 _unk[6];
    } PACKED;

    struct OuyaInputReport0x07 {
        AnalogStick<u16> left_stick;
        AnalogStick<u16> right_stick;
        u16 left_trigger;
        u16 right_trigger;
        OuyaButtonData buttons;
    } PACKED;

    struct OuyaReportData {
        u8 id;
        union {
            OuyaInputReport0x03 input0x03;
            OuyaInputReport0x07 input0x07;
        };
    } PACKED;

    class OuyaController final : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = {
                {0x2836, 0x0001}
            };

            OuyaController(const bluetooth::Address *address, HardwareID id)
            : EmulatedSwitchController(address, id) { }

            void ProcessInputData(const bluetooth::HidReport *report) override;

        private:
            void MapInputReport0x03(const OuyaReportData *src);
            void MapInputReport0x07(const OuyaReportData *src);

    };

}
