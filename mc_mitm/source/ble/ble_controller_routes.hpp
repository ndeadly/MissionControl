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
#include <cstdint>
#include <memory>
#include <mutex>

namespace ams::ble {

    // Hold a reference across dispatch. A controller-local mutex cannot protect a
    // raw pointer between lookup and entry into HandleInputReport during teardown.
    template<typename Controller, typename Mutex, size_t Capacity>
    class ControllerRoutes {
        struct Entry {
            uint32_t conn_id = UINT32_MAX;
            std::shared_ptr<Controller> controller;
        };
        Mutex m_mutex;
        std::array<Entry, Capacity> m_entries{};

        public:
            void Register(size_t slot, uint32_t conn_id, std::shared_ptr<Controller> controller) {
                std::scoped_lock lk(m_mutex);
                m_entries[slot] = {conn_id, std::move(controller)};
            }

            void Unregister(size_t slot) {
                std::scoped_lock lk(m_mutex);
                m_entries[slot] = {};
            }

            std::shared_ptr<Controller> Find(uint32_t conn_id) {
                if (conn_id == UINT32_MAX) return nullptr;
                std::scoped_lock lk(m_mutex);
                for (const auto &entry : m_entries) {
                    if (entry.conn_id == conn_id) return entry.controller;
                }
                return nullptr;
            }
    };
}
