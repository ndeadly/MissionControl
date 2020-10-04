/*
 * Copyright (C) 2020 ndeadly
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "bluetoothmitm_utils.hpp"

namespace ams::mitm::utils {

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

}
