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
#include <switch.h>
#include <stratosphere.hpp>

namespace ams::ble::profile {

    void Initialize();
    bool IsEnabled();

    // Called on BtdrvBleEventType_ConnectionUpdate
    void RecordConnectionUpdate(u16 conn_interval, u16 conn_latency, u16 supervision_tout);

    // Fast-path timing hooks (microsecond precision using hardware ticks)
    u64 RecordReportStart();
    void RecordReportEnd(u64 start_tick);

    // Periodic flush to /config/MissionControl/ble_profile.txt (called on background cadence or disconnect)
    void FlushIfDue();
    void FlushNow();
    void Reset();
    void BeginSession();

}
