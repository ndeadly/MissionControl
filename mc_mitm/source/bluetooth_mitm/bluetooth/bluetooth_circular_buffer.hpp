/*
 * Copyright (c) 2020-2025 ndeadly
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

#include "bluetooth_types.hpp"

namespace ams::bluetooth {

    struct CircularBufferPacketHeader {
        u8 type;
        os::Tick timestamp;
        u64 size;
    };

    struct CircularBufferPacket {
        CircularBufferPacketHeader header;
        HidReportEventInfo data;
    };

    class CircularBuffer {
        public:
            static constexpr size_t BufferSize = 10000;
            static constexpr size_t MaxNameLength = 16;

        public:
            CircularBuffer();

            void Initialize(const char *name); // 10.0.0+, previously took event argument
            void Finalize();
            bool IsInitialized();
            u64 GetWriteableSize();
            void SetWriteCompleteEvent(os::EventType *event);
            Result Write(u8 type, const void *data, size_t size);
            void DiscardOldPackets(u8 type, u32 age_limit);
            CircularBufferPacket *Read();
            Result Free();

        private:
            ALWAYS_INLINE void _setReadOffset(u32 offset);
            ALWAYS_INLINE void _setWriteOffset(u32 offset);
            ALWAYS_INLINE u32 _getWriteOffset();
            ALWAYS_INLINE u32 _getReadOffset();
            ALWAYS_INLINE Result _write(u8 type, const void *data, size_t size);
            ALWAYS_INLINE void _updateUtilization();
            ALWAYS_INLINE CircularBufferPacket *_read();

        private:
            os::SdkMutex  m_mutex;
            os::EventType *m_event;

            u8 m_data[BufferSize];
            util::Atomic<u32> m_write_offset;
            util::Atomic<u32> m_read_offset;
            s64 m_size;
            char m_name[MaxNameLength + 1];
            bool m_initialized;
    };

    enum EventBufferType {
        EventBufferType_None,
        EventBufferType_HidReport,
        EventBufferType_Bluetooth,
        EventBufferType_Ble,
        EventBufferType_BleCore,
        EventBufferType_BleHid,
    };

    struct BufferedEventInfo {
        CircularBuffer buffer;
        EventBufferType type;
        bool ready;
    };

}
