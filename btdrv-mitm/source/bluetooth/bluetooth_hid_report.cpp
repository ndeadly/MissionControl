#include "bluetooth_hid_report.hpp"

#include <atomic>
#include <mutex>
#include <cstring>
#include "bluetooth_circularbuffer.hpp"
#include "../btdrv_mitm_flags.hpp"
#include "../controllermanager.hpp"
#include "../controllers/bluetoothcontroller.hpp"
#include "../controllers/switchcontroller.hpp"

#include "../btdrv_mitm_logging.hpp"

namespace ams::bluetooth::hid::report {

    namespace {

        std::atomic<bool> g_isInitialized(false);

        os::ThreadType g_eventHandlerThread;
        alignas(os::ThreadStackAlignment) u8 g_eventHandlerThreadStack[0x2000];

        // This is only required  on fw < 7.0.0
        os::Mutex g_eventDataLock(false);
        u8 g_eventDataBuffer[0x480];
        HidEventType g_currentEventType;

        SharedMemory g_realBtShmem;
        SharedMemory g_fakeBtShmem;

        bluetooth::CircularBuffer *g_realBuffer;
        bluetooth::CircularBuffer *g_fakeBuffer;

        os::SystemEventType g_btHidReportSystemEvent;
        os::SystemEventType g_btHidReportSystemEventFwd;
        os::SystemEventType g_btHidReportSystemEventUser;

        u8 g_fakeReportBuffer[0x42] = {};
        HidReportData *g_fakeReportData = reinterpret_cast<HidReportData *>(g_fakeReportBuffer);

        void EventThreadFunc(void *arg) {
            while (true) {
                os::WaitSystemEvent(&g_btHidReportSystemEvent);
                HandleEvent();
            }
        }

    }

    bool IsInitialized(void) {
        return g_isInitialized;
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

    Result Initialize(Handle eventHandle) {
        os::AttachReadableHandleToSystemEvent(&g_btHidReportSystemEvent, eventHandle, false, os::EventClearMode_AutoClear);

        R_TRY(os::CreateSystemEvent(&g_btHidReportSystemEventFwd, os::EventClearMode_AutoClear, true));
        R_TRY(os::CreateSystemEvent(&g_btHidReportSystemEventUser, os::EventClearMode_AutoClear, true));

        R_TRY(os::CreateThread(&g_eventHandlerThread, 
            EventThreadFunc, 
            nullptr, 
            g_eventHandlerThreadStack, 
            sizeof(g_eventHandlerThreadStack), 
            -10
        ));

        os::StartThread(&g_eventHandlerThread); 

        g_isInitialized = true;

        return ams::ResultSuccess();
    }

    void Finalize(void) {
        os::DestroyThread(&g_eventHandlerThread);

        //if (hos::GetVersion() < hos::Version_7_0_0)
            //delete g_fakeBuffer;

        os::DestroySystemEvent(&g_btHidReportSystemEventUser);
        os::DestroySystemEvent(&g_btHidReportSystemEventFwd); 

        g_isInitialized = false;
    }

    Result MapRemoteSharedMemory(Handle handle) {
        shmemLoadRemote(&g_realBtShmem, handle, BLUETOOTH_SHAREDMEM_SIZE, Perm_Rw);
        R_TRY(shmemMap(&g_realBtShmem));
        g_realBuffer = reinterpret_cast<bluetooth::CircularBuffer *>(shmemGetAddr(&g_realBtShmem));
        BTDRV_LOG_FMT("Real shmem @ 0x%p", (void *)g_realBuffer);

        return ams::ResultSuccess();
    }

    Result InitializeReportBuffer(void) {
        BTDRV_LOG_FMT("btdrv-mitm: InitializeReportBuffer");

        // Todo: maybe just create shared memory for all fw?
        if (hos::GetVersion() < hos::Version_7_0_0) {
            g_fakeBuffer = new CircularBuffer();
        }
        else {
            R_TRY(shmemCreate(&g_fakeBtShmem, BLUETOOTH_SHAREDMEM_SIZE, Perm_Rw, Perm_Rw));
            R_TRY(shmemMap(&g_fakeBtShmem));
            g_fakeBuffer = reinterpret_cast<CircularBuffer *>(shmemGetAddr(&g_fakeBtShmem));
        }

        g_fakeBuffer->Initialize("HID Report");
        g_fakeBuffer->type = CircularBufferType_HidReport;
        g_fakeBuffer->_unk3 = 1;

        return ams::ResultSuccess();
    }

    /*
    Result InitializeFakeSharedMemory(void) {
        R_TRY(shmemCreate(&g_fakeBtShmem, BLUETOOTH_SHAREDMEM_SIZE, Perm_Rw, Perm_Rw));
        R_TRY(shmemMap(&g_fakeBtShmem));
        g_fakeBuffer = reinterpret_cast<CircularBuffer *>(shmemGetAddr(&g_fakeBtShmem));
        BTDRV_LOG_FMT("Fake shmem @ 0x%p", (void *)g_fakeBuffer);

        // Initialise fake hid report buffer the same way bluetooth does
        g_fakeBuffer->Initialize("HID Report");
        g_fakeBuffer->type = CircularBufferType_HidReport;
        g_fakeBuffer->_unk3 = 1;

        return ams::ResultSuccess();
    }
    */

    /* Write a fake report into the circular buffer */
    Result WriteFakeHidData(const Address *address, const HidData *data) {

        BTDRV_LOG_DATA_MSG((void*)data, data->length + sizeof(data->length), "btdrv-mitm: WriteFakeHidData");

        u16 bufferSize = data->length + 0x11;
        u8 buffer[bufferSize] = {};
        auto fakeReportData = reinterpret_cast<HidReportData *>(buffer);

        if (hos::GetVersion() < hos::Version_9_0_0) {
            fakeReportData->size = bufferSize;
            std::memcpy(&fakeReportData->address, address, sizeof(Address));
            std::memcpy(&fakeReportData->report, data, data->length + sizeof(data->length));
        }
        else {
            std::memcpy(&fakeReportData->v2.address, address, sizeof(Address));
            std::memcpy(&fakeReportData->v2.report, data, data->length + sizeof(data->length));
        }

        g_fakeBuffer->Write(4, fakeReportData, bufferSize); 
        os::SignalSystemEvent(&g_btHidReportSystemEventFwd);

        return ams::ResultSuccess();
    }

    /* Only used for < 7.0.0. newer firmwares read straight from shared memory */ 
    Result GetEventInfo(HidEventType *type, u8* buffer, size_t size) {

        /*
        auto packet = reinterpret_cast<CircularBufferPacket *>(g_fakeBuffer->Read());
        if (!packet)
            return -1;
        */

       //BTDRV_LOG_FMT("!!! GetEventInfo Called");

       CircularBufferPacket *packet;

        while (true) {
            if (g_fakeBuffer->readOffset == g_fakeBuffer->writeOffset)
                continue;

            // Get packet from real buffer
            packet = reinterpret_cast<CircularBufferPacket *>(&g_fakeBuffer->data[g_fakeBuffer->readOffset]);
            if (!packet)
                continue;

            // Move read pointer past current packet (I think this is what Free does)
            if (g_fakeBuffer->readOffset != g_fakeBuffer->writeOffset) {
                u32 newOffset = g_fakeBuffer->readOffset + packet->header.size + sizeof(CircularBufferPacketHeader);
                if (newOffset >= BLUETOOTH_CIRCBUFFER_SIZE)
                    newOffset = 0;

                g_fakeBuffer->_setReadOffset(newOffset);
            }

            if (packet->header.type == 0xff)
                continue;

            break;
        }



        auto eventData = reinterpret_cast<HidEventData *>(buffer);

        *type = static_cast<HidEventType>(packet->header.type);
        std::memcpy(&eventData->getReport.address, &packet->data.address, sizeof(Address));
        eventData->getReport.status = HidStatus_Ok;
        eventData->getReport.report_length = packet->header.size;

        std::memcpy(&eventData->getReport.report_data, &packet->data, packet->header.size);

        //BTDRV_LOG_DATA_MSG(&packet->data, packet->header.size, "btdrv-mitm: hid::report::GetEventInfo -> Read");
        //g_fakeBuffer->Free();
        
        return ams::ResultSuccess();
    }

    void _HandleEvent() {
        controller::BluetoothController *controller;
        CircularBufferPacket *realPacket;      

        // Take snapshot of current write offset
        u32 writeOffset = g_realBuffer->writeOffset;

        while (true) {
            if (g_realBuffer->readOffset == writeOffset)
                break;

            // Get packet from real buffer
            //realPacket = reinterpret_cast<bluetooth::CircularBufferPacket *>(g_realBuffer->_read());
            realPacket = reinterpret_cast<CircularBufferPacket *>(&g_realBuffer->data[g_realBuffer->readOffset]);
            if (!realPacket)
                break;

            // Move read pointer past current packet (I think this is what Free does)
            if (g_realBuffer->readOffset != writeOffset) {
                u32 newOffset = g_realBuffer->readOffset + realPacket->header.size + sizeof(CircularBufferPacketHeader);
                if (newOffset >= BLUETOOTH_CIRCBUFFER_SIZE)
                    newOffset = 0;

                g_realBuffer->_setReadOffset(newOffset);
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
                            g_fakeBuffer->Write(realPacket->header.type, &realPacket->data, realPacket->header.size);
                        }
                        else {
                            const HidReport *inReport;
                            HidReport *outReport;
                            // copy address and stuff over
                            if (hos::GetVersion() < hos::Version_9_0_0) {
                                g_fakeReportData->size = 0x42;
                                std::memcpy(&g_fakeReportData->address, &realPacket->data.address, sizeof(Address));
                                inReport = &realPacket->data.report;
                                outReport = &g_fakeReportData->report;
                            }
                            else {
                                std::memcpy(&g_fakeReportData->v2.address, &realPacket->data.v2.address, sizeof(Address));
                                inReport = &realPacket->data.v2.report;
                                outReport = &g_fakeReportData->v2.report;
                            }

                            auto switchData = reinterpret_cast<controller::SwitchReportData *>(&outReport->data);
                            switchData->report0x30.timer = os::ConvertToTimeSpan(realPacket->header.timestamp).GetMilliSeconds() & 0xff;

                            // Translate packet to switch pro format
                            controller->convertReportFormat(inReport, outReport);
                            //BTDRV_LOG_DATA(g_fakeReportData, sizeof(g_fakeReportBuffer));

                            // Write the converted report to our fake buffer
                            g_fakeBuffer->Write(4, g_fakeReportData, sizeof(g_fakeReportBuffer));                
                        }
                    }
                    break;

                default:

                    BTDRV_LOG_FMT("unknown packet received: %d", realPacket->header.type); 
                    //g_fakeBuffer->Write(realPacket->header.type, &realPacket->data, realPacket->header.size);
                    break;
            }

        } 
    }

    void _HandleEventDeprecated(void) {

        std::scoped_lock lk(g_eventDataLock);
        R_ABORT_UNLESS(btdrvGetHidReportEventInfo(&g_currentEventType, g_eventDataBuffer, sizeof(g_eventDataBuffer)));

        auto eventData = reinterpret_cast<HidEventData *>(g_eventDataBuffer);

        //BTDRV_LOG_FMT("hid report event [%02d]", g_currentEventType);

        switch (g_currentEventType) {

            case HidEvent_GetReport:
                {
                    // Locate the controller that sent the report
                    auto controller = ams::mitm::btdrv::locateController(&eventData->getReport.address);
                    if (!controller) {
                        return;
                    }
                    
                    if (controller->isSwitchController()) {
                        //BTDRV_LOG_DATA_MSG(&eventData->getReport.report_data, eventData->getReport.report_length, "Switch controller -> Write");
                        int rc = g_fakeBuffer->Write(g_currentEventType, &eventData->getReport.report_data, eventData->getReport.report_length);
                        //BTDRV_LOG_FMT("Write result: %d", rc);
                    }
                    else {
                        const HidReport *inReport;
                        HidReport *outReport;

                        //BTDRV_LOG_FMT("Non-Switch controller");

                        g_fakeReportData->size = 0x42; // Todo: check size is correct for report 0x30
                        std::memcpy(&g_fakeReportData->address, &eventData->getReport.address, sizeof(Address));
                        inReport = &eventData->getReport.report_data.report;
                        outReport = &g_fakeReportData->report;

                        auto switchData = reinterpret_cast<controller::SwitchReportData *>(&outReport->data);
                        switchData->report0x30.timer = os::ConvertToTimeSpan(os::GetSystemTick()).GetMilliSeconds() & 0xff;

                        // Translate packet to switch pro format
                        controller->convertReportFormat(inReport, outReport);
                        //BTDRV_LOG_DATA(g_fakeReportData, sizeof(g_fakeReportBuffer));

                        // Write the converted report to our fake buffer
                        g_fakeBuffer->Write(4, g_fakeReportData, sizeof(g_fakeReportBuffer));  
                    }
                }
                break;

            default:
                BTDRV_LOG_FMT("unknown packet received: %d", g_currentEventType); 
                //g_fakeBuffer->Write(g_currentEventType, &eventData->getReport.report_data, &eventData->getReport.report_length);
                break;
        }
    }

    void HandleEvent(void) {
        
        if (hos::GetVersion() < hos::Version_7_0_0) {
            _HandleEventDeprecated();
        }
        else {
            _HandleEvent();
        }

        if (!g_redirectHidReportEvents) {
            os::SignalSystemEvent(&g_btHidReportSystemEventFwd);
        }
        else {
            os::SignalSystemEvent(&g_btHidReportSystemEventUser);
        }
        
    }

}
