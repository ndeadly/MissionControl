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
#include "ble_log.hpp"
#include <cstdarg>
#include <cstdio>

namespace ams::ble::log {

    namespace {

        constexpr const char LogFilePath[] = "sdmc:/config/MissionControl/ble.log";
        constexpr const char PreviousLogFilePath[] = "sdmc:/config/MissionControl/ble.log.prev";
        constexpr size_t LineBufferSize = 0x200;

        constinit os::SdkMutex g_lock;
        constinit char g_line_buffer[LineBufferSize];
        constinit bool g_enabled = false;

        void AppendToFile(const char *data, size_t size) {
            fs::FileHandle file;
            if (R_FAILED(fs::OpenFile(std::addressof(file), LogFilePath, fs::OpenMode_Write | fs::OpenMode_AllowAppend))) {
                return;
            }
            ON_SCOPE_EXIT { fs::CloseFile(file); };

            s64 offset = 0;
            if (R_FAILED(fs::GetFileSize(std::addressof(offset), file))) {
                return;
            }

            static_cast<void>(fs::WriteFile(file, offset, data, size, fs::WriteOption::Flush));
        }

        void WriteLine(size_t length) {
            if (length >= LineBufferSize - 1) {
                length = LineBufferSize - 2;
            }
            g_line_buffer[length++] = '\n';
            AppendToFile(g_line_buffer, length);
        }

        size_t WriteTimestamp() {
            auto ms = os::GetSystemTick().ToTimeSpan().GetMilliSeconds();
            int n = std::snprintf(g_line_buffer, LineBufferSize, "[%6d.%03d] ", static_cast<int>(ms / 1000), static_cast<int>(ms % 1000));
            return n > 0 ? static_cast<size_t>(n) : 0;
        }

    }

    void Initialize() {
        std::scoped_lock lk(g_lock);

        // These may already exist, failures are fine
        static_cast<void>(fs::CreateDirectory("sdmc:/config"));
        static_cast<void>(fs::CreateDirectory("sdmc:/config/MissionControl"));

        // Start each boot with a fresh log, keeping the previous boot's for a crash to be looked into
        static_cast<void>(fs::DeleteFile(PreviousLogFilePath));
        static_cast<void>(fs::RenameFile(LogFilePath, PreviousLogFilePath));
        if (R_FAILED(fs::CreateFile(LogFilePath, 0))) {
            return;
        }

        g_enabled = true;
    }

    bool IsEnabled() {
        return g_enabled;
    }

    void Write(const char *fmt, ...) {
        if (!g_enabled) {
            return;
        }

        std::scoped_lock lk(g_lock);

        size_t n = WriteTimestamp();

        va_list args;
        va_start(args, fmt);
        int m = std::vsnprintf(g_line_buffer + n, LineBufferSize - n - 1, fmt, args);
        va_end(args);

        if (m > 0) {
            n += static_cast<size_t>(m);
        }

        WriteLine(n);
    }

}
