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
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>

namespace ams::controller::xbox {
    constexpr uint16_t StickCenter       = 0x8000;
    constexpr int32_t MaxStickRadius    = 0x7fff;
    constexpr int32_t HosInnerDeadzone  = 0x1000; // Matches 12.5% (0x100/0x800) inner deadzone in DefaultAnalogStickModelValues

    inline std::pair<uint16_t, uint16_t> ApplyRadialDeadzone(uint16_t raw_x, uint16_t raw_y, int percent) {
        percent = std::clamp(percent, 0, 50);
        if (percent == 0) return {raw_x, raw_y};
        const uint32_t deadzone = MaxStickRadius * percent / 100;
        const int32_t dx = static_cast<int32_t>(raw_x) - static_cast<int32_t>(StickCenter);
        const int32_t dy = static_cast<int32_t>(raw_y) - static_cast<int32_t>(StickCenter);
        const uint64_t dist_sq = static_cast<uint64_t>(static_cast<int64_t>(dx) * dx + static_cast<int64_t>(dy) * dy);
        const uint64_t deadzone_sq = static_cast<uint64_t>(deadzone) * deadzone;

        if (dist_sq <= deadzone_sq) {
            return { StickCenter, StickCenter };
        }

        if (dist_sq >= static_cast<uint64_t>(MaxStickRadius) * MaxStickRadius) {
            return { raw_x, raw_y };
        }

        const float dist = std::sqrt(static_cast<float>(dist_sq));
        const float norm = (dist - static_cast<float>(deadzone)) / static_cast<float>(MaxStickRadius - deadzone);
        const float target_dist = static_cast<float>(HosInnerDeadzone) + norm * static_cast<float>(MaxStickRadius - HosInnerDeadzone);
        const float scale = target_dist / dist;

        const int32_t out_x = std::clamp<int32_t>(static_cast<int32_t>(StickCenter) + static_cast<int32_t>(std::lround(dx * scale)), 0, 0xffff);
        const int32_t out_y = std::clamp<int32_t>(static_cast<int32_t>(StickCenter) + static_cast<int32_t>(std::lround(dy * scale)), 0, 0xffff);
        return { static_cast<uint16_t>(out_x), static_cast<uint16_t>(out_y) };
    }

}
