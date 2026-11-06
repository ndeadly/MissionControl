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
#include "ble_sleep.hpp"
#include "ble_events.hpp"
#include "ble_log.hpp"

namespace ams::ble {

    void PrepareForSleep() {
        // Stop coordinator work and retain event-slot ownership until every
        // disconnect is sent. Never consume the coordinator's event queue here.
        const bool owns_radio = events::BeginSleep();
        ON_SCOPE_EXIT { events::EndSleep(); };
        if (!owns_radio) return;

        static_cast<void>(btdrvStopBleScan());
        // Include a late connection queued for adoption but not assigned a slot.
        constexpr u32 SleepLinkCount = MaxBleControllers + 1;
        bool cancelled[SleepLinkCount] = {};
        u32 requested[SleepLinkCount];
        std::fill(std::begin(requested), std::end(requested), InvalidConnId);
        const auto deadline = os::GetSystemTick().ToTimeSpan() + TimeSpan::FromSeconds(1);
        for (;;) {
            bool pending = false;
            for (u32 slot = 0; slot < SleepLinkCount; ++slot) {
                const auto link = events::GetSleepLink(slot);
                if (!link.active) continue;
                pending = true;
                if (!cancelled[slot]) {
                    // A pending direct connection can complete during sleep
                    // preparation. Cancel it and keep checking for a late ID.
                    if (link.client_if != InvalidClientIf) {
                        const Result rc = btdrvCancelConnectGattServer(link.client_if, link.address, true);
                        log::Write("sleep: cancel slot=%u rc=0x%x", slot, rc.GetValue());
                    }
                    cancelled[slot] = true;
                }
                if (link.conn_id != InvalidConnId && requested[slot] != link.conn_id) {
                    const Result rc = btdrvDisconnectGattServer(link.conn_id);
                    log::Write("sleep: disconnect slot=%u conn_id=%u rc=0x%x", slot, link.conn_id, rc.GetValue());
                    requested[slot] = link.conn_id;
                }
            }
            if (!pending) {
                log::Write("sleep: all BLE links down before PSC acknowledgement");
                return;
            }
            if (os::GetSystemTick().ToTimeSpan() >= deadline) {
                log::Write("sleep: BLE disconnect acknowledgement timed out");
                return;
            }
            os::SleepThread(TimeSpan::FromMilliSeconds(20));
        }
    }
}
