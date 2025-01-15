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

    enum XboxOneDPadDirection {
        XboxOneDPad_Released,
        XboxOneDPad_N,
        XboxOneDPad_NE,
        XboxOneDPad_E,
        XboxOneDPad_SE,
        XboxOneDPad_S,
        XboxOneDPad_SW,
        XboxOneDPad_W,
        XboxOneDPad_NW
    };

    enum XboxOnePowerMode {
        XboxOnePowerMode_USB         = 0,
        XboxOnePowerMode_Battery     = 1,
        XboxOnePowerMode_PlayNCharge = 2
    };

    // Used on older firmware
    struct XboxOneButtonDataOld {
        u8      dpad;

        u8 A            : 1;
        u8 B            : 1;
        u8 X            : 1;
        u8 Y            : 1;
        u8 LB           : 1;
        u8 RB           : 1;
        u8 view         : 1;
        u8 menu         : 1;

        u8 lstick_press : 1;
        u8 rstick_press : 1;
        u8              : 0;
    } PACKED;

    // Used on latest firmwares
    struct XboxOneButtonData {
        u8      dpad;

        u8 A            : 1;
        u8 B            : 1;
        u8              : 1;
        u8 X            : 1;
        u8 Y            : 1;
        u8              : 1;
        u8 LB           : 1;
        u8 RB           : 1;

        u8              : 3;
        u8 menu         : 1;
        u8 guide        : 1;
        u8 lstick_press : 1;
        u8 rstick_press : 1;
        u8              : 0;

        u8 view         : 1;
        u8              : 0;
    } PACKED;

    struct XboxOneOutputReport0x03 {
        u8 enable;
        u8 magnitude_left;
        u8 magnitude_right;
        u8 magnitude_strong;
        u8 magnitude_weak;
        u8 pulse_sustain_10ms;
        u8 pulse_release_10ms;
        u8 loop_count;
    } PACKED;

    struct XboxOneInputReport0x01 {
        AnalogStick<u16> left_stick;
        AnalogStick<u16> right_stick;
        u16 left_trigger;
        u16 right_trigger;
        union {
            XboxOneButtonData buttons;

            struct {
                XboxOneButtonDataOld buttons;
            } old;
        };
    } PACKED;

    struct XboxOneInputReport0x02{
        u8 guide : 1;
        u8       : 0;
    } PACKED;

    struct XboxOneInputReport0x04 {
        u8 capacity : 2;
        u8 mode     : 2;
        u8 charging : 1;
        u8          : 2;
        u8 online   : 1;
    } PACKED;
 
    struct XboxOneReportData {
        u8 id;
        union {
            XboxOneOutputReport0x03 output0x03;
            XboxOneInputReport0x01 input0x01;
            XboxOneInputReport0x02 input0x02;
            XboxOneInputReport0x04 input0x04;
        };
    } PACKED;

    class XboxOneController final : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = {
                {0x045e, 0x02e0},   // Official Xbox One S Controller
                {0x045e, 0x02fd},   // Official Xbox One S Controller
                {0x045e, 0x0b00},   // Official Xbox One Elite 2 Controller
                {0x045e, 0x0b05},   // Official Xbox One Elite 2 Controller
                {0x045e, 0x0b0a}    // Official Xbox Adaptive Controller
            };

            XboxOneController(const bluetooth::Address *address, HardwareID id)
            : EmulatedSwitchController(address, id) { }

            Result SetVibration(const SwitchMotorData *motor_data);
            void ProcessInputData(const bluetooth::HidReport *report) override;

        private:
            void MapInputReport0x01(const XboxOneReportData *src, bool new_format);
            void MapInputReport0x02(const XboxOneReportData *src);
            void MapInputReport0x04(const XboxOneReportData *src);

    };

}
