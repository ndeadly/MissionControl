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

    enum MadCatzDPadDirection {
        MadCatzDPad_Released,
        MadCatzDPad_N,
        MadCatzDPad_NE,
        MadCatzDPad_E,
        MadCatzDPad_SE,
        MadCatzDPad_S,
        MadCatzDPad_SW,
        MadCatzDPad_W,
        MadCatzDPad_NW
    };

    struct MadCatzStickData {
        u8 x;
        u8 y;
    } PACKED;

    struct MadCatzButtonData {
        u8 X      : 1;
        u8 A      : 1;
        u8 B      : 1;
        u8 Y      : 1;
        u8 L1     : 1;
        u8 R1     : 1;
        u8 L2     : 1;
        u8 R2     : 1;

        u8 select : 1;
        u8 start  : 1;
        u8 L3     : 1;
        u8 R3     : 1;
        u8 home   : 1;
        u8        : 0;

        u8 dpad;
    } PACKED;

    struct MadCatzInputReport0x01 {
        MadCatzButtonData buttons;
        MadCatzStickData left_stick;
        MadCatzStickData right_stick;
        u8 left_trigger;
        u8 right_trigger;
    } PACKED;

    struct MadCatzInputReport0x02 {
        u8             : 2;
        u8 volume_up   : 1;
        u8 volume_down : 1;
        u8 play        : 1;
        u8 forward     : 1;
        u8 rewind      : 1;
        u8             : 0;
    } PACKED;

    struct MadCatzInputReport0x81 {
        struct {
            union {
                struct {
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
                };

                struct {
                    u8 A     : 1;
                    u8 B     : 1;
                    u8       : 1;
                    u8 X     : 1;
                    u8 Y     : 1;
                    u8       : 1;
                    u8 L1    : 1;
                    u8 R1    : 1;

                    u8       : 3;
                    u8 start : 1;
                    u8       : 1;
                    u8 L3    : 1;
                    u8 R3    : 1;
                    u8       : 0;
                } xinput;
            };

            u8 dpad;
        } buttons;
        MadCatzStickData left_stick;
        MadCatzStickData right_stick;
        u8 left_trigger;
        u8 right_trigger;
        u8 reserved;
    } PACKED;

    struct MadCatzInputReport0x82 {
        struct {
            u8        : 2;
            u8 R1     : 1;
            u8 L1     : 1;
            u8 Y      : 1;
            u8 B      : 1;
            u8 X      : 1;
            u8 select : 1;

            u8 dpad;
        } buttons;
        u8 reserved;
    } PACKED;

    struct MadCatzInputReport0x83 {
        struct {
            u8 R2 : 1;
            u8 L2 : 1;
            u8 R3 : 1;
            u8 L3 : 1;
            u8    : 0;
        } buttons;
        MadCatzStickData left_stick;
        u8 reserved[2];
    } PACKED;

    struct MadCatzReportData {
        u8 id;
        union {
            MadCatzInputReport0x01 input0x01;
            MadCatzInputReport0x02 input0x02;
            MadCatzInputReport0x81 input0x81;
            MadCatzInputReport0x82 input0x82;
            MadCatzInputReport0x83 input0x83;
        };
    } PACKED;

    class MadCatzController final : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = {
                {0x0738, 0x5266},   // Mad Catz C.T.R.L.R
                {0x0738, 0x5250},   // Mad Catz C.T.R.L.R for Samsung
                {0x0738, 0x5269}    // Mad Catz L.Y.N.X. 3
            };

            MadCatzController(const bluetooth::Address *address, HardwareID id)
            : EmulatedSwitchController(address, id) { }

            void ProcessInputData(const bluetooth::HidReport *report) override;

        private:
            void MapInputReport0x01(const MadCatzReportData *src);
            void MapInputReport0x02(const MadCatzReportData *src);
            void MapInputReport0x81(const MadCatzReportData *src);
            void MapInputReport0x82(const MadCatzReportData *src);
            void MapInputReport0x83(const MadCatzReportData *src);

    };

}
