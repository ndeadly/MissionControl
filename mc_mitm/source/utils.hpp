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
#include <stratosphere.hpp>
#include "bluetooth_mitm/bluetooth/bluetooth_types.hpp"

namespace ams::utils {

    s32 ConvertToHorizonPriority(s32 user_priority);
    s32 ConvertToUserPriority(s32 horizon_priority);

    Result BluetoothAddressToString(const bluetooth::Address *address, char *out, size_t out_size);

}
