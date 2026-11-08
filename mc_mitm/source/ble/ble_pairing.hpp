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
    // Only successful subscriptions in the current native stack qualify for
    // reconnect. A discovered address or an old GATT cache is not a bond.
    class SessionHistory {
        struct Peer {
            bluetooth::Address address = {};
            u32 generation = 0;
            bool valid = false;
        };
        Peer m_peers[MaxBleControllers] = {};
        public:
            bool Contains(const bluetooth::Address &address, u32 generation) const {
                for (const auto &peer : m_peers) {
                    if (peer.valid && peer.generation == generation && AddressEqual(peer.address, address)) return true;
                }
                return false;
            }
            void Remember(u32 slot, const bluetooth::Address &address, u32 generation) {
                if (slot < MaxBleControllers) m_peers[slot] = {address, generation, true};
            }
    };

    Result Connect(u32 slot, u8 client_if, const bluetooth::Address &address, u32 *out_conn_id, TimeSpan timeout, bool reconnect);
    // A fresh security indication only permits Subscribe to verify authenticated GATT access.
    bool PrepareSecurity(u32 slot, const bluetooth::Address &address);
}
