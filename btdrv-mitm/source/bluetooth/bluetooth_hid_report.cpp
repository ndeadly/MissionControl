#include "bluetooth_hid_report.hpp"
#include "bluetooth_circularbuffer.hpp"
#include "../btdrv_mitm_flags.hpp"
#include "../controllermanager.hpp"
#include "../controllers/bluetoothcontroller.hpp"
#include "../controllers/switchcontroller.hpp"

#include "../btdrv_mitm_logging.hpp"

namespace ams::bluetooth::hid::report {

    namespace {

        os::ThreadType g_eventHandlerThread;
        alignas(os::ThreadStackAlignment) u8 g_eventHandlerThreadStack[0x2000];

        SharedMemory g_realBtShmem;
        SharedMemory g_fakeBtShmem;

        bluetooth::CircularBuffer *g_realCircBuff;
        bluetooth::CircularBuffer *g_fakeCircBuff;

        os::SystemEventType g_btHidReportSystemEvent;
        os::SystemEventType g_btHidReportSystemEventFwd;
        os::SystemEventType g_btHidReportSystemEventUser;

        u8 g_fakeReportBuffer[0x42] = {};
        HidReportData *g_fakeReportData = reinterpret_cast<HidReportData *>(g_fakeReportBuffer);

    }

    SharedMemory *GetRealSharedMemory(void) {
        return &g_realBtShmem;
    }

    SharedMemory *GetFakeSharedMemory(void) {
        return &g_fakeBtShmem;
    }

    os::SystemEventType *GetSystemEvent(void) {
        return &g_btHidReportSystemEvent;
    }

    os::SystemEventType *GetForwardEvent(void) {
        return &g_btHidReportSystemEventFwd;
    }

    os::SystemEventType *GetUserForwardEvent(void) {
        return &g_btHidReportSystemEventUser;
    }

    Result MapRemoteSharedMemory(Handle handle) {
        shmemLoadRemote(&g_realBtShmem, handle, BLUETOOTH_SHAREDMEM_SIZE, Perm_Rw);
        R_TRY(shmemMap(&g_realBtShmem));
        g_realCircBuff = reinterpret_cast<bluetooth::CircularBuffer *>(shmemGetAddr(&g_realBtShmem));
        BTDRV_LOG_FMT("Real shmem @ 0x%p", (void *)g_realCircBuff);

        return ams::ResultSuccess();
    }

    Result InitializeFakeSharedMemory(void) {
        R_TRY(shmemCreate(&g_fakeBtShmem, BLUETOOTH_SHAREDMEM_SIZE, Perm_Rw, Perm_Rw));
        R_TRY(shmemMap(&g_fakeBtShmem));
        g_fakeCircBuff = reinterpret_cast<CircularBuffer *>(shmemGetAddr(&g_fakeBtShmem));
        BTDRV_LOG_FMT("Fake shmem @ 0x%p", (void *)g_fakeCircBuff);

        // Initialise fake hid report buffer
        g_fakeCircBuff->Initialize("HID Report");
        g_fakeCircBuff->id = 1;
        g_fakeCircBuff->_unk3 = 1;

        return ams::ResultSuccess();
    }


    Result ProcessHidReportPackets(CircularBuffer *realBuffer, CircularBuffer *fakeBuffer) {
        controller::BluetoothController *controller;
        CircularBufferPacket *realPacket;          

        /*
        BTDRV_LOG_FMT("realBuffer: name: %s, roffs: %d, woffs: %d, capacity: %d, _unk3: %d, id: %d", 
            realBuffer->name,
            realBuffer->readOffset, 
            realBuffer->writeOffset,
            realBuffer->size,
            realBuffer->_unk3,
            realBuffer->id);
        BTDRV_LOG_FMT("fakeBuffer: name: %s, roffs: %d, woffs: %d, capacity: %d, _unk3: %d, id: %d", 
            fakeBuffer->name,
            fakeBuffer->readOffset, 
            fakeBuffer->writeOffset, 
            fakeBuffer->size,
            fakeBuffer->_unk3,
            fakeBuffer->id);
        */
        

        // Take snapshot of current write offset
        u32 writeOffset = realBuffer->writeOffset;

        while (true) {
            if (realBuffer->readOffset == writeOffset)
                break;

            // Get packet from real buffer
            //realPacket = reinterpret_cast<bluetooth::CircularBufferPacket *>(realBuffer->_read());
            realPacket = reinterpret_cast<CircularBufferPacket *>(&realBuffer->data[realBuffer->readOffset]);
            if (!realPacket)
                break;

            // Move read pointer past current packet (I think this is what Free does)
            if (realBuffer->readOffset != writeOffset) {
                u32 newOffset = realBuffer->readOffset + realPacket->header.size + sizeof(CircularBufferPacketHeader);
                if (newOffset >= BLUETOOTH_CIRCBUFFER_SIZE)
                    newOffset = 0;

                realBuffer->_setReadOffset(newOffset);
            }
            
            //BTDRV_LOG_DATA(&realPacket->data, realPacket->header.size);
            //BTDRV_LOG_DATA(realPacket, realPacket->header.size + sizeof(bluetooth::CircularBufferPacketHeader));
            //BTDRV_LOG_FMT("fakeBuffer: [%d] writing %d bytes to data[%d]", realPacket->header.type, realPacket->header.size + sizeof(bluetooth::CircularBufferPacketHeader), fakeBuffer->writeOffset);

            switch (realPacket->header.type) {
                case 0xff:
                    // Skip over packet type 0xff. This packet indicates the buffer should wrap around on next read.
                    // Since our buffer read and write offsets can differ from the real buffer we want to write this ourselves 
                    // when appropriate via CircularBuffer::Write()
                    continue;
                    
                case 4:
                    {
                        // Locate the controller that sent the report
                        controller = ams::mitm::btdrv::locateController(hos::GetVersion() < hos::Version_9_0_0 ? &realPacket->data.address : &realPacket->data.v2.address);
                        if (!controller) {
                            continue;
                        } 
                        
                        if (controller->isSwitchController()) {
                            // Write unmodified packet directly to fake buffer (_write call will add new timestamp)
                            fakeBuffer->Write(realPacket->header.type, &realPacket->data, realPacket->header.size);
                        }
                        else {
                            const HidReport *inReport;
                            HidReport *outReport;
                            // copy address and stuff over
                            if (hos::GetVersion() < hos::Version_10_0_0) {
                                g_fakeReportData->size = 0;    // Todo: calculate size of report data
                                std::memcpy(&g_fakeReportData->address, &realPacket->data.address, sizeof(BluetoothAddress));
                                inReport = &realPacket->data.report;
                                outReport = &g_fakeReportData->report;
                            }
                            else {
                                std::memcpy(&g_fakeReportData->v2.address, &realPacket->data.v2.address, sizeof(BluetoothAddress));
                                inReport = &realPacket->data.v2.report;
                                outReport = &g_fakeReportData->v2.report;
                            }

                            auto switchData = reinterpret_cast<controller::SwitchReportData *>(&outReport->data);
                            switchData->report0x30.timer = os::ConvertToTimeSpan(realPacket->header.timestamp).GetMilliSeconds() & 0xff;

                            // Translate packet to switch pro format
                            controller->convertReportFormat(inReport, outReport);
                            //BTDRV_LOG_DATA(g_fakeReportData, sizeof(g_fakeReportBuffer));

                            // Write the converted report to our fake buffer
                            fakeBuffer->Write(4, g_fakeReportData, sizeof(g_fakeReportBuffer));                
                        }
                    }
                    break;

                default:
                    BTDRV_LOG_FMT("unknown packet received: %d", realPacket->header.type);
                    fakeBuffer->Write(realPacket->header.type, &realPacket->data, realPacket->header.size);
                    break;
            }

        } 

        return ams::ResultSuccess();
    }

    void HandleEvent(void) {
        ProcessHidReportPackets(g_realCircBuff, g_fakeCircBuff);

        // Signal our forwarder events
        //os::SignalSystemEvent(&btHidReportSystemEventUser);
        os::SignalSystemEvent(&g_btHidReportSystemEventFwd);
    }


    void BluetoothHidReportEventThreadFunc(void *arg) {
        /*
        R_ABORT_UNLESS(hiddbgInitialize());
        // Todo: move these to some class constuctor or something?
        if (hos::GetVersion() >= hos::Version_7_0_0)
            R_ABORT_UNLESS(hiddbgAttachHdlsWorkBuffer());
        */

        while (true) {
            // Wait for real bluetooth event 
            os::WaitSystemEvent(&g_btHidReportSystemEvent);

            HandleEvent();
        }

        /*
        if (hos::GetVersion() >= hos::Version_7_0_0)
            R_ABORT_UNLESS(hiddbgReleaseHdlsWorkBuffer());
        
        hiddbgExit();
        */
    }

    ams::Result InitializeEvents(void) {
        R_TRY(os::CreateSystemEvent(&g_btHidReportSystemEventFwd, os::EventClearMode_AutoClear, true));
        R_TRY(os::CreateSystemEvent(&g_btHidReportSystemEventUser, os::EventClearMode_AutoClear, true));
        
        return ams::ResultSuccess();
    }

    ams::Result StartEventHandlerThread(void) {
        R_TRY(os::CreateThread(&g_eventHandlerThread, 
            BluetoothHidReportEventThreadFunc, 
            nullptr, 
            g_eventHandlerThreadStack, 
            sizeof(g_eventHandlerThreadStack), 
            -10
            //18  // priority of hid sysmodule
        ));

        os::StartThread(&g_eventHandlerThread); 

        return ams::ResultSuccess();
    }

}
