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
#include "ble_profile.hpp"
#include <cstdio>
#include <algorithm>

namespace ams::ble::profile {

    namespace {

        constexpr const char ProfileFilePath[] = "sdmc:/config/MissionControl/ble_profile.txt";
        constexpr size_t FlushThreshold = 500; // Flush summary every 500 reports (~5s at 100Hz)
        constexpr size_t BufferSize = 0x800;

        constinit os::SdkMutex g_lock;
        constinit bool g_enabled = false;

        constinit u64 g_total_reports = 0;
        constinit u64 g_window_reports = 0;
        constinit s64 g_last_arrival_tick = 0;

        constinit s64 g_min_delta_us = 0;
        constinit s64 g_max_delta_us = 0;
        constinit s64 g_sum_delta_us = 0;

        constinit s64 g_min_process_us = 0;
        constinit s64 g_max_process_us = 0;
        constinit s64 g_sum_process_us = 0;

        constinit u16 g_conn_interval = 0;
        constinit u16 g_conn_latency = 0;
        constinit u16 g_supervision_tout = 0;

        void WriteSummaryToFile() {
            if (g_window_reports == 0) {
                return;
            }

            char buffer[BufferSize];

            const u32 interval_w = g_conn_interval > 0 ? (static_cast<u32>(g_conn_interval) * 125) / 100 : 0;
            const u32 interval_f = g_conn_interval > 0 ? (static_cast<u32>(g_conn_interval) * 125) % 100 : 0;
            const u32 interval_hz = g_conn_interval > 0 ? (800 / g_conn_interval) : 0;

            const u64 avg_delta_us = g_sum_delta_us / g_window_reports;
            const u32 avg_delta_w = static_cast<u32>(avg_delta_us / 1000);
            const u32 avg_delta_f = static_cast<u32>((avg_delta_us % 1000) / 10);

            const u32 min_delta_w = static_cast<u32>(g_min_delta_us / 1000);
            const u32 min_delta_f = static_cast<u32>((g_min_delta_us % 1000) / 10);

            const u32 max_delta_w = static_cast<u32>(g_max_delta_us / 1000);
            const u32 max_delta_f = static_cast<u32>((g_max_delta_us % 1000) / 10);

            const u32 actual_hz = avg_delta_us > 0 ? static_cast<u32>(1000000 / avg_delta_us) : 0;
            const u32 avg_process_us = static_cast<u32>(g_sum_process_us / g_window_reports);

            int len = std::snprintf(buffer, sizeof(buffer),
                "=== Mission Control BLE Latency & Cadence Profile ===\n"
                "Total Reports Processed : %lu\n"
                "Current Sample Window   : %lu reports\n"
                "\n"
                "Negotiated BLE Link Parameters:\n"
                "  Connection Interval  : %u.%02u ms (%u Hz) [raw: %u]\n"
                "  Slave Latency        : %u (events peripheral may skip)\n"
                "  Supervision Timeout  : %u ms\n"
                "\n"
                "Packet Arrival Cadence (Radio Link Timing):\n"
                "  Min Interval : %u.%02u ms\n"
                "  Avg Interval : %u.%02u ms (%u reports/sec)\n"
                "  Max Interval : %u.%02u ms\n"
                "\n"
                "mc.mitm Input Forwarding Latency (Parse -> Format -> Hid):\n"
                "  Min Processing Time : %ld us\n"
                "  Avg Processing Time : %u us\n"
                "  Max Processing Time : %ld us\n"
                "=====================================================\n\n",
                g_total_reports, g_window_reports,
                interval_w, interval_f, interval_hz, g_conn_interval,
                g_conn_latency,
                static_cast<u32>(g_supervision_tout * 10),
                min_delta_w, min_delta_f,
                avg_delta_w, avg_delta_f, actual_hz,
                max_delta_w, max_delta_f,
                g_min_process_us, avg_process_us, g_max_process_us
            );

            if (len <= 0) {
                return;
            }

            fs::FileHandle file;
            if (R_FAILED(fs::OpenFile(std::addressof(file), ProfileFilePath, fs::OpenMode_Write))) {
                return;
            }
            ON_SCOPE_EXIT { fs::CloseFile(file); };

            static_cast<void>(fs::WriteFile(file, 0, buffer, std::min<size_t>(len, sizeof(buffer)), fs::WriteOption::Flush));
        }

    }

    void Initialize() {
        std::scoped_lock lk(g_lock);

        static_cast<void>(fs::CreateDirectory("sdmc:/config"));
        static_cast<void>(fs::CreateDirectory("sdmc:/config/MissionControl"));

        static_cast<void>(fs::DeleteFile(ProfileFilePath));
        static_cast<void>(fs::CreateFile(ProfileFilePath, 0));

        Reset();
        g_enabled = true;
    }

    bool IsEnabled() {
        return g_enabled;
    }

    void RecordConnectionUpdate(u16 conn_interval, u16 conn_latency, u16 supervision_tout) {
        if (!g_enabled) {
            return;
        }

        std::scoped_lock lk(g_lock);
        g_conn_interval = conn_interval;
        g_conn_latency = conn_latency;
        g_supervision_tout = supervision_tout;
    }

    u64 RecordReportStart() {
        if (!g_enabled) {
            return 0;
        }

        const s64 now_tick = os::GetSystemTick().GetInt64Value();

        std::scoped_lock lk(g_lock);
        if (g_last_arrival_tick > 0) {
            const s64 delta_us = os::Tick(now_tick - g_last_arrival_tick).ToTimeSpan().GetMicroSeconds();
            if (g_window_reports == 0) {
                g_min_delta_us = delta_us;
                g_max_delta_us = delta_us;
                g_sum_delta_us = delta_us;
            } else {
                g_min_delta_us = std::min(g_min_delta_us, delta_us);
                g_max_delta_us = std::max(g_max_delta_us, delta_us);
                g_sum_delta_us += delta_us;
            }
        }
        g_last_arrival_tick = now_tick;

        return static_cast<u64>(now_tick);
    }

    void RecordReportEnd(u64 start_tick) {
        if (!g_enabled || (start_tick == 0)) {
            return;
        }

        const s64 now_tick = os::GetSystemTick().GetInt64Value();
        const s64 process_us = os::Tick(now_tick - start_tick).ToTimeSpan().GetMicroSeconds();

        std::scoped_lock lk(g_lock);
        if (g_window_reports == 0) {
            g_min_process_us = process_us;
            g_max_process_us = process_us;
            g_sum_process_us = process_us;
        } else {
            g_min_process_us = std::min(g_min_process_us, process_us);
            g_max_process_us = std::max(g_max_process_us, process_us);
            g_sum_process_us += process_us;
        }

        ++g_window_reports;
        ++g_total_reports;

        if (g_window_reports >= FlushThreshold) {
            WriteSummaryToFile();
            g_window_reports = 0;
            g_sum_delta_us = 0;
            g_min_delta_us = 0;
            g_max_delta_us = 0;
            g_sum_process_us = 0;
            g_min_process_us = 0;
            g_max_process_us = 0;
        }
    }

    void FlushIfDue() {
        if (!g_enabled) {
            return;
        }
        std::scoped_lock lk(g_lock);
        if (g_window_reports >= FlushThreshold) {
            WriteSummaryToFile();
        }
    }

    void FlushNow() {
        if (!g_enabled) {
            return;
        }
        std::scoped_lock lk(g_lock);
        WriteSummaryToFile();
    }

    void Reset() {
        g_total_reports = 0;
        g_window_reports = 0;
        g_last_arrival_tick = 0;
        g_min_delta_us = 0;
        g_max_delta_us = 0;
        g_sum_delta_us = 0;
        g_min_process_us = 0;
        g_max_process_us = 0;
        g_sum_process_us = 0;
    }

}
