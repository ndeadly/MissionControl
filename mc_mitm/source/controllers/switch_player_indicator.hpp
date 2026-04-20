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

    using SwitchPlayerIndicatorType = util::BitPack8;

    class SwitchPlayerIndicator {
        public:
            enum class PlayerNumber: u8 {
                Invalid,
                One,
                Two,
                Three,
                Four,
                Five,
                Six,
                Seven,
                Eight
            };

        private:
            using LedPattern      = util::BitPack8::Field<0, 4, u8>;
            using LedFlashPattern = util::BitPack8::Field<4, 4, u8>;

            static constexpr PlayerNumber LedPlayerLookup[] = {
                PlayerNumber::Invalid, // 0000
                PlayerNumber::One,     // 0001
                PlayerNumber::Invalid, // 0010
                PlayerNumber::Two,     // 0011
                PlayerNumber::Invalid, // 0100
                PlayerNumber::Six,     // 0101
                PlayerNumber::Eight,   // 0110
                PlayerNumber::Three,   // 0111
                PlayerNumber::One,     // 1000
                PlayerNumber::Five,    // 1001
                PlayerNumber::Six,     // 1010
                PlayerNumber::Seven,   // 1011
                PlayerNumber::Two,     // 1100
                PlayerNumber::Seven,   // 1101
                PlayerNumber::Three,   // 1110
                PlayerNumber::Four,    // 1111
            };

        private:
            SwitchPlayerIndicatorType m_player_indicator;

        public:
            constexpr SwitchPlayerIndicator() : m_player_indicator{0} { }

            constexpr SwitchPlayerIndicator(SwitchPlayerIndicatorType player_indicator) : m_player_indicator(player_indicator) { }

        public:
            constexpr void SetState(SwitchPlayerIndicatorType player_indicator) {
                m_player_indicator = player_indicator;
            }

            constexpr SwitchPlayerIndicatorType GetState() const {
                return m_player_indicator;
            }

        public:
            constexpr void SetLedPattern(u8 pattern) {
                m_player_indicator.Set<LedPattern>(pattern & 0xF);
            }

            constexpr void SetLedFlashPattern(u8 pattern) {
                m_player_indicator.Set<LedFlashPattern>(pattern & 0xF);
            }

            constexpr PlayerNumber GetPlayerNumber() const {
                return LedPlayerLookup[GetLedPattern()];
            }

            constexpr u8 GetLedPattern() const {
                return m_player_indicator.Get<LedPattern>();
            }

            constexpr u8 GetLedFlashPattern() const {
                return m_player_indicator.Get<LedFlashPattern>();
            }
    };

}
