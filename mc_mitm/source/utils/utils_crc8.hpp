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
#include <stratosphere.hpp>

namespace ams::utils {

    template <u8 Polynomial>
    class Crc8 {
        public:
            static constexpr u8 Calculate(const void *data, size_t size, u8 seed = 0x00) {
                auto *bytes = reinterpret_cast<const u8 *>(data);

                u8 crc = seed;
                for (size_t i = 0; i < size; ++i) {
                    crc = CrcLookup[crc ^ bytes[i]];
                }
                return crc;
            }

        private:
            static constexpr std::array<u8, UINT8_MAX> CrcLookup = []() {
                std::array<u8, UINT8_MAX> table {};

                for (size_t i = 0; i < UINT8_MAX; ++i) {
                    u8 crc = i;
                    for (int b = 8; b > 0; --b) {
                        if (crc & BIT(7)) {
                            crc = (crc << 1) ^ Polynomial;
                        } else {
                            crc <<= 1;
                        }
                    }
                    
                    table[i] = crc;
                }
                return table;
            }();
    };

}
