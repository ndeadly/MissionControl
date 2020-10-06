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

    enum EightBitDoDPadDirection : uint16_t {
        EightBitDoDPad_Released = 0x0000,
        EightBitDoDPad_N        = 0x0052,
        EightBitDoDPad_NE       = 0x524f,
        EightBitDoDPad_E        = 0x004f,
        EightBitDoDPad_SE       = 0x4f51,
        EightBitDoDPad_S        = 0x0051,
        EightBitDoDPad_SW       = 0x5150,
        EightBitDoDPad_W        = 0x0050,   
        EightBitDoDPad_NW       = 0x5250,     
    };

    struct EightBitDoStickData {
        uint8_t x;
        uint8_t y;
    } __attribute__((packed));

    struct EightBitDoButtonData {
        uint8_t A       : 1;
        uint8_t B       : 1;
        uint8_t         : 1;
        uint8_t X       : 1;
        uint8_t Y       : 1;
        uint8_t         : 1;
        uint8_t L       : 1;
        uint8_t R       : 1;

        uint8_t         : 2;
        uint8_t select  : 1;
        uint8_t start   : 1;
        uint8_t         : 0;
    }__attribute__((packed));

    struct EightBitDoInputReport0x01 {
        uint8_t _unk0[2];
        uint16_t dpad;
        uint8_t _unk1[4];
    } __attribute__((packed));

    struct EightBitDoInputReport0x03 {
        uint8_t dpad;
        EightBitDoStickData left_stick;
        EightBitDoStickData right_stick;
        uint8_t _unk[3];
        EightBitDoButtonData buttons;
    } __attribute__((packed));

    struct EightBitDoReportData{
        uint8_t id;
        union {
            EightBitDoInputReport0x01 input0x01;
            EightBitDoInputReport0x03 input0x03;
        };
    } __attribute__((packed));

    class EightBitDoController : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = { 
                {0x05a0, 0x3232}    // 8BitDo Zero
            };  

            EightBitDoController(const bluetooth::Address *address) 
                : EmulatedSwitchController(address) { };

            void UpdateControllerState(const bluetooth::HidReport *report);

        private:
            void HandleInputReport0x01(const EightBitDoReportData *src);
            void HandleInputReport0x03(const EightBitDoReportData *src);

    };


}
