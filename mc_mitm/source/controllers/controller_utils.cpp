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
#include "controller_utils.hpp"

namespace ams::controller {

    uint8_t convert_battery_100(uint8_t level) {
        return level ? (((level - 1) / 25) + 1) << 1 : 0;
    }

    uint8_t convert_battery_255(uint8_t level) {
        return level ? ((level / 64) + 1) << 1 : 0;
    }

}
