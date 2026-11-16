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

    enum MadCatzInputReport0x01Format {
        MadCatzInputReport0x01Format_FirmwareStock,
        MadCatzInputReport0x01Format_Firmware125,
    };

    struct MadCatzInputReport0x01 {
        struct {
            u8 X      : 1;
            u8 A      : 1;
            u8 B      : 1;
            u8 Y      : 1;
            u8 LB     : 1;
            u8 RB     : 1;
            u8 LT     : 1; // Valid only in Desktop mode.
            u8 RT     : 1; // Valid only in Desktop mode.

            u8 back   : 1; // Valid only in Desktop mode.
            u8 start  : 1;
            u8 L3     : 1;
            u8 R3     : 1;
            u8 emblem : 1; // Valid only in Desktop mode. Instantaneous.
            u8        : 0;
        } buttons;
        DirectionalPadType<1, 0> dpad;
        struct {
            union {
                struct {
                    AnalogStick<u16> left_stick;
                    AnalogStick<u16> right_stick;
                    AnalogTriggerType<u16> left_trigger;
                    AnalogTriggerType<u16> right_trigger;
                } fw_stock;
                struct {
                    AnalogStick<u8> left_stick;
                    AnalogStick<u8> right_stick;
                    AnalogTriggerType<u8> left_trigger;
                    AnalogTriggerType<u8> right_trigger;
                } fw_125;
            };
        } analogs;
    } PACKED;

    struct MadCatzInputReport0x02 {
        u8 ff_tap      : 1; // Instantaneous.
        u8 rewind_tap  : 1; // Instantaneous.
        u8 volume_up   : 1;
        u8 volume_down : 1;
        u8 play_pause  : 1;
        u8 ff_hold     : 1;
        u8 rew_hold    : 1;
        u8 system_back : 1; // Valid only in Smart mode. Instantaneous.

        u8 menu_up     : 1; // Valid only in Mouse mode. D-pad acts as a media remote's d-pad.
        u8 menu_down   : 1; //  "
        u8 menu_left   : 1; //  "
        u8 menu_right  : 1; //  "
        u8 system_home : 1; // Valid only in Smart mode. Instantaneous.
        u8             : 0;
    } PACKED;

    struct MadCatzInputReport0x81 {
        struct {
            union {
                struct {
                    u8 A      : 1;
                    u8 B      : 1;
                    u8 X      : 1;
                    u8 Y      : 1;
                    u8 L1     : 1;
                    u8 R1     : 1;
                    u8 select : 1;
                    u8 start  : 1;

                    u8 L3     : 1;
                    u8 R3     : 1;
                    u8        : 0;
                };

                struct {
                    u8 A     : 1;
                    u8 B     : 1;
                    u8       : 1;
                    u8 X     : 1;
                    u8 Y     : 1;
                    u8       : 1;
                    u8 L1    : 1;
                    u8 R1    : 1;

                    u8       : 3;
                    u8 start : 1;
                    u8       : 1;
                    u8 L3    : 1;
                    u8 R3    : 1;
                    u8       : 0;
                } xinput;
            };
        } buttons;
        DirectionalPadType<1, 0> dpad;
        AnalogStick<u8> left_stick;
        AnalogStick<u8> right_stick;
        AnalogTriggerType<u8> left_trigger;
        AnalogTriggerType<u8> right_trigger;
        u8 reserved;
    } PACKED;

    struct MadCatzInputReport0x82 {
        struct {
            u8            : 2;
            u8 R1         : 1;
            u8 L1         : 1;
            u8 Y          : 1;
            u8 B          : 1;
            u8 X          : 1;
            u8 select     : 1;

            u8 dpad_up    : 1;
            u8 dpad_down  : 1;
            u8 dpad_left  : 1;
            u8 dpad_right : 1;
            u8            : 4;
        } buttons;
        u8 reserved;
    } PACKED;

    struct MadCatzInputReport0x83 {
        struct {
            u8 R2 : 1;
            u8 L2 : 1;
            u8 R3 : 1;
            u8 L3 : 1;
            u8    : 0;
        } buttons;
        AnalogStick<u8> left_stick;
        u8 reserved[2];
    } PACKED;

    struct MadCatzReportData {
        u8 id;
        union {
            MadCatzInputReport0x01              input0x01;
            MadCatzInputReport0x02              input0x02;

            MadCatzInputReport0x81              input0x81;
            MadCatzInputReport0x82              input0x82;
            MadCatzInputReport0x83              input0x83;
        };
    } PACKED;

    class MadCatzController final : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = {
                {0x0738, 0x5266},   // Mad Catz C.T.R.L. R
                {0x0738, 0x5262},   // Mad Catz Micro C.T.R.L. R

                {0x0738, 0x5250},   // Mad Catz C.T.R.L. R for Samsung
                {0x0738, 0x5269}    // Mad Catz L.Y.N.X. 3
            };

            MadCatzController(bluetooth::Address address, HardwareID id) : EmulatedSwitchController(address, id) { }

            virtual void ParseInputReport(const u8 *report_buffer, size_t size) override;

        private:
            void MapInputReport0x01(const MadCatzReportData *src, MadCatzInputReport0x01Format fmt);
            void MapInputReport0x02(const MadCatzReportData *src);

            void MapInputReport0x81(const MadCatzReportData *src);
            void MapInputReport0x82(const MadCatzReportData *src);
            void MapInputReport0x83(const MadCatzReportData *src);
    };

}
