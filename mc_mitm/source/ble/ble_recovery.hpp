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
#include <cstddef>
#include <cstdint>

namespace ams::ble {
    // Coordinator-owned. Never reset a working BLE player's radio. A timed
    // retry budget cannot get stuck permanently on a controller that won't pair.
    class RadioRecovery {
        bool m_pending = false;
        bool m_waiting_for_clients = false;
        std::int64_t m_last_attempt_ms = -60000;
        std::uint32_t m_previous_generation = 0;
        public:
            void Request() { m_pending = true; }
            bool Begin(bool enabled, bool pairing, bool awake, size_t active_links, std::int64_t now_ms) {
                if (!enabled || !pairing || !awake || active_links || !m_pending ||
                    m_waiting_for_clients || now_ms - m_last_attempt_ms < 60000) return false;
                m_pending = false;
                m_last_attempt_ms = now_ms;
                return true;
            }
            void Sleep() { m_pending = false; m_waiting_for_clients = false; }
            void AwaitClients(std::uint32_t previous_generation) {
                m_previous_generation = previous_generation;
                m_waiting_for_clients = true;
            }
            bool WaitingForClients() const { return m_waiting_for_clients; }
            bool AcceptClients(std::uint32_t generation, size_t count) {
                if (m_waiting_for_clients && (generation == m_previous_generation || count == 0)) return false;
                m_waiting_for_clients = false;
                return true;
            }
    };
}
