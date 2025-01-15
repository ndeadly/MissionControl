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
#include <switch.h>

namespace ams::controller {

    constexpr u8 convert_battery_100(u8 level) {
        return level ? (((level - 1) / 25) + 1) << 1 : 0;
    }

    constexpr u8 convert_battery_255(u8 level) {
        return level ? ((level / 64) + 1) << 1 : 0;
    }

}
