#pragma once
#include <stratosphere.hpp>
#include <switch.h>

#define BLUETOOTH_CIRCBUFFER_SIZE 10000

namespace ams::bluetooth {

    enum CircularBufferType {
        CircularBufferType_HidReport = 0x1,
        CircularBufferType_Bluetooth,
        CircularBufferType_Ble,
        CircularBufferType_BleCore,
        CircularBufferType_BleHid,
    };

    struct CircularBufferPacketHeader{
        u8          type;           //+0x00
        os::Tick    timestamp;      //+0x08
        u64         size;           //+0x10
    };

    struct CircularBufferPacket{
        CircularBufferPacketHeader header;
        BluetoothHidReportData     data;
    };

    //class CircularBuffer {
    struct CircularBuffer {
        //public:
            CircularBuffer(void);

            void Initialize(const char *name); // 10.0.0+, previously took event argument
            void Finalize(void);
            bool IsInitialized(void);
            u64  GetWriteableSize(void);
            void SetWriteCompleteEvent(os::EventType *event);
            u64  Write(u8 type, void *data, size_t size);
            void DiscardOldPackets(u8 a1, u32 a2);
            void *Read(void);
            u64  Free(void);

        //private:
            void _setReadOffset(u32 offset);
            void _setWriteOffset(u32 offset);
            u32  _getWriteOffset(void);
            u32  _getReadOffset(void);
            u64  _write(u8 type, void *data, size_t size);
            void _updateUtilization(void);
            void *_read(void);

        //private:
            os::SdkMutex    mutex;
            os::EventType   *event;
            
            u8      data[BLUETOOTH_CIRCBUFFER_SIZE];
            u32     writeOffset;
            u32     readOffset;
            s64 	size;
            char    name[16];
            u8      _unk1;
            bool 	isInitialized;
            u8      _unk2[6];
            CircularBufferType 	type;
            bool    _unk3;
            //u8      _unk3[4];
    };

}
