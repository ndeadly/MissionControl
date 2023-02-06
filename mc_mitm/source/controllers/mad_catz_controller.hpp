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
        uint8_t x;
        uint8_t y;
    } __attribute__ ((__packed__));

    struct MadCatzButtonData {
        uint8_t X       : 1;
        uint8_t A       : 1;
        uint8_t B       : 1;
        uint8_t Y       : 1;
        uint8_t L1      : 1;
        uint8_t R1      : 1;
        uint8_t L2      : 1;
        uint8_t R2      : 1;

        uint8_t select  : 1;
        uint8_t start   : 1;
        uint8_t L3      : 1;
        uint8_t R3      : 1;
        uint8_t home    : 1;
        uint8_t         : 0;

        uint8_t dpad;
    } __attribute__ ((__packed__));

    struct MadCatzInputReport0x01 {
        MadCatzButtonData buttons;
        MadCatzStickData left_stick;
        MadCatzStickData right_stick;
        uint8_t left_trigger;
        uint8_t right_trigger;
    } __attribute__ ((__packed__)); 

    struct MadCatzInputReport0x02 {
        uint8_t             : 2;
        uint8_t volume_up   : 1;
        uint8_t volume_down : 1;
        uint8_t play        : 1;
        uint8_t forward     : 1;
        uint8_t rewind      : 1;
        uint8_t             : 0;
    } __attribute__ ((__packed__)); 

    struct MadCatzInputReport0x81 {
        struct {
            union {
                struct {
                    uint8_t A      : 1;
                    uint8_t B      : 1;
                    uint8_t X      : 1;
                    uint8_t Y      : 1;
                    uint8_t L1     : 1;
                    uint8_t R1     : 1;
                    uint8_t select : 1;
                    uint8_t start  : 1;

                    uint8_t L3     : 1;
                    uint8_t R3     : 1;
                    uint8_t        : 0;
                };

                struct {
                    uint8_t A     : 1;
                    uint8_t B     : 1;
                    uint8_t       : 1;
                    uint8_t X     : 1;
                    uint8_t Y     : 1;
                    uint8_t       : 1;
                    uint8_t L1    : 1;
                    uint8_t R1    : 1;

                    uint8_t       : 3;
                    uint8_t start : 1;
                    uint8_t       : 1;
                    uint8_t L3    : 1;
                    uint8_t R3    : 1;
                    uint8_t       : 0;
                } xinput;
            };

            uint8_t dpad;
        } buttons;
        MadCatzStickData left_stick;
        MadCatzStickData right_stick;
        uint8_t left_trigger;
        uint8_t right_trigger;
        uint8_t reserved;
    } __attribute__ ((__packed__));

    struct MadCatzInputReport0x82 {
        struct {
            uint8_t        : 2;
            uint8_t R1     : 1;
            uint8_t L1     : 1;
            uint8_t Y      : 1;
            uint8_t B      : 1;
            uint8_t X      : 1;
            uint8_t select : 1;

            uint8_t dpad;
        } buttons;
        uint8_t reserved;
    } __attribute__ ((__packed__));

    struct MadCatzInputReport0x83 {
        struct {
            uint8_t R2 : 1;
            uint8_t L2 : 1;
            uint8_t R3 : 1;
            uint8_t L3 : 1;
            uint8_t    : 0;
        } buttons;
        MadCatzStickData left_stick;
        uint8_t reserved[2];
    } __attribute__ ((__packed__));

    struct MadCatzReportData {
        uint8_t id;
        union {
            MadCatzInputReport0x01 input0x01;
            MadCatzInputReport0x02 input0x02;
            MadCatzInputReport0x81 input0x81;
            MadCatzInputReport0x82 input0x82;
            MadCatzInputReport0x83 input0x83;
        };
    } __attribute__((packed));

    class MadCatzController : public EmulatedSwitchController {

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
