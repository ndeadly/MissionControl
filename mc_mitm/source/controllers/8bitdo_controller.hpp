/*
 * Copyright (c) 2020-2023 ndeadly
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

    enum EightBitDoControllerType {
        EightBitDoControllerType_Zero,
        EightBitDoControllerType_Sn30ProXboxCloud,
        EightBitDoControllerType_Sn30ProXboxCloudFwV2,
        EightBitDoControllerType_Ultimate24gWireless,
        EightBitDoControllerType_Other
    };

    enum EightBitDoReportFormat {
        EightBitDoReportFormat_ZeroV1,
        EightBitDoReportFormat_ZeroV2,
        EightBitDoReportFormat_Other
    };

    enum EightBitDoDPadDirectionV1 : u16 {
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
        u8 x;
        u8 y;
    } PACKED;

    struct EightBitDoStickData16 {
        u16 x;
        u16 y;
    } PACKED;

    struct EightBitDoButtonData {
        u8 A              : 1;
        u8 B              : 1;
        u8                : 1;
        u8 X              : 1;
        u8 Y              : 1;
        u8                : 1;
        u8 L1             : 1;
        u8 R1             : 1;

        union {
            struct {
                u8 select : 1;
                u8        : 2;
                u8 start  : 1;
                u8 home   : 1;
                u8 L3     : 1;
                u8 R3     : 1;
                u8        : 1;
            } v1;

            struct {
                u8 L2     : 1;
                u8 R2     : 1;
                u8 select : 1;
                u8 start  : 1;
                u8 home   : 1;
                u8 L3     : 1;
                u8 R3     : 1;
                u8        : 1;
            } v2;
        };
    } PACKED;

    struct EightBitDoInputReport0x01V1 {
        u8 _unk0[2];
        u16 dpad;
        u8 _unk1[4];
    } PACKED;

    struct EightBitDoInputReport0x01V2 {
        EightBitDoButtonData buttons;
        u8 dpad;
        EightBitDoStickData16 left_stick;
        EightBitDoStickData16 right_stick;
        u8 left_trigger;
        u8 right_trigger;
        u8 _unk0;
    } PACKED;

    struct EightBitDoInputReport0x03V1 {
        u8 dpad;
        EightBitDoStickData8 left_stick;
        EightBitDoStickData8 right_stick;
        u8 _unk[3];
        EightBitDoButtonData buttons;
    } PACKED;

    struct EightBitDoInputReport0x03V2 {
        u8 dpad;
        EightBitDoStickData8 left_stick;
        EightBitDoStickData8 right_stick;
        u8 _unk[2];
        EightBitDoButtonData buttons;
    } PACKED;

    struct EightBitDoInputReport0x03V3 {
        u8 dpad;
        EightBitDoStickData8 left_stick;
        EightBitDoStickData8 right_stick;
        u8 right_trigger;
        u8 left_trigger;
        EightBitDoButtonData buttons;
        u8 battery;
        u8 _unk0;
    } PACKED;

    struct EightBitDoReportData {
        u8 id;
        union {
            EightBitDoInputReport0x01V1 input0x01_v1;
            EightBitDoInputReport0x01V2 input0x01_v2;
            EightBitDoInputReport0x03V1 input0x03_v1;
            EightBitDoInputReport0x03V2 input0x03_v2;
            EightBitDoInputReport0x03V3 input0x03_v3;
        };
    } PACKED;

    class EightBitDoController final : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = {
                {0x05a0, 0x3232}, // 8BitDo Zero
                {0x2dc8, 0x2100}, // 8BitDo SN30 Pro for Xbox Cloud Gaming
                {0x2dc8, 0x2101}, // 8BitDo SN30 Pro for Xbox Cloud Gaming (fw 2.00)
                {0x2dc8, 0x3012}  // 8BitDo Ultimate 2.4g Wireless
            };

            EightBitDoController(const bluetooth::Address *address, HardwareID id)
            : EmulatedSwitchController(address, id) { 
                if ((id.vid == hardware_ids[0].vid) && (id.pid == hardware_ids[0].pid))
                    m_controller_type = EightBitDoControllerType_Zero;
                else if ((id.vid == hardware_ids[1].vid) && (id.pid == hardware_ids[1].pid))
                    m_controller_type = EightBitDoControllerType_Sn30ProXboxCloud;
                else if ((id.vid == hardware_ids[2].vid) && (id.pid == hardware_ids[2].pid))
                    m_controller_type = EightBitDoControllerType_Sn30ProXboxCloudFwV2;
                else if ((id.vid == hardware_ids[3].vid) && (id.pid == hardware_ids[3].pid))
                    m_controller_type = EightBitDoControllerType_Ultimate24gWireless;
                else
                    m_controller_type = EightBitDoControllerType_Other;
            }

            void ProcessInputData(const bluetooth::HidReport *report) override;

        private:
            void MapInputReport0x01(const EightBitDoReportData *src);
            void MapInputReport0x03(const EightBitDoReportData *src, EightBitDoReportFormat fmt);

            EightBitDoControllerType m_controller_type;
    };

}
