#include <cstring>
#include "circularbuffer.hpp"

#include "btdrv_mitm_logging.hpp"

namespace ams::bluetooth {

    CircularBuffer::CircularBuffer(void) {
        this->readOffset = 0;
        this->writeOffset = 0;
        this->isInitialized = false;
    }

    void CircularBuffer::Initialize(const char *name) {
        if (!name || this->isInitialized)
            fatalThrow(-1);

        this->readOffset = 0;
        this->writeOffset = 0;
        strncpy(this->name, name, 0x10);
        this->_unk1 = 0;
        this->size = BLUETOOTH_CIRCBUFFER_SIZE;
        //this->_unk0 = 0; // Maybe init mutex?
        os::InitializeSdkMutex(&this->mutex);
        this->isInitialized = true;
    }

    void CircularBuffer::Finalize(void) {
        if (!this->isInitialized)
            fatalThrow(-1);
            
        this->isInitialized = false;
        this->event = nullptr;
    }

    bool CircularBuffer::IsInitialized(void) {
        return this->isInitialized;
    }

    u64 CircularBuffer::GetWriteableSize(void) {
        u32 readOffset = this->readOffset;
        u32 writeOffset = this->writeOffset;
        u64 size;

        if (!this->isInitialized)
            return 0;

        if (readOffset <= writeOffset)
            size = (BLUETOOTH_CIRCBUFFER_SIZE - 1) - writeOffset + readOffset;
        else
            size = readOffset + ~writeOffset;

        if (size > BLUETOOTH_CIRCBUFFER_SIZE) 
            size = 0;

        return size;
    }

    void CircularBuffer::AttachEvent(os::EventType *event) {
        this->event = event;
    }

    u64  CircularBuffer::Write(u8 type, void *data, size_t size) {
        if (this->IsInitialized()) {
            os::LockSdkMutex(&this->mutex);

            u32 writeableSize = this->GetWriteableSize();
            u32 packetSize = size + sizeof(CircularBufferPacketHeader);

            if (packetSize > writeableSize) {
                if (this->event)
                    os::SignalEvent(this->event);

                os::UnlockSdkMutex(&this->mutex);
                return -1;
            }
            else {
                if (packetSize > 0) {

                }

            }


            // Not sure how to connec this logic just yet

        }

        // Todo: finish this
        return -1;
    }

    void CircularBuffer::DiscardOldPackets(u8 type, u32 age) {
        if (this->isInitialized) {

            CircularBufferPacket *packet;
            //u64 timespan;
            TimeSpan ts;
            do {
                if (this->readOffset == this->writeOffset)
                    return;

                packet = reinterpret_cast<CircularBufferPacket *>(this->data[this->readOffset]);
                if (packet->header.type != 0xff) {

                    if (packet->header.type != type)
                        return;

                    //timespan = armTicksToNs(armGetSystemTick() - packet->header.timestamp);
                    //ts = armTicksToNs(armGetSystemTick() - packet->header.timestamp);
                    ts = os::ConvertToTimeSpan(os::GetSystemTick() - packet->header.timestamp);

                    /*
                    u64 uVar1 = ((s64)ts >> 0x20) * 0xd7b634db + ((ts & 0xffffffff) * 0xd7b634db >> 0x20);
                    if ((((s64)ts >> 0x20) * 0x431bde82 + ((s64)uVar1 >> 0x20) +
                        ((s64)((ts & 0xffffffff) * 0x431bde82 + (uVar1 & 0xffffffff)) >> 0x20) >>
                        0x12) - ((s64)ts >> 0x3f) <= (s64)age) {
                        return;
                    }
                    */

                }

                this->Free();
            } while (this->isInitialized);
        }
    }

    void *CircularBuffer::Read(void) {
        return this->_read();
    }

    u64  CircularBuffer::Free(void) {
        if (this->isInitialized) {

            u32 readOffset = this->readOffset;
            if (readOffset != this->writeOffset) {
                u32 newOffset = readOffset + this->data[readOffset + 16] + sizeof(CircularBufferPacketHeader);
                if (newOffset >= BLUETOOTH_CIRCBUFFER_SIZE) {
                    newOffset = 0;
                }
                
                if (newOffset < BLUETOOTH_CIRCBUFFER_SIZE) {
                    this->readOffset = newOffset;
                    return 0;
                }

                // Maybe fatalThrow here?
                //fatalThrow(-1);
            }
        }
        
        return -1;
    }

    // hid_report_buffer_write + modifications
    u64 CircularBuffer::WritePacket(const CircularBufferPacket *srcPacket) {
        if (!this->_unk3 || !this->isInitialized)
            return -1;

        /*
        this->DiscardOldPackets(0x27, 0x64);

        if () {}

        if (size > BLUETOOTH_CIRCBUFFER_SIZE)
            return -1;
        */


        BTDRV_LOG_FMT("write offset: %d", this->writeOffset);
        BTDRV_LOG_FMT("buffer address: 0x%p", (void *)this);
        BTDRV_LOG_FMT("data address: 0x%p", (void *)&this->data);
        CircularBufferPacket *packet = reinterpret_cast<CircularBufferPacket *>(&this->data[this->writeOffset]);
        BTDRV_LOG_FMT("packet address: 0x%p", (void *)packet);
        packet->header.type = srcPacket->header.type;
        packet->header.timestamp = srcPacket->header.timestamp;
        packet->header.size = srcPacket->header.size;

        if (srcPacket->header.type != -1) {
            if (&srcPacket->data && (srcPacket->header.size > 0))
                memcpy(&packet->data, &srcPacket->data, srcPacket->header.size);
            else 
                return -1;
        }

        u32 newOffset = this->writeOffset + srcPacket->header.size + sizeof(packet->header);
        if (newOffset >= BLUETOOTH_CIRCBUFFER_SIZE)
            return -1;

        if (newOffset == BLUETOOTH_CIRCBUFFER_SIZE)
            this->writeOffset = 0; 
        else
            this->writeOffset = newOffset;

        return 0;
    }

    void CircularBuffer::_setReadOffset(u32 offset) {
        if (offset >= BLUETOOTH_CIRCBUFFER_SIZE)
            fatalThrow(-1);

        this->readOffset = offset;
    }

    void CircularBuffer::_setWriteOffset(u32 offset) {
        if (offset >= BLUETOOTH_CIRCBUFFER_SIZE)
            fatalThrow(-1);

        this->writeOffset = offset;
    }

    u32 CircularBuffer::_getWriteOffset(void) {
        return this->writeOffset;
    }

    u32 CircularBuffer::_getReadOffset(void) {
        return this->readOffset;
    }

    u64 CircularBuffer::_write(u8 type, void *data, size_t size) {
        CircularBufferPacket *packet = reinterpret_cast<CircularBufferPacket *>(&this->data[this->writeOffset]);
        packet->header.type = type;
        packet->header.timestamp = os::GetSystemTick(); //armGetSystemTick();
        packet->header.size = size;

        if (type != -1) {
            if (data && (size > 0))
                memcpy(&packet->data, data, size);
            else 
                return -1;
        }

        u32 newOffset = this->writeOffset + size + sizeof(CircularBufferPacketHeader);
        if (newOffset >= BLUETOOTH_CIRCBUFFER_SIZE)
            return -1;

        if (newOffset == BLUETOOTH_CIRCBUFFER_SIZE)
            this->writeOffset = 0; 
        else
            this->writeOffset = newOffset;

        return 0;
    }

    void CircularBuffer::_updateUtilization(void) {
        if(this->isInitialized) {
            u32 writeableSize = this->GetWriteableSize();
            if (writeableSize + 1000 < this->size)
                this->size = writeableSize;
        }
        else if(this->size > 1000) {
            this->size = 0;
        }
    }

    void *CircularBuffer::_read(void) {
        if (this->isInitialized) {
            do {
                u32 readOffset = this->readOffset;
                if (readOffset == this->writeOffset)
                    break;
                
                if (this->data[readOffset] != -1)
                    return this->data + readOffset;
                
                if (!this->isInitialized)
                    break;

                readOffset = this->readOffset;
                if (readOffset != this->writeOffset) {
                    u32 newOffset = readOffset + this->data[readOffset + 16] + sizeof(CircularBufferPacketHeader);
                    if (newOffset >= BLUETOOTH_CIRCBUFFER_SIZE)
                        newOffset = 0;

                    this->_setReadOffset(newOffset);
                }
            
            } while(this->isInitialized);
        }
        
        return nullptr;
    }

}
