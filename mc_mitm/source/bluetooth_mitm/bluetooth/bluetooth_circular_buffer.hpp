/*
 * Copyright (c) 2020-2022 ndeadly
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

    constexpr int BLUETOOTH_BUFFER_SIZE = 10000;

    enum CircularBufferType {
        CircularBufferType_Other,
        CircularBufferType_HidReport,
        CircularBufferType_Bluetooth,
        CircularBufferType_Ble,
        CircularBufferType_BleCore,
        CircularBufferType_BleHid,
    };

    struct CircularBufferPacketHeader{
        u8       type;
        os::Tick timestamp;
        u64      size;
    };

    struct CircularBufferPacket{
        CircularBufferPacketHeader header;
        HidReportEventInfo data;
    };

    class CircularBuffer {

        public:
            CircularBuffer();

            void Initialize(const char *name); // 10.0.0+, previously took event argument
            void Finalize();
            bool IsInitialized();
            u64 GetWriteableSize();
            void SetWriteCompleteEvent(os::EventType *event);
            u64 Write(u8 type, void *data, size_t size);
            void DiscardOldPackets(u8 type, u32 ageLimit);
            CircularBufferPacket *Read();
            u64 Free();

        private:
            void _setReadOffset(u32 offset);
            void _setWriteOffset(u32 offset);
            u32  _getWriteOffset();
            u32  _getReadOffset();
            u64  _write(u8 type, void *data, size_t size);
            void _updateUtilization();
            CircularBufferPacket *_read();

            os::SdkMutex  mutex;
            os::EventType *event;
            
            u8   data[BLUETOOTH_BUFFER_SIZE];
            u32  writeOffset;
            u32  readOffset;
            s64  size;
            char name[16];
            u8   _unk1;
            bool isInitialized;
            u8   _unk2[6];

        public:
            CircularBufferType type;
            bool _unk3;
            //u8 _unk3[4];
    };

}
