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
#include <stratosphere.hpp>

#include "switch_hid_reports.hpp"
#include "switch_hid_commands.hpp"
#include "switch_ext_grip_commands.hpp"

#include "switch_power_info.hpp"
#include "switch_button_status.hpp"
#include "switch_analog_stick.hpp"
#include "switch_motor_status.hpp"

#include "switch_sixaxis_types.hpp"
#include "switch_mcu_types.hpp"
#include "switch_attachment_types.hpp"

namespace ams::controller {

     enum SwitchPlayerNumber : u8 {
        SwitchPlayerNumber_One,
        SwitchPlayerNumber_Two,
        SwitchPlayerNumber_Three,
        SwitchPlayerNumber_Four,
        SwitchPlayerNumber_Five,
        SwitchPlayerNumber_Six,
        SwitchPlayerNumber_Seven,
        SwitchPlayerNumber_Eight,
        SwitchPlayerNumber_Unknown = 0xf
    };

    struct HardwareID {
        u16 vid;
        u16 pid;
    };

    struct RGBColour {
        u8 r;
        u8 g;
        u8 b;
    } PACKED;

    struct SwitchControllerColours {
        RGBColour body;
        RGBColour buttons;
        RGBColour left_grip;
        RGBColour right_grip;
    } PACKED;

}
