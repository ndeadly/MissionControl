#include <cstring>
#include "circularbuffer.hpp"

#include "btdrv_mitm_logging.hpp"

namespace ams::bluetooth {

    // done
    CircularBuffer::CircularBuffer(void) {
        this->readOffset = 0;
        this->writeOffset = 0;
        this->isInitialized = false;
    }

    // done except unknowns
    void CircularBuffer::Initialize(const char *name) {
        if (!name || this->isInitialized)
            fatalThrow(-1);

        this->readOffset = 0;
        this->writeOffset = 0;
        std::strncpy(this->name, name, 0x10);
        this->_unk1 = 0;
        this->size = BLUETOOTH_CIRCBUFFER_SIZE;
        os::InitializeSdkMutex(&this->mutex);
        this->isInitialized = true;
    }

    // done
    void CircularBuffer::Finalize(void) {
        if (!this->isInitialized)
            fatalThrow(-1);
            
        this->isInitialized = false;
        this->event = nullptr;
    }

    // done
    bool CircularBuffer::IsInitialized(void) {
        return this->isInitialized;
    }

    // done
    u64 CircularBuffer::GetWriteableSize(void) {
        u32 readOffset = this->readOffset;
        u32 writeOffset = this->writeOffset;

        if (!this->isInitialized)
            return 0;

        u64 size;
        if (readOffset <= writeOffset)
            size = (BLUETOOTH_CIRCBUFFER_SIZE - 1) - writeOffset + readOffset;
        else
            size = readOffset + ~writeOffset;

        if (size > BLUETOOTH_CIRCBUFFER_SIZE) 
            size = 0;

        return size;
    }

    // done
    void CircularBuffer::AttachEvent(os::EventType *event) {
        this->event = event;
    }

    // *** WIP ***
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

    // possibly done
    void CircularBuffer::DiscardOldPackets(u8 type, u32 ageLimit) {
        if (this->isInitialized) {

            CircularBufferPacket *packet;
            TimeSpan timespan;
            do {
                if (this->readOffset == this->writeOffset)
                    return;

                packet = reinterpret_cast<CircularBufferPacket *>(this->data[this->readOffset]);
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

    // done
    void *CircularBuffer::Read(void) {
        return this->_read();
    }

    // done
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


    // done (needs testing) <- new one
    /*
    u64 CircularBuffer::Free(void) {
        if (!this->isInitialized)
            return -1;
        
        if (this->readOffset == this->writeOffset)
            return -1;
        
        CircularBufferPacket *packet = reinterpret_cast<CircularBufferPacket *>(this->data[this->readOffset]);
        u32 newOffset = this->readOffset + packet->header.size + sizeof(packet->header);
        
        if (newOffset >= BLUETOOTH_CIRCBUFFER_SIZE)
            newOffset = 0;
                    
        if (newOffset < BLUETOOTH_CIRCBUFFER_SIZE) {
            this->readOffset = newOffset;
            return 0;
        }
        
        fatalThrow(-1);
    }
    */




    /*
    // hid_report_buffer_write + modifications
    u64 CircularBuffer::WritePacket(const CircularBufferPacket *srcPacket) {
        if (!this->_unk3 || !this->isInitialized)
            return -1;

        this->DiscardOldPackets(0x27, 0x64);

        if () {}

        if (size > BLUETOOTH_CIRCBUFFER_SIZE)
            return -1;


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
    */

    // done 
    void CircularBuffer::_setReadOffset(u32 offset) {
        if (offset >= BLUETOOTH_CIRCBUFFER_SIZE)
            fatalThrow(-1);

        this->readOffset = offset;
    }

    // done
    void CircularBuffer::_setWriteOffset(u32 offset) {
        if (offset >= BLUETOOTH_CIRCBUFFER_SIZE)
            fatalThrow(-1);

        this->writeOffset = offset;
    }

    // done
    u32 CircularBuffer::_getWriteOffset(void) {
        return this->writeOffset;
    }

    // done
    u32 CircularBuffer::_getReadOffset(void) {
        return this->readOffset;
    }

    // done
    u64 CircularBuffer::_write(u8 type, void *data, size_t size) {
        CircularBufferPacket *packet = reinterpret_cast<CircularBufferPacket *>(&this->data[this->writeOffset]);
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
        if (newOffset > BLUETOOTH_CIRCBUFFER_SIZE)
            return -1;

        if (newOffset == BLUETOOTH_CIRCBUFFER_SIZE)
            this->writeOffset = 0; 
        else
            this->writeOffset = newOffset;

        return 0;
    }

    // possibly done
    /*
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
    */

    // done
    void CircularBuffer::_updateUtilization(void) {
        u32 newCapacity = this->isInitialized ? this->GetWriteableSize() : 0;

        if (this->size > newCapacity + 1000)
            this->size = newCapacity;
    }

    // done (needs checking) <-new one
    void *CircularBuffer::_read(void) {
        if (this->isInitialized) {
            CircularBufferPacket *packet;
            u32 newOffset;
            do {
                if (this->readOffset == this->writeOffset)
                    return nullptr;

                packet = reinterpret_cast<CircularBufferPacket *>(&this->data[this->readOffset]);
                
                if (packet->header.type != 0xff)
                    return packet;
                    //return &this->data[this->readOffset];
                                
                if (!this->isInitialized)
                    return nullptr;
                
                if (this->readOffset != this->writeOffset) {
                    newOffset = this->readOffset + packet->header.size + sizeof(packet->header);
                    if (newOffset >= BLUETOOTH_CIRCBUFFER_SIZE)
                        newOffset = 0;
                    
                    this->_setReadOffset(newOffset);
                }
                
            } while (this->isInitialized);
        }	
        
        return nullptr;
    }

    /*
    // done
    void *CircularBuffer::_read(void) {
        if (this->isInitialized) {
            do {
                u32 readOffset = this->readOffset;
                if (readOffset == this->writeOffset)
                    break;
                
                if (this->data[readOffset] != 0xff)
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
    */


}
