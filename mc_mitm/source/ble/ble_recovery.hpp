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

namespace ams::ble {
    // Coordinator-owned. One attempt per successful session prevents a failing
    // controller from repeatedly resetting the radio and disconnecting Joy-Cons.
    class RadioRecovery {
        bool m_pending = false;
        bool m_attempted = false;
        public:
            void Request() { m_pending = true; }
            void Connected() { m_attempted = false; }
            bool Begin(bool enabled, bool pairing, bool awake, size_t active_links) {
                if (!enabled || !pairing || !awake || active_links || !m_pending || m_attempted) return false;
                m_pending = false;
                m_attempted = true;
                return true;
            }
    };
}
