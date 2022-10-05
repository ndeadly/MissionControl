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
#include "utils.hpp"

namespace ams::utils {

    bool BluetoothAddressCompare(const bluetooth::Address *addr1, const bluetooth::Address *addr2) {
        return std::memcmp(addr1, addr2, sizeof(bluetooth::Address)) == 0;
    }

    Result BluetoothAddressToString(const bluetooth::Address *address, char *out, size_t out_size) {
        if (out_size < 2*sizeof(bluetooth::Address) + 1)
            return -1;

        char ch;
        for (uint32_t i = 0; i < sizeof(bluetooth::Address); ++i) {
            ch = address->address[i] >> 4;
            *out++ = ch + (ch <= 9 ? '0' : 'a' - 0xa);
            ch = address->address[i] & 0x0f;
            *out++ = ch + (ch <= 9 ? '0' : 'a' - 0xa);
        }
        *out = '\0';

        return ams::ResultSuccess();
    }

}
