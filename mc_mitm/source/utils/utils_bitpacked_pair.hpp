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

namespace ams::utils {

    struct BitPackedPair {
        std::array<u8, 3> packed;

        constexpr void SetValues(u16 v1, u16 v2) {
            packed[0] = v1 & 0xFF;
            packed[1] = ((v1 >> 8) & 0x0F) | ((v2 & 0x0F) << 4);
            packed[2] = (v2 >> 4) & 0xFF;
        }

        constexpr void SetFirstValue(u16 v) {
            packed[0] = v & 0xFF;
            packed[1] = (packed[1] & 0xF0) | ((v >> 8) & 0x0F);
        }

        constexpr void SetSecondValue(u16 v) {
            packed[1] = (packed[1] & 0x0F) | ((v & 0x0F) << 4);
            packed[2] = (v >> 4) & 0xFF;
        }

        constexpr std::pair<u16, u16> GetValues() const {
            return { GetFirstValue(), GetSecondValue() };
        }

        constexpr u16 GetFirstValue() const {
            return packed[0] | ((packed[1] & 0x0F) << 8);
        }

        constexpr u16 GetSecondValue() const {
            return (packed[1] >> 4) | (packed[2] << 4);
        }
    };

}
