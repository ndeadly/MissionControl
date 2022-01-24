/*
 * Copyright (c) 2020-2021 ndeadly
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

    namespace {

        constexpr inline s32 TargetThreadPriorityRangeSize = svc::LowestThreadPriority - svc::HighestThreadPriority + 1;
        constexpr inline s32 UserThreadPriorityOffset = 28;
        constexpr inline s32 HighestTargetThreadPriority = 0;
        constexpr inline s32 LowestTargetThreadPriority = TargetThreadPriorityRangeSize - 1;

    }

    s32 ConvertToHorizonPriority(s32 user_priority) {
        const s32 horizon_priority = user_priority + UserThreadPriorityOffset;
        AMS_ASSERT(HighestTargetThreadPriority <= horizon_priority && horizon_priority <= LowestTargetThreadPriority);
        return horizon_priority;
    }

    s32 ConvertToUserPriority(s32 horizon_priority) {
        AMS_ASSERT(HighestTargetThreadPriority <= horizon_priority && horizon_priority <= LowestTargetThreadPriority);
        return horizon_priority - UserThreadPriorityOffset;
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

    void ParseBoolean(const char *value, bool *out) {
        if (strcasecmp(value, "true") == 0)
            *out = true;
        else if (strcasecmp(value, "false") == 0)
            *out = false;
    }

    void ParseUInt32(const char *value, uint32_t *out) {
        *out = atoi(value);
    }

    void ParseBluetoothAddress(const char *value, bluetooth::Address *out) {
        // Check length of address string is correct
        if (std::strlen(value) != 3*sizeof(bluetooth::Address) - 1) return;

        // Parse bluetooth mac address
        char buf[2 + 1];
        bluetooth::Address address = {};
        for (uint32_t i = 0; i < sizeof(bluetooth::Address); ++i) {
            // Convert hex pair to number
            std::memcpy(buf, &value[i*3], 2);
            address.address[i] = static_cast<uint8_t>(std::strtoul(buf, nullptr, 16));

            // Check for colon separator
            if ((i < sizeof(bluetooth::Address) - 1) && (value[i*3 + 2] != ':'))
                return;
        }

        *out = address;
    }

}
