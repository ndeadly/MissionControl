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

    enum DualsenseDPadDirection {
        DualsenseDPad_N,
        DualsenseDPad_NE,
        DualsenseDPad_E,
        DualsenseDPad_SE,
        DualsenseDPad_S,
        DualsenseDPad_SW,
        DualsenseDPad_W,
        DualsenseDPad_NW,
        DualsenseDPad_Released
    };

    struct DualsenseStickData {
        uint8_t x;
        uint8_t y;
    } __attribute__((packed));

    struct DualsenseButtonData {
        uint8_t dpad       : 4;
        uint8_t square     : 1;
        uint8_t cross      : 1;
        uint8_t circle     : 1;
        uint8_t triangle   : 1;

        uint8_t L1         : 1;
        uint8_t R1         : 1;
        uint8_t L2         : 1;
        uint8_t R2         : 1;
        uint8_t share      : 1;
        uint8_t options    : 1;
        uint8_t L3         : 1;
        uint8_t R3         : 1;
        
        uint8_t ps         : 1;
        uint8_t tpad       : 1;
        uint8_t counter    : 6;
    } __attribute__((packed));

    struct DualsenseOutputReport0x31  {
        struct {
            uint8_t data[75];
        };
        uint32_t crc;
    } __attribute__((packed));

    struct DualsenseInputReport0x01 {
        DualsenseStickData      left_stick;
        DualsenseStickData      right_stick;
        DualsenseButtonData     buttons;
        uint8_t                 left_trigger;
        uint8_t                 right_trigger;
    } __attribute__((packed));

    struct DualsenseInputReport0x31 {
        uint8_t                 _unk0;
        DualsenseStickData      left_stick;
        DualsenseStickData      right_stick;
        uint8_t                 left_trigger;
        uint8_t                 right_trigger;
        uint8_t                 counter;
        DualsenseButtonData     buttons;
        uint8_t                 _unk1[5];
        uint16_t                vel_x;
        uint16_t                vel_y;
        uint16_t                vel_z;
        uint16_t                acc_x;
        uint16_t                acc_y;
        uint16_t                acc_z;
        uint8_t                 _unk2[25];

        uint8_t battery_level    : 4;
        uint8_t usb              : 1;
        uint8_t full             : 1;
        uint8_t                  : 0;
    } __attribute__((packed));

    struct DualsenseReportData {
        uint8_t id;
        union {
            DualsenseInputReport0x01 input0x01;
            DualsenseInputReport0x31 input0x31;
        };
    } __attribute__((packed));

    class DualsenseController : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = { 
                {0x054c, 0x0ce6}    // Sony Dualsense Controller
            };  

            DualsenseController(const bluetooth::Address *address) 
                : EmulatedSwitchController(address), m_rumble_high(0), m_rumble_low(0), m_led_flags(0), m_led_colour({0, 0, 0}) { };

            Result Initialize(void);
            Result SetPlayerLed(uint8_t led_mask);
            Result SetLightbarColour(RGBColour colour);

            void UpdateControllerState(const bluetooth::HidReport *report);

            Result HandleRumbleReport(const bluetooth::HidReport *report);

        private:
            void HandleInputReport0x01(const DualsenseReportData *src);
            void HandleInputReport0x31(const DualsenseReportData *src);

            void MapButtons(const DualsenseButtonData *buttons);

            Result PushRumbleLedState(void);

            uint8_t m_rumble_high;
            uint8_t m_rumble_low;

            uint8_t m_led_flags;
            RGBColour m_led_colour;
    };
}
