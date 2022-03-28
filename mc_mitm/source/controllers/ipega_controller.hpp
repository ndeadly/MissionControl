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

    struct IpegaStickData {
        uint8_t x;
        uint8_t y;
    } __attribute__((packed));

    struct IpegaButtonData {
        uint8_t dpad;

        uint8_t A            : 1;
        uint8_t B            : 1;
        uint8_t L3_g910      : 1;
        uint8_t X            : 1;
        uint8_t Y            : 1;
        uint8_t R3_g910      : 1;
        uint8_t LB           : 1;
        uint8_t RB           : 1;

        uint8_t LT           : 1;
        uint8_t RT           : 1;
        uint8_t view         : 1;
        uint8_t menu         : 1;
        uint8_t              : 1;
        uint8_t lstick_press : 1;
        uint8_t rstick_press : 1;
        uint8_t              : 0;
    } __attribute__((packed));

    struct IpegaInputReport0x02 {
        uint8_t         : 7;
        uint8_t home    : 1;
    } __attribute__((packed));

    struct IpegaInputReport0x07 {
        IpegaStickData   left_stick;
        IpegaStickData   right_stick;
        IpegaButtonData  buttons;
        uint8_t          right_trigger;
        uint8_t          left_trigger;
    } __attribute__((packed));

    struct IpegaReportData {
        uint8_t id;
        union {
            IpegaInputReport0x02 input0x02;
            IpegaInputReport0x07 input0x07;
        };
    } __attribute__ ((__packed__));

    class IpegaController : public EmulatedSwitchController {

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
