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

    struct DualsenseInputReport0x01 {
        DualsenseStickData     left_stick;
        DualsenseStickData     right_stick;
        DualsenseButtonData    buttons;
        uint8_t                 left_trigger;
        uint8_t                 right_trigger;
    } __attribute__((packed));

    struct DualsenseReportData {
        uint8_t id;
        union {
            DualsenseInputReport0x01 input0x01;
        };
    } __attribute__((packed));

    class DualsenseController : public EmulatedSwitchController {

        public:
            static constexpr const HardwareID hardware_ids[] = { 
                {0x054c, 0x0ce6}    // Sony Dualsense Controller
            };  

            DualsenseController(const bluetooth::Address *address) 
                : EmulatedSwitchController(address) { };

            //Result Initialize(void);

            void UpdateControllerState(const bluetooth::HidReport *report);

        private:
            void HandleInputReport0x01(const DualsenseReportData *src);
            void MapButtons(const DualsenseButtonData *buttons);

    };
}
