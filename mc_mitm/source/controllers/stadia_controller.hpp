/*
 * Copyright (c) 2020-2026 ndeadly
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

    enum StadiaDpadDirection : u8 {
        StadiaDpad_N    = 0x00,
        StadiaDpad_NE   = 0x02,
        StadiaDpad_E    = 0x04,
        StadiaDpad_SE   = 0x06,
        StadiaDpad_None = 0x08,
        StadiaDpad_S    = 0x0A,
        StadiaDpad_SW   = 0x0C,
        StadiaDpad_W    = 0x0E,
        StadiaDpad_NW   = 0x10,
    };

    struct StadiaButtonData0 {
        u8 capture    : 1;  // bit 0
        u8 assistant  : 1;  // bit 1
        u8 lt_digital : 1;  // bit 2
        u8 rt_digital : 1;  // bit 3
        u8 home       : 1;  // bit 4  (Stadia logo)
        u8 start      : 1;  // bit 5
        u8 select     : 1;  // bit 6
        u8 lstick     : 1;  // bit 7  (L3)
    } PACKED;

    struct StadiaButtonData1 {
        u8 rstick : 1;  // bit 0  (R3)
        u8 rb     : 1;  // bit 1
        u8 lb     : 1;  // bit 2
        u8 y      : 1;  // bit 3
        u8 x      : 1;  // bit 4
        u8 b      : 1;  // bit 5
        u8 a      : 1;  // bit 6
        u8        : 1;
    } PACKED;

    struct StadiaInputReport0x03 {
        u8 id;
        u8 dpad;
        StadiaButtonData0 buttons0;
        StadiaButtonData1 buttons1;
        AnalogStick<u8> left_stick;
        AnalogStick<u8> right_stick;
        u8 left_trigger;
        u8 right_trigger;
        u8 _reserved;
    } PACKED;

    struct StadiaReportData {
        u8 id;
        union {
            StadiaInputReport0x03 input0x03;
        };
    } PACKED;

    class StadiaController final : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = {
                { 0x18d1, 0x9400 },  // Google Stadia Controller (GA00722-GB)
            };

            StadiaController(bluetooth::Address address, HardwareID id)
            : EmulatedSwitchController(address, id) { }

            void ParseInputReport(const u8 *report_buffer, size_t size) override;

        private:
            void MapInputReport0x03(const StadiaReportData *src);

    };

}
