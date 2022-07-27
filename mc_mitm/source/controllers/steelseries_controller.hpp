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

    enum SteelseriesDPadDirection {
        SteelseriesDPad_N,
        SteelseriesDPad_NE,
        SteelseriesDPad_E,
        SteelseriesDPad_SE,
        SteelseriesDPad_S,
        SteelseriesDPad_SW,
        SteelseriesDPad_W,
        SteelseriesDPad_NW,
        SteelseriesDPad_Released = 0x0f,
    };

    enum SteelseriesDPadDirection2 {
        SteelseriesDPad2_Released = 0x0,
        SteelseriesDPad2_N,
        SteelseriesDPad2_NE,
        SteelseriesDPad2_E,
        SteelseriesDPad2_SE,
        SteelseriesDPad2_S,
        SteelseriesDPad2_SW,
        SteelseriesDPad2_W,
        SteelseriesDPad2_NW,
    };

    struct SteelseriesStickData {
        u8 x;
        u8 y;
    } PACKED;

    struct SteelseriesStickData2 {
        s16 x;
        s16 y;
    } PACKED;

    struct SteelseriesButtonData {
        u8 A      : 1;
        u8 B      : 1;
        u8        : 1;
        u8 X      : 1;
        u8 Y      : 1;
        u8        : 1;
        u8 L1     : 1;
        u8 R1     : 1;

        u8 L2     : 1;
        u8 R2     : 1;
        u8        : 1;
        u8 start  : 1;
        u8 select : 1;
        u8 L3     : 1;
        u8 R3     : 1;
        u8        : 0;
    } PACKED;

    struct SteelseriesMfiButtonData {
        u8 dpad_up;
        u8 dpad_right;
        u8 dpad_down;
        u8 dpad_left;
        u8 A;
        u8 B;
        u8 X;
        u8 Y;
        u8 L1;
        u8 R1;
        u8 L2;
        u8 R2;

        u8 menu : 1;
        u8      : 0;
    } PACKED;

    struct SteelseriesMfiInputReport {
        SteelseriesMfiButtonData buttons;
        SteelseriesStickData left_stick;
        SteelseriesStickData right_stick;
    } PACKED;

    struct SteelseriesInputReport0x01 {
        u8 dpad;
        SteelseriesStickData left_stick;
        SteelseriesStickData right_stick;
        SteelseriesButtonData buttons;
    } PACKED;

    struct SteelseriesInputReport0x01_v2 {
        u8 dpad;
        SteelseriesButtonData buttons;
        SteelseriesStickData2 left_stick;
        SteelseriesStickData2 right_stick;
        u16 right_trigger;
        u16 left_trigger;
    } PACKED;

    struct SteelseriesInputReport0x02 {
        u8 select : 1;
        u8 home   : 1;
        u8        : 0;
    } PACKED;

    struct SteelseriesInputReport0x12 {
        u8      : 3;
        u8 home : 1;
        u8      : 0;
        
        u8 _unk0[2];

        u8 _unk1;  // Maybe battery
        
    } PACKED;

    struct SteelseriesInputReport0xc4 {
        SteelseriesStickData left_stick;
        SteelseriesStickData right_stick;
        u8 left_trigger;
        u8 right_trigger;
        SteelseriesButtonData buttons;
        u8 dpad;
        u8 _unk[2];
    } PACKED;

    struct SteelseriesReportData {
        union {
            struct {
                u8 id;
                union {
                    SteelseriesInputReport0x01 input0x01;
                    SteelseriesInputReport0x01_v2 input0x01_v2;
                    SteelseriesInputReport0x02 input0x02;
                    SteelseriesInputReport0x12 input0x12;
                    SteelseriesInputReport0xc4 input0xc4;
                };
            };

            SteelseriesMfiInputReport input_mfi;
        };
    } PACKED;

    class SteelseriesController final : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = {
                {0x1038, 0x1412},   // Steelseries Free
                {0x0111, 0x1420},   // Steelseries Nimbus
                {0x0111, 0x1431},   // Steelseries Stratus Duo
                {0x0111, 0x1419}    // Steelseries Stratus XL
            };

            SteelseriesController(const bluetooth::Address *address, HardwareID id)
            : EmulatedSwitchController(address, id) { }

            void ProcessInputData(const bluetooth::HidReport *report) override;

        private:
            void MapInputReport0x01(const SteelseriesReportData *src);
            void MapInputReport0x01_v2(const SteelseriesReportData *src);
            void MapInputReport0x02(const SteelseriesReportData *src);
            void MapInputReport0x12(const SteelseriesReportData *src);
            void MapInputReport0xc4(const SteelseriesReportData *src);
            void MapMfiInputReport(const SteelseriesReportData *src);
    };

}
