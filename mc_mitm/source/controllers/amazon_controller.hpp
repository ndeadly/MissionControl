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

    struct AmazonButtonData {
        u8 A      : 1;
        u8 B      : 1;
        u8        : 1;
        u8 X      : 1;
        u8 Y      : 1;
        u8        : 1;
        u8 L1     : 1;
        u8 R1     : 1;

        u8        : 1;
        u8        : 1;
        u8 back   : 1;
        u8 menu   : 1;
        u8 middle : 1;
        u8 L3     : 1;
        u8 R3     : 1;
        u8        : 0;
    } PACKED;

    struct AmazonInputReport0x01 {
        AnalogStick<u8> left_stick;
        AnalogStick<u8> right_stick;
        u8 left_trigger;
        u8 right_trigger;
        AmazonButtonData buttons;
        DirectionalPadType<1, 0> dpad;
        u8 battery;
    } PACKED;

    struct AmazonInputReport0x02 {
        u8      : 4;
        u8 home : 1;
        u8      : 0;
    } PACKED;

    struct AmazonReportData {
        u8 id;
        union {
            AmazonInputReport0x01 input0x01;
            AmazonInputReport0x02 input0x02;
        };
    } PACKED;

    class AmazonController final : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = {
                {0x1949, 0x0402}    // Amazon Fire Game Controller
            };

            static constexpr const char FireGameControllerName[] = "Amazon Fire Game Controller";

            AmazonController(bluetooth::Address address, HardwareID id) : EmulatedSwitchController(address, id) { }

            virtual void ParseInputReport(const u8 *report_buffer, size_t size) override;

        private:
            void MapInputReport0x01(const AmazonReportData *src);
            void MapInputReport0x02(const AmazonReportData *src);
    };

}
