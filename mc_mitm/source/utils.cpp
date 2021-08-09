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

}
