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
#include "bluetooth_circular_buffer.hpp"

namespace ams::bluetooth {

    CircularBuffer::CircularBuffer() {
        m_read_offset = 0;
        m_write_offset = 0;
        m_initialized = false;
        m_event = nullptr;
    }

    void CircularBuffer::Initialize(const char *name) {
        AMS_ABORT_UNLESS(!(m_initialized || name == nullptr));

        m_read_offset = 0;
        m_write_offset = 0;
        std::strncpy(m_name, name, CircularBuffer::MaxNameLength);
        m_name[CircularBuffer::MaxNameLength] = '\0';
        m_size = CircularBuffer::BufferSize;
        m_initialized = true;
    }

    void CircularBuffer::Finalize() {
        AMS_ABORT_UNLESS(m_initialized);

        m_initialized = false;
        m_event = nullptr;
    }

    bool CircularBuffer::IsInitialized() {
        return m_initialized;
    }

    u64 CircularBuffer::GetWriteableSize() {
        if (!m_initialized) {
            return 0;
        }

        u32 read_offset = this->_getReadOffset();
        u32 write_offset = this->_getWriteOffset();

        u64 size;
        if (read_offset <= write_offset) {
            size = (CircularBuffer::BufferSize - 1) - write_offset + read_offset;
        } else {
            size = read_offset - write_offset - 1;
        }

        if (size > CircularBuffer::BufferSize) {
            size = 0;
        }

        return size;
    }

    void CircularBuffer::SetWriteCompleteEvent(os::EventType *event) {
        m_event = event;
    }

    Result CircularBuffer::Write(u8 type, const void *data, size_t size) {
        if (!m_initialized) {
            R_RETURN(-1);
        }

        std::scoped_lock lk(m_mutex);

        ON_SCOPE_EXIT {
            if (m_event) {
                os::SignalEvent(m_event);
            }
        };

        if (size + sizeof(CircularBufferPacketHeader) > this->GetWriteableSize()) {
            R_RETURN(-1);
        }

        u32 write_offset = this->_getWriteOffset();
        if (size + 2*sizeof(CircularBufferPacketHeader) > CircularBuffer::BufferSize - write_offset) {
            R_TRY(this->_write(0xff, nullptr, (CircularBuffer::BufferSize - write_offset) - sizeof(CircularBufferPacketHeader)));
        }

        R_TRY(this->_write(type, data, size));

        this->_updateUtilization();

        R_SUCCEED();
    }

    void CircularBuffer::DiscardOldPackets(u8 type, u32 age_limit) {
        while (m_initialized) {
            u32 read_offset = this->_getReadOffset();
            u32 write_offset = this->_getWriteOffset();

            if (read_offset == write_offset) {
                return;
            }

            auto packet = reinterpret_cast<CircularBufferPacket *>(&m_data[read_offset]);
            if (packet->header.type != 0xff) {

                if (packet->header.type != type) {
                    return;
                }

                TimeSpan timespan = os::ConvertToTimeSpan(os::GetSystemTick() - packet->header.timestamp);
                if (timespan.GetMilliSeconds() <= age_limit) {
                    return;
                }
            }

            u32 new_offset = read_offset + packet->header.size + sizeof(packet->header);
            if (new_offset >= CircularBuffer::BufferSize) {
                new_offset = 0;
            }

            this->_setReadOffset(new_offset);
        }
    }

    CircularBufferPacket *CircularBuffer::Read() {
        return this->_read();
    }

    Result CircularBuffer::Free() {
        if (!m_initialized) {
            R_RETURN(-1);
        }

        u32 read_offset = this->_getReadOffset();
        u32 write_offset = this->_getWriteOffset();

        if (read_offset == write_offset) {
            R_RETURN(-1);
        }

        auto packet = reinterpret_cast<CircularBufferPacket *>(&m_data[read_offset]);
        u32 new_offset = read_offset + packet->header.size + sizeof(packet->header);
        if (new_offset >= CircularBuffer::BufferSize) {
            new_offset = 0;
        }

        this->_setReadOffset(new_offset);

        R_SUCCEED();
    }

    void CircularBuffer::_setReadOffset(u32 offset) {
        AMS_ABORT_UNLESS(offset < CircularBuffer::BufferSize);

        m_read_offset = offset;
    }

    void CircularBuffer::_setWriteOffset(u32 offset) {
        AMS_ABORT_UNLESS(offset < CircularBuffer::BufferSize);

        m_write_offset = offset;
    }

    u32 CircularBuffer::_getWriteOffset() {
        return m_write_offset;
    }

    u32 CircularBuffer::_getReadOffset() {
        return m_read_offset;
    }

    Result CircularBuffer::_write(u8 type, const void *data, size_t size) {
        u32 write_offset = this->_getWriteOffset();

        auto packet = reinterpret_cast<CircularBufferPacket *>(&m_data[write_offset]);
        packet->header.type = type;
        packet->header.timestamp = os::GetSystemTick();
        packet->header.size = size;

        if (type != 0xff) {
            if (!(data && size)) {
                R_RETURN(-1);
            }

            memcpy(&packet->data, data, size);
        }

        u32 new_offset = write_offset + size + sizeof(CircularBufferPacketHeader);
        if (new_offset == CircularBuffer::BufferSize) {
            new_offset = 0;
        } else if (new_offset > CircularBuffer::BufferSize) {
            R_RETURN(-1);
        }

        this->_setWriteOffset(new_offset);

        R_SUCCEED();
    }

    void CircularBuffer::_updateUtilization() {
        u32 new_capacity = m_initialized ? this->GetWriteableSize() : 0;

        if (m_size > new_capacity + 1000) {
            m_size = new_capacity;
        }
    }

    CircularBufferPacket *CircularBuffer::_read() {
        while (m_initialized) {
            u32 read_offset = this->_getReadOffset();
            u32 write_offset = this->_getWriteOffset();

            if (read_offset == write_offset) {
                break;
            }

            auto packet = reinterpret_cast<CircularBufferPacket *>(&m_data[read_offset]);

            if (packet->header.type != 0xff) {
                return packet;
            }

            u32 new_offset = read_offset + packet->header.size + sizeof(packet->header);
            if (new_offset >= CircularBuffer::BufferSize) {
                new_offset = 0;
            }

            this->_setReadOffset(new_offset);
        };

        return nullptr;
    }

}
