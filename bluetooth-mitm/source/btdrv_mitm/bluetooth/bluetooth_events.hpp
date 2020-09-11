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
#pragma once
#include <stratosphere.hpp>
#include "bluetooth_core.hpp"
#include "bluetooth_hid.hpp"
#include "bluetooth_ble.hpp"

namespace ams::bluetooth::events {

    enum BtdrvEventType {
        BtdrvEventType_BluetoothCore,
        BtdrvEventType_BluetoothHid,
        BtdrvEventType_BluetoothBle,
    };

    Result Initialize(void);
    void Finalize(void);

}
