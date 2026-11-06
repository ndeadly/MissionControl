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
#include <array>
#include <cstddef>
#include <cstdint>

namespace ams::ble {
    // Coordinator-owned. An idle link loss waits for the other players. A failed
    // pairing may recover the shared stack even with live links. Only the failed
    // address reconnecting rearms that recovery, so another pad cannot cause a
    // reset loop by successfully reconnecting while the problem pad keeps failing.
    class RadioRecovery {
        using Address = std::array<std::uint8_t, 6>;
        bool m_pending = false;
        bool m_attempted = false;
        bool m_have_target = false;
        Address m_target = {};
        bool m_waiting_for_clients = false;
        std::uint32_t m_previous_generation = 0;
        public:
            void Request() { m_pending = true; }
            void PairingFailed(const Address &address) {
                if (!m_have_target) {
                    m_target = address;
                    m_have_target = true;
                }
                m_pending = true;
            }
            void Connected(const Address &address) {
                if (m_have_target && m_target != address) return;
                m_pending = false;
                m_attempted = false;
                m_have_target = false;
            }
            bool Begin(bool enabled, bool pairing, bool awake, size_t active_links) {
                if (!enabled || !pairing || !awake || (active_links && !m_have_target) || !m_pending || m_attempted || m_waiting_for_clients) return false;
                m_pending = false;
                m_attempted = true;
                return true;
            }
            // Only before the reset IPC: sleep or leaving the pairing screen
            // during teardown defers the attempt instead of consuming it.
            void Defer() { m_pending = true; m_attempted = false; }
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
