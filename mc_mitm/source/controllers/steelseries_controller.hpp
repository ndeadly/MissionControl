/*
 * Copyright (c) 2020-2022 ndeadly
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
        uint8_t x;
        uint8_t y;
    } __attribute__ ((__packed__));

    struct SteelseriesButtonData {
        uint8_t A       : 1;
        uint8_t B       : 1;
        uint8_t         : 1;
        uint8_t X       : 1;
        uint8_t Y       : 1;
        uint8_t         : 1;
        uint8_t L       : 1;
        uint8_t R       : 1;

        uint8_t         : 3;
        uint8_t start   : 1;
        uint8_t select  : 1;
        uint8_t         : 0;
    } __attribute__ ((__packed__));

    struct SteelseriesButtonData2 {
        uint8_t A       : 1;
        uint8_t B       : 1;
        uint8_t         : 1;
        uint8_t X       : 1;
        uint8_t Y       : 1;
        uint8_t         : 1;
        uint8_t L1      : 1;
        uint8_t R1      : 1;

        uint8_t L2      : 1;
        uint8_t R2      : 1;
        uint8_t start   : 1;
        uint8_t select  : 1;
        uint8_t         : 1;
        uint8_t L3      : 1;
        uint8_t R3      : 1;
        uint8_t         : 0;
    } __attribute__ ((__packed__));

    struct SteelseriesMfiButtonData {
        uint8_t dpad_up;
        uint8_t dpad_right;
        uint8_t dpad_down;
        uint8_t dpad_left;
        uint8_t A;
        uint8_t B;
        uint8_t X;
        uint8_t Y;
        uint8_t L1;
        uint8_t R1;
        uint8_t L2;
        uint8_t R2;

        uint8_t menu : 1;
        uint8_t      : 0;
    } __attribute__ ((__packed__));

    struct SteelseriesMfiInputReport {
        SteelseriesMfiButtonData buttons;
        SteelseriesStickData left_stick;
        SteelseriesStickData right_stick;
    } __attribute__((packed));

    struct SteelseriesInputReport0x01 {
        uint8_t dpad;
        SteelseriesStickData left_stick;
        SteelseriesStickData right_stick;
        SteelseriesButtonData buttons;
    } __attribute__((packed));

    struct SteelseriesInputReport0x12 {
        uint8_t      : 3;
        uint8_t home : 1;
        uint8_t      : 0;
        
        uint8_t _unk0[2];

        uint8_t _unk1;  // Maybe battery
        
    } __attribute__((packed));

    struct SteelseriesInputReport0xc4 {
        SteelseriesStickData left_stick;
        SteelseriesStickData right_stick;
        uint8_t left_trigger;
        uint8_t right_trigger;
        SteelseriesButtonData2 buttons;
        uint8_t dpad;
        uint8_t _unk[2];
    } __attribute__((packed));

    struct SteelseriesReportData {
        union {
            struct {
                uint8_t id;
                union {
                    SteelseriesInputReport0x01 input0x01;
                    SteelseriesInputReport0x12 input0x12;
                    SteelseriesInputReport0xc4 input0xc4;
                };
            };

            SteelseriesMfiInputReport input_mfi;
        };
    } __attribute__((packed));

    class SteelseriesController : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = {
                {0x1038, 0x1412},   // Steelseries Free
                {0x0111, 0x1420},   // Steelseries Nimbus
                {0x0111, 0x1431}    // Steelseries Stratus Duo
            };

            SteelseriesController(const bluetooth::Address *address, HardwareID id)
            : EmulatedSwitchController(address, id) { }

            bool SupportsSetTsiCommand() { return !(m_id.pid == 0x1412); }

            void ProcessInputData(const bluetooth::HidReport *report) override;

        private:
            void MapInputReport0x01(const SteelseriesReportData *src);
            void MapInputReport0x12(const SteelseriesReportData *src);
            void MapInputReport0xc4(const SteelseriesReportData *src);
            void MapMfiInputReport(const SteelseriesReportData *src);
    };

}
