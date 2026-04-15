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

    enum class SwitchBatteryLevel : u8 {
        Empty,
        Critical,
        Low,
        Medium,
        Full
    };

    struct SwitchBatteryLevelConverter {

        template <std::integral T>
        static constexpr SwitchBatteryLevel ConvertValue(T value) {
            constexpr T divisor = (std::numeric_limits<T>::max() + 1) >> 2;
            u8 tmp_level = value ? (((value - 1) / divisor) + 1) : 0;
            return static_cast<SwitchBatteryLevel>(tmp_level);
        }

        static constexpr SwitchBatteryLevel ConvertPercentage(u8 percentage) {
            u8 tmp_level = percentage ? (((percentage - 1) / 25) + 1) : 0;
            return static_cast<SwitchBatteryLevel>(tmp_level);
        }

    };

}
