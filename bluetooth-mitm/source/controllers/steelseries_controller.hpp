/*
 * Copyright (C) 2020 ndeadly
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
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
        SteelseriesDPad_Released = 0x0f
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
	
	struct SteelseriesInputReport0x01 {
        uint8_t                 dpad;
        SteelseriesStickData    left_stick;
        SteelseriesStickData    right_stick;
        SteelseriesButtonData   buttons;
    } __attribute__((packed));
	
	struct SteelseriesReportData {
        uint8_t id;
        union {
            SteelseriesInputReport0x01  input0x01;
        };
    } __attribute__((packed));

    class SteelseriesController : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = { 
                {0x1038, 0x1412} 
            };  

            SteelseriesController(const bluetooth::Address *address) 
                : EmulatedSwitchController(address) { };

            void ConvertReportFormat(const bluetooth::HidReport *in_report, bluetooth::HidReport *out_report);

        private:
            void HandleInputReport0x01(const SteelseriesReportData *src, SwitchReportData *dst);

    };

}
