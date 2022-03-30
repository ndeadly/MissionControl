/*
 * Copyright (c) 2020-2021 ndeadly
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

    enum EightBitDoReportFormat {
        EightBitDoReportFormat_ZeroV1,
        EightBitDoReportFormat_ZeroV2,
        EightBitDoReportFormat_Other
    };

    enum EightBitDoDPadDirectionV1 : uint16_t {
        EightBitDoDPadV1_Released = 0x0000,
        EightBitDoDPadV1_N        = 0x0052,
        EightBitDoDPadV1_NE       = 0x524f,
        EightBitDoDPadV1_E        = 0x004f,
        EightBitDoDPadV1_SE       = 0x4f51,
        EightBitDoDPadV1_S        = 0x0051,
        EightBitDoDPadV1_SW       = 0x5150,
        EightBitDoDPadV1_W        = 0x0050,   
        EightBitDoDPadV1_NW       = 0x5250,     
    };

    enum EightBitDoDPadDirectionV2 {
        EightBitDoDPadV2_N,
        EightBitDoDPadV2_NE,
        EightBitDoDPadV2_E,
        EightBitDoDPadV2_SE,
        EightBitDoDPadV2_S,
        EightBitDoDPadV2_SW,
        EightBitDoDPadV2_W,
        EightBitDoDPadV2_NW,
        EightBitDoDPadV2_Released
    };

    struct EightBitDoStickData8 {
        uint8_t x;
        uint8_t y;
    } __attribute__((packed));

    struct EightBitDoStickData16 {
        uint16_t x;
        uint16_t y;
    } __attribute__((packed));

    struct EightBitDoButtonDataV1 {
        uint8_t A       : 1;
        uint8_t B       : 1;
        uint8_t         : 1;
        uint8_t X       : 1;
        uint8_t Y       : 1;
        uint8_t         : 1;
        uint8_t L1      : 1;
        uint8_t R1      : 1;

        uint8_t         : 2;
        uint8_t select  : 1;
        uint8_t start   : 1;
		uint8_t			: 0;
    }__attribute__((packed));

    struct EightBitDoButtonDataV2 {
        uint8_t A       : 1;
        uint8_t B       : 1;
        uint8_t         : 1;
        uint8_t X       : 1;
        uint8_t Y       : 1;
        uint8_t         : 1;
        uint8_t L1      : 1;
        uint8_t R1      : 1;

        uint8_t select  : 1;
        uint8_t         : 2;
        uint8_t start   : 1;
        uint8_t home    : 1;
        uint8_t L3		: 1;
		uint8_t R3		: 1;
		uint8_t			: 1;

        uint8_t dpad;
    }__attribute__((packed));

    struct EightBitDoInputReport0x01V1 {
        uint8_t _unk0[2];
        uint16_t dpad;
        uint8_t _unk1[4];
    } __attribute__((packed));

    struct EightBitDoInputReport0x01V2 {
        EightBitDoButtonDataV2 buttons;
        EightBitDoStickData16 left_stick;
        EightBitDoStickData16 right_stick;
        uint8_t left_trigger;
        uint8_t right_trigger;
        uint8_t _unk0;
    } __attribute__((packed));

    struct EightBitDoInputReport0x03V1 {
        uint8_t dpad;
        EightBitDoStickData8 left_stick;
        EightBitDoStickData8 right_stick;
        uint8_t _unk[3];
        EightBitDoButtonDataV1 buttons;
    } __attribute__((packed));

    struct EightBitDoInputReport0x03V2 {
        uint8_t dpad;
        EightBitDoStickData8 left_stick;
        EightBitDoStickData8 right_stick;
        uint8_t _unk[2];
        EightBitDoButtonDataV1 buttons;
    } __attribute__((packed));

    struct EightBitDoReportData {
        uint8_t id;
        union {
            EightBitDoInputReport0x01V1 input0x01_v1;
            EightBitDoInputReport0x01V2 input0x01_v2;
            EightBitDoInputReport0x03V1 input0x03_v1;
            EightBitDoInputReport0x03V2 input0x03_v2;
        };
    } __attribute__((packed));

    class EightBitDoController : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = {
                {0x05a0, 0x3232}, // 8BitDo Zero
                {0x2dc8, 0x2100}  // 8BitDo SN30 Pro for Xbox Cloud Gaming
            };  

            EightBitDoController(const bluetooth::Address *address, HardwareID id)
            : EmulatedSwitchController(address, id) { }

            bool SupportsSetTsiCommand(void) { return !((m_id.vid == 0x05a0) && (m_id.pid == 0x3232)); }

            void ProcessInputData(const bluetooth::HidReport *report) override;

        private:
            void MapInputReport0x01(const EightBitDoReportData *src, EightBitDoReportFormat fmt);
            void MapInputReport0x03(const EightBitDoReportData *src, EightBitDoReportFormat fmt);

    };

}
