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
#include "bluetooth_circular_buffer.hpp"

namespace ams::bluetooth {

    CircularBuffer::CircularBuffer() {
        this->readOffset = 0;
        this->writeOffset = 0;
        this->isInitialized = false;
        this->event = nullptr;
    }

    void CircularBuffer::Initialize(const char *name) {
        if (!name || this->isInitialized)
            fatalThrow(-1);

        this->readOffset = 0;
        this->writeOffset = 0;
        std::strncpy(this->name, name, sizeof(this->name) - 1);
        this->_unk1 = 0;
        this->size = BLUETOOTH_BUFFER_SIZE;
        this->isInitialized = true;
    }

    void CircularBuffer::Finalize() {
        if (!this->isInitialized)
            fatalThrow(-1);

        this->isInitialized = false;
        this->event = nullptr;
    }

    bool CircularBuffer::IsInitialized() {
        return this->isInitialized;
    }

    u64 CircularBuffer::GetWriteableSize() {
        u32 readOffset = this->readOffset;
        u32 writeOffset = this->writeOffset;

        if (!this->isInitialized)
            return 0;

        u64 size;
        if (readOffset <= writeOffset)
            size = (BLUETOOTH_BUFFER_SIZE - 1) - writeOffset + readOffset;
        else
            size = readOffset - writeOffset - 1;

        if (size > BLUETOOTH_BUFFER_SIZE)
            size = 0;

        return size;
    }

    void CircularBuffer::SetWriteCompleteEvent(os::EventType *event) {
        this->event = event;
    }

    u64 CircularBuffer::Write(u8 type, void *data, size_t size) {

        if (!this->isInitialized)
            return -1;

        std::scoped_lock lk(this->mutex);

        ON_SCOPE_EXIT {
            if (this->event)
                os::SignalEvent(this->event);
        };

        if (size + sizeof(CircularBufferPacketHeader) <= this->GetWriteableSize()) {
            if (size + 2*sizeof(CircularBufferPacketHeader) > BLUETOOTH_BUFFER_SIZE - this->writeOffset) {
                if (const auto res = this->_write(0xff, nullptr, (BLUETOOTH_BUFFER_SIZE - this->writeOffset) - sizeof(CircularBufferPacketHeader)); res != 0) {
                    return res;
                }
            }

            if (const auto res = this->_write(type, data, size); res != 0) {
                return res;
            }
            this->_updateUtilization();

            return 0;
        }

        return -1;
    }

    void CircularBuffer::DiscardOldPackets(u8 type, u32 ageLimit) {
        if (this->isInitialized) {

            CircularBufferPacket *packet;
            TimeSpan timespan;
            do {
                if (this->readOffset == this->writeOffset)
                    return;

                packet = reinterpret_cast<CircularBufferPacket *>(&this->data[this->readOffset]);
                if (packet->header.type != 0xff) {

                    if (packet->header.type != type)
                        return;

                    timespan = os::ConvertToTimeSpan(os::GetSystemTick() - packet->header.timestamp);
                    if (timespan.GetMilliSeconds() <= ageLimit)
                        return;
                }

                this->Free();
            } while (this->isInitialized);
        }
    }

    CircularBufferPacket *CircularBuffer::Read() {
        return this->_read();
    }

    u64 CircularBuffer::Free() {
        if (!this->isInitialized)
            return -1;

        if (this->readOffset == this->writeOffset)
            return -1;

        auto packet = reinterpret_cast<CircularBufferPacket *>(&this->data[this->readOffset]);
        u32 newOffset = this->readOffset + packet->header.size + sizeof(packet->header);

        if (newOffset >= BLUETOOTH_BUFFER_SIZE)
            newOffset = 0;

        if (newOffset < BLUETOOTH_BUFFER_SIZE) {
            this->readOffset = newOffset;
            return 0;
        }

        fatalThrow(-1);
    }

    void CircularBuffer::_setReadOffset(u32 offset) {
        if (offset >= BLUETOOTH_BUFFER_SIZE)
            fatalThrow(-1);

        this->readOffset = offset;
    }

    void CircularBuffer::_setWriteOffset(u32 offset) {
        if (offset >= BLUETOOTH_BUFFER_SIZE)
            fatalThrow(-1);

        this->writeOffset = offset;
    }

    u32 CircularBuffer::_getWriteOffset() {
        return this->writeOffset;
    }

    u32 CircularBuffer::_getReadOffset() {
        return this->readOffset;
    }

    u64 CircularBuffer::_write(u8 type, void *data, size_t size) {
        auto packet = reinterpret_cast<CircularBufferPacket *>(&this->data[this->writeOffset]);
        packet->header.type = type;
        packet->header.timestamp = os::GetSystemTick();
        packet->header.size = size;

        if (type != 0xff) {
            if (data && (size > 0))
                memcpy(&packet->data, data, size);
            else
                return -1;
        }

        u32 newOffset = this->writeOffset + size + sizeof(CircularBufferPacketHeader);
        if (newOffset > BLUETOOTH_BUFFER_SIZE)
            return -1;

        if (newOffset == BLUETOOTH_BUFFER_SIZE)
            this->writeOffset = 0;
        else
            this->writeOffset = newOffset;

        return 0;
    }

    void CircularBuffer::_updateUtilization() {
        u32 newCapacity = this->isInitialized ? this->GetWriteableSize() : 0;

        if (this->size > newCapacity + 1000)
            this->size = newCapacity;
    }

    CircularBufferPacket *CircularBuffer::_read() {
        if (this->isInitialized) {
            CircularBufferPacket *packet;
            u32 newOffset;
            do {
                if (this->readOffset == this->writeOffset)
                    return nullptr;

                packet = reinterpret_cast<CircularBufferPacket *>(&this->data[this->readOffset]);

                if (packet->header.type != 0xff)
                    return packet;

                if (!this->isInitialized)
                    return nullptr;

                if (this->readOffset != this->writeOffset) {
                    newOffset = this->readOffset + packet->header.size + sizeof(packet->header);
                    if (newOffset >= BLUETOOTH_BUFFER_SIZE)
                        newOffset = 0;

                    this->_setReadOffset(newOffset);
                }

            } while (this->isInitialized);
        }

        return nullptr;
    }

}
