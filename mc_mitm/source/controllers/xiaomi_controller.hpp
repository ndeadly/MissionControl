/*
 * Copyright (c) 2020-2023 ndeadly
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

    enum XiaomiDPadDirection {
        XiaomiDPad_N,
        XiaomiDPad_NE,
        XiaomiDPad_E,
        XiaomiDPad_SE,
        XiaomiDPad_S,
        XiaomiDPad_SW,
        XiaomiDPad_W,
        XiaomiDPad_NW,
        XiaomiDPad_Released = 0x0f
    };

    struct XiaomiStickData {
        u8 x;
        u8 y;
    } PACKED;

    struct XiaomiButtonData {
        u8 A            : 1;
        u8 B            : 1;
        u8              : 1;
        u8 X            : 1;
        u8 Y            : 1;
        u8              : 1;
        u8 L1           : 1;
        u8 R1           : 1;

        u8 L2           : 1;
        u8 R2           : 1;
        u8 back         : 1;
        u8 menu         : 1;
        u8              : 1;
        u8 lstick_press : 1;
        u8 rstick_press : 1;
        u8              : 0;

        u8 _unk;

        u8 dpad;
    } PACKED;

    struct XiaomiInputReport0x04 {
        XiaomiButtonData buttons;
        XiaomiStickData left_stick;
        XiaomiStickData right_stick;
        u8 _unk0[2];
        u8 left_trigger;
        u8 right_trigger;
        u16 accel_x;
        u16 accel_y;
        u16 accel_z;
        u8 battery;
        u8 home : 1;
        u8      : 0;
    } PACKED;

    struct XiaomiReportData {
        u8 id;
        union {
            XiaomiInputReport0x04 input0x04;
        };
    } PACKED;

    class XiaomiController final : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = {
                {0x2717, 0x3144}    // Xiaomi Mi Controller
            };

            XiaomiController(const bluetooth::Address *address, HardwareID id)
            : EmulatedSwitchController(address, id) { }

            Result Initialize();

            void ProcessInputData(const bluetooth::HidReport *report) override;

        private:
            void MapInputReport0x04(const XiaomiReportData *src);
    };

}
