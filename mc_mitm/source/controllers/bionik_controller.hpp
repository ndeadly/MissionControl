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

    enum BionikDPadDirection {
        BionikDPad_N,
        BionikDPad_NE,
        BionikDPad_E,
        BionikDPad_SE,
        BionikDPad_S,
        BionikDPad_SW,
        BionikDPad_W,
        BionikDPad_NW,
        BionikDPad_Released = 0x0f
    };

    struct BionikButtonData {
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
        u8        : 1;
        u8 L3     : 1;
        u8 R3     : 1;
        u8        : 0;

        u8 dpad;
    } PACKED;

    struct BionikInputReport0x03 {
        BionikButtonData buttons;
        AnalogStick<u8> left_stick;
        AnalogStick<u8> right_stick;
        u8 left_trigger;
        u8 right_trigger;
        u8 reserved;
    } PACKED;

    struct BionikInputReport0x04 {
        struct {
            u8      : 1;
            u8 home : 1;
            u8 back : 1;
            u8      : 0;
        } buttons;
        u8 reserved[3];
    };

    struct BionikReportData {
        u8 id;
        union {
            BionikInputReport0x03 input0x03;
            BionikInputReport0x04 input0x04;
        };
    } PACKED;

    class BionikController final : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = {
                {0x2e2c, 0x0002}    // Bionik Vulkan
            };

            BionikController(const bluetooth::Address *address, HardwareID id)
            : EmulatedSwitchController(address, id) { }

            void ProcessInputData(const bluetooth::HidReport *report) override;

        private:
            void MapInputReport0x03(const BionikReportData *src);
            void MapInputReport0x04(const BionikReportData *src);
    };

}
