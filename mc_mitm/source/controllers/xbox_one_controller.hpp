/*
 * Copyright (c) 2020-2022 ndeadly
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

    struct XboxOneStickData {
        uint16_t x;
        uint16_t y;
    } __attribute__ ((__packed__));

    // Used on older firmware
    struct XboxOneButtonDataOld {
        uint8_t      dpad;

        uint8_t A            : 1;
        uint8_t B            : 1;
        uint8_t X            : 1;
        uint8_t Y            : 1;
        uint8_t LB           : 1;
        uint8_t RB           : 1;
        uint8_t view         : 1;
        uint8_t menu         : 1;

        uint8_t lstick_press : 1;
        uint8_t rstick_press : 1;
        uint8_t              : 0;
    } __attribute__ ((__packed__));

    // Used on latest firmwares
    struct XboxOneButtonData {
        uint8_t      dpad;

        uint8_t A            : 1;
        uint8_t B            : 1;
        uint8_t              : 1;
        uint8_t X            : 1;
        uint8_t Y            : 1;
        uint8_t              : 1;
        uint8_t LB           : 1;
        uint8_t RB           : 1;

        uint8_t              : 3;
        uint8_t menu         : 1;
        uint8_t guide        : 1;
        uint8_t lstick_press : 1;
        uint8_t rstick_press : 1;
        uint8_t              : 0;

        uint8_t view         : 1;
        uint8_t              : 0;

    } __attribute__ ((__packed__));

    struct XboxOneOutputReport0x03 {
        uint8_t enable;
        uint8_t magnitude_left;
        uint8_t magnitude_right;
        uint8_t magnitude_strong;
        uint8_t magnitude_weak;
        uint8_t pulse_sustain_10ms;
        uint8_t pulse_release_10ms;
        uint8_t loop_count;
    } __attribute__ ((__packed__));

    struct XboxOneInputReport0x01 {
        XboxOneStickData left_stick;
        XboxOneStickData right_stick;
        uint16_t         left_trigger;
        uint16_t         right_trigger;
        union {
            XboxOneButtonData buttons;

            struct {
                XboxOneButtonDataOld buttons;
            } old;
        };
    } __attribute__ ((__packed__));

    struct XboxOneInputReport0x02{
        uint8_t guide   : 1;
        uint8_t         : 0; 
    } __attribute__ ((__packed__));

    struct XboxOneInputReport0x04 {
        uint8_t capacity : 2;
        uint8_t mode     : 2;
        uint8_t charging : 1;
        uint8_t          : 2;
        uint8_t online   : 1;
    } __attribute__ ((__packed__));
 
    struct XboxOneReportData {
        uint8_t id;
        union {
            XboxOneOutputReport0x03 output0x03;
            XboxOneInputReport0x01  input0x01;
            XboxOneInputReport0x02  input0x02;
            XboxOneInputReport0x04  input0x04;
        };
    } __attribute__ ((__packed__));

    class XboxOneController : public EmulatedSwitchController {

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

            bool SupportsSetTsiCommand() { return false; }

            Result SetVibration(const SwitchRumbleData *rumble_data);
            void ProcessInputData(const bluetooth::HidReport *report) override;

        private:
            void MapInputReport0x01(const XboxOneReportData *src, bool new_format);
            void MapInputReport0x02(const XboxOneReportData *src);
            void MapInputReport0x04(const XboxOneReportData *src);

    };

}
