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

namespace ams::controller {

    struct SwitchButtonStatusType {
        u8 Y            : 1;
        u8 X            : 1;
        u8 B            : 1;
        u8 A            : 1;
        u8 SR_right     : 1;
        u8 SL_right     : 1;
        u8 R            : 1;
        u8 ZR           : 1;

        u8 minus        : 1;
        u8 plus         : 1;
        u8 rstick_press : 1;
        u8 lstick_press : 1;
        u8 home         : 1;
        u8 capture      : 1;
        u8              : 1;
        u8 wired        : 1;

        u8 dpad_down    : 1;
        u8 dpad_up      : 1;
        u8 dpad_right   : 1;
        u8 dpad_left    : 1;
        u8 SR_left      : 1;
        u8 SL_left      : 1;
        u8 L            : 1;
        u8 ZL           : 1;
    } PACKED;

}
