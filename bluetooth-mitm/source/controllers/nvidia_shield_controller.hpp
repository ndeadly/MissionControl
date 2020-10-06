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

    enum NvidiaShieldDPadDirection {
        NvidiaShieldDPad_N,
        NvidiaShieldDPad_NE,
        NvidiaShieldDPad_E,
        NvidiaShieldDPad_SE,
        NvidiaShieldDPad_S,
        NvidiaShieldDPad_SW,
        NvidiaShieldDPad_W,
        NvidiaShieldDPad_NW,
        NvidiaShieldDPad_Released = 0x80
    };

    struct NvidiaShieldStickData {
        uint16_t x;
        uint16_t y;
    } __attribute__((packed));

    struct NvidiaShieldButtonData {
        uint8_t A       : 1;
        uint8_t B       : 1;
        uint8_t X       : 1;
        uint8_t Y       : 1;
        uint8_t LB      : 1;
        uint8_t RB      : 1;
        uint8_t L3      : 1;
        uint8_t R3      : 1;

        uint8_t start   : 1;
        uint8_t         : 0;
    } __attribute__((packed));

    struct NvidiaShieldInputReport0x01 {
        uint8_t _unk0;  // maybe a counter?
        uint8_t dpad;
        NvidiaShieldButtonData buttons;
        uint16_t left_trigger;
        uint16_t right_trigger;
        NvidiaShieldStickData left_stick;
        NvidiaShieldStickData right_stick;
        uint8_t home    : 1;
        uint8_t back    : 1;
        uint8_t         : 0;
    } __attribute__((packed));

    struct NvidiaShieldInputReport0x03 {
        uint8_t _unk[15];
    } __attribute__((packed));

    struct NvidiaShieldReportData{
        uint8_t id;
        union {
            NvidiaShieldInputReport0x01 input0x01;
            NvidiaShieldInputReport0x03 input0x03;
        };
    } __attribute__((packed));

    class NvidiaShieldController : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = { 
                {0x0955, 0x7214}    // Nvidia Shield Controller (2017) v1.04 
            };  

            NvidiaShieldController(const bluetooth::Address *address) 
                : EmulatedSwitchController(address) { };

            void UpdateControllerState(const bluetooth::HidReport *report);

        private:
            void HandleInputReport0x01(const NvidiaShieldReportData *src);
            void HandleInputReport0x03(const NvidiaShieldReportData *src);

    };

}
