#include "bluetooth_hid_report_events.hpp"
#include "../btdrv_mitm_flags.hpp"
#include "../controllermanager.hpp"
#include "../controllers/bluetoothcontroller.hpp"
#include "../controllers/switchcontroller.hpp"

#include "../btdrv_mitm_logging.hpp"

ams::os::ThreadType g_bt_hid_report_event_task_thread;
alignas(ams::os::ThreadStackAlignment) u8 g_bt_hid_report_event_task_stack[0x2000];

SharedMemory g_realBtShmem;
SharedMemory g_fakeBtShmem;

ams::bluetooth::CircularBuffer *g_realCircBuff;
ams::bluetooth::CircularBuffer *g_fakeCircBuff;

ams::os::SystemEventType g_btHidReportSystemEvent;
ams::os::SystemEventType g_btHidReportSystemEventFwd;
ams::os::SystemEventType g_btHidReportSystemEventUser;

u8 g_fakeReportBuffer[0x42] = {};
HidReportData *g_fakeReportData = reinterpret_cast<HidReportData *>(g_fakeReportBuffer);

ams::Result ProcessHidReportPackets(ams::bluetooth::CircularBuffer *realBuffer, ams::bluetooth::CircularBuffer *fakeBuffer) {
    controller::BluetoothController *controller;
    ams::bluetooth::CircularBufferPacket *realPacket;          

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
        realPacket = reinterpret_cast<ams::bluetooth::CircularBufferPacket *>(&realBuffer->data[realBuffer->readOffset]);
        if (!realPacket)
            break;

        // Move read pointer past current packet (I think this is what Free does)
        if (realBuffer->readOffset != writeOffset) {
            u32 newOffset = realBuffer->readOffset + realPacket->header.size + sizeof(ams::bluetooth::CircularBufferPacketHeader);
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
                    controller = ams::mitm::btdrv::locateController(ams::hos::GetVersion() < ams::hos::Version_9_0_0 ? &realPacket->data.address : &realPacket->data.v2.address);
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
                        if (ams::hos::GetVersion() < ams::hos::Version_10_0_0) {
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
                        switchData->report0x30.timer = ams::os::ConvertToTimeSpan(realPacket->header.timestamp).GetMilliSeconds() & 0xff;

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


void BluetoothHidReportEventThreadFunc(void *arg) {
    /*
    R_ABORT_UNLESS(hiddbgInitialize());
    // Todo: move these to some class constuctor or something?
    if (hos::GetVersion() >= hos::Version_7_0_0)
        R_ABORT_UNLESS(hiddbgAttachHdlsWorkBuffer());
    */

    while (true) {
        // Wait for real bluetooth event 
        ams::os::WaitSystemEvent(&g_btHidReportSystemEvent);

        ProcessHidReportPackets(g_realCircBuff, g_fakeCircBuff);

        // Signal our forwarder events
        //os::SignalSystemEvent(&btHidReportSystemEventUser);
        ams::os::SignalSystemEvent(&g_btHidReportSystemEventFwd);

        //BTDRV_LOG_FMT("wrote hid report packets");
    }

    /*
    if (hos::GetVersion() >= hos::Version_7_0_0)
        R_ABORT_UNLESS(hiddbgReleaseHdlsWorkBuffer());
    
    hiddbgExit();
    */
}

ams::Result InitializeBluetoothHidReportEvents(void) {
    R_TRY(ams::os::CreateSystemEvent(&g_btHidReportSystemEventFwd, ams::os::EventClearMode_AutoClear, true));
    R_TRY(ams::os::CreateSystemEvent(&g_btHidReportSystemEventUser, ams::os::EventClearMode_AutoClear, true));
    
    return ams::ResultSuccess();
}

ams::Result InitializeBluetoothHidReportFakeSharedMemory(void) {
    R_TRY(shmemCreate(&g_fakeBtShmem, BLUETOOTH_SHAREDMEM_SIZE, Perm_Rw, Perm_Rw));
    R_TRY(shmemMap(&g_fakeBtShmem));
    g_fakeCircBuff = reinterpret_cast<ams::bluetooth::CircularBuffer *>(shmemGetAddr(&g_fakeBtShmem));
    BTDRV_LOG_FMT("Fake shmem @ 0x%p", (void *)g_fakeCircBuff);

    // Initialise fake hid report buffer
    g_fakeCircBuff->Initialize("HID Report");
    g_fakeCircBuff->id = 1;
    g_fakeCircBuff->_unk3 = 1;

    return ams::ResultSuccess();
}

ams::Result StartBluetoothHidReportEventThread(void) {
    R_TRY(ams::os::CreateThread(&g_bt_hid_report_event_task_thread, 
        BluetoothHidReportEventThreadFunc, 
        nullptr, 
        g_bt_hid_report_event_task_stack, 
        sizeof(g_bt_hid_report_event_task_stack), 
        -10
        //18  // priority of hid sysmodule
    ));

    ams::os::StartThread(&g_bt_hid_report_event_task_thread); 

    return ams::ResultSuccess();
}
