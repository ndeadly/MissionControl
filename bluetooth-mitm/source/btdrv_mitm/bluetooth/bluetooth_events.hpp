/*
 * Copyright (c) 2020 ndeadly
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
