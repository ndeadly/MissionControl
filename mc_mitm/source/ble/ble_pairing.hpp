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
#include "ble_types.hpp"

namespace ams::ble::pairing {
    Result Connect(u32 slot, u8 client_if, const bluetooth::Address &address, u32 *out_conn_id, TimeSpan timeout);
    Result ClearPeer(u32 slot, const bluetooth::Address &address);
    // A fresh security indication only permits Subscribe to verify authenticated GATT access.
    bool PrepareSecurity(u32 slot, const bluetooth::Address &address);
}
