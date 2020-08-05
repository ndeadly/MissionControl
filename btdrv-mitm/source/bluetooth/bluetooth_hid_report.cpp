#include "bluetooth_hid_report.hpp"

#include <atomic>
#include <mutex>
#include <cstring>
#include "bluetooth_circularbuffer.hpp"
#include "../btdrv_mitm_flags.hpp"
#include "../controllers/controllermanager.hpp"

#include "../btdrv_mitm_logging.hpp"

namespace ams::bluetooth::hid::report {

    namespace {

        std::atomic<bool> g_isInitialized(false);

        os::ThreadType g_eventHandlerThread;
        alignas(os::ThreadStackAlignment) u8 g_eventHandlerThreadStack[0x2000];

        // This is only required  on fw < 7.0.0
        os::Mutex g_eventDataLock(false);
        u8 g_eventDataBuffer[0x480];
        bluetooth::HidEventType g_currentEventType;

        SharedMemory g_realBtShmem;
        SharedMemory g_fakeBtShmem;

        bluetooth::CircularBuffer *g_realBuffer;
        bluetooth::CircularBuffer *g_fakeBuffer;

        os::SystemEventType g_btHidReportSystemEvent;
        os::SystemEventType g_btHidReportSystemEventFwd;
        os::SystemEventType g_btHidReportSystemEventUser;

        u8 g_fakeReportBuffer[0x42] = {};
        bluetooth::HidReportData *g_fakeReportData = reinterpret_cast<bluetooth::HidReportData *>(g_fakeReportBuffer);

        // Buffer for hid report responses. Might be able to replace the above
        bluetooth::HidReport g_hidReport = {};

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
        if (hos::GetVersion() < hos::Version_7_0_0)
            return nullptr;

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

        os::DestroySystemEvent(&g_btHidReportSystemEventUser);
        os::DestroySystemEvent(&g_btHidReportSystemEventFwd); 

        g_isInitialized = false;
    }

    Result MapRemoteSharedMemory(Handle handle) {
        shmemLoadRemote(&g_realBtShmem, handle, BLUETOOTH_SHAREDMEM_SIZE, Perm_Rw);
        R_TRY(shmemMap(&g_realBtShmem));
        g_realBuffer = reinterpret_cast<bluetooth::CircularBuffer *>(shmemGetAddr(&g_realBtShmem));

        return ams::ResultSuccess();
    }

    Result InitializeReportBuffer(void) {
        BTDRV_LOG_FMT("btdrv-mitm: InitializeReportBuffer");

        R_TRY(shmemCreate(&g_fakeBtShmem, BLUETOOTH_SHAREDMEM_SIZE, Perm_Rw, Perm_Rw));
        R_TRY(shmemMap(&g_fakeBtShmem));
        g_fakeBuffer = reinterpret_cast<bluetooth::CircularBuffer *>(shmemGetAddr(&g_fakeBtShmem));

        g_fakeBuffer->Initialize("HID Report");
        g_fakeBuffer->type = bluetooth::CircularBufferType_HidReport;
        g_fakeBuffer->_unk3 = 1;

        return ams::ResultSuccess();
    }

    /* Write a fake report into the circular buffer */
    Result WriteFakeHidData(const bluetooth::Address *address, const bluetooth::HidReport *report) {

        //BTDRV_LOG_DATA_MSG((void*)report, report->size + sizeof(report->size), "btdrv-mitm: WriteFakeHidData");

        u16 bufferSize = report->size + 0x11;
        u8 buffer[bufferSize] = {};
        auto fakeReportData = reinterpret_cast<bluetooth::HidReportData *>(buffer);

        if (hos::GetVersion() < hos::Version_9_0_0) {
            fakeReportData->size = bufferSize;
            std::memcpy(&fakeReportData->address, address, sizeof(bluetooth::Address));
            std::memcpy(&fakeReportData->report, report, report->size + sizeof(report->size));
        }
        else {
            std::memcpy(&fakeReportData->v2.address, address, sizeof(bluetooth::Address));
            std::memcpy(&fakeReportData->v2.report, report, report->size + sizeof(report->size));
        }

        g_fakeBuffer->Write(4, fakeReportData, bufferSize); 
        os::SignalSystemEvent(&g_btHidReportSystemEventFwd);

        return ams::ResultSuccess();
    }

    /* Write a fake subcommand response into buffer */
    Result FakeSubCmdResponse(const bluetooth::Address *address, const u8 response[], size_t size) {
        auto report = &g_hidReport;
        auto reportData = reinterpret_cast<controller::SwitchReportData *>(&report->data);
        report->size = sizeof(controller::SwitchInputReport0x21);
        reportData->id   = 0x21;
        reportData->input0x21.conn_info   = 0;
        reportData->input0x21.battery     = 8;
        reportData->input0x21.buttons     = {0x00, 0x00, 0x00};
        reportData->input0x21.left_stick  = {0x0b, 0xb8, 0x78};
        reportData->input0x21.right_stick = {0xd9, 0xd7, 0x81};
        reportData->input0x21.vibrator    = 0;
        std::memcpy(&reportData->input0x21.subcmd, response, size);

        reportData->input0x21.timer = os::ConvertToTimeSpan(os::GetSystemTick()).GetMilliSeconds() & 0xff;

        // Todo: change types so we don't have to cast
        return bluetooth::hid::report::WriteFakeHidData(address, report);
    }

    /* Only used for < 7.0.0. Newer firmwares read straight from shared memory */ 
    Result GetEventInfo(bluetooth::HidEventType *type, u8* buffer, size_t size) {

       //BTDRV_LOG_FMT("!!! GetEventInfo Called");

        while (true) {

            auto packet = g_fakeBuffer->Read();
            if (!packet)
                return -1;

            g_fakeBuffer->Free();

            if (packet->header.type == 0xff) {
                continue;
            }
            else {
                auto eventData = reinterpret_cast<bluetooth::HidEventData *>(buffer);

                *type = static_cast<bluetooth::HidEventType>(packet->header.type);
                std::memcpy(&eventData->getReport.address, &packet->data.address, sizeof(bluetooth::Address));
                eventData->getReport.status = HidStatus_Ok;
                eventData->getReport.report_length = packet->header.size;

                std::memcpy(&eventData->getReport.report_data, &packet->data, packet->header.size);
                break;
            }      
        }

        //BTDRV_LOG_DATA_MSG(&packet->data, packet->header.size, "btdrv-mitm: hid::report::GetEventInfo -> Read");
        
        return ams::ResultSuccess();
    }

    void _HandleEvent() {

        while (true) {
            // Get packet from real buffer
            auto realPacket = g_realBuffer->Read();
            if (!realPacket)
                break;

            g_realBuffer->Free();

            switch (realPacket->header.type) {
                case 0xff:
                    // Skip over packet type 0xff. This packet indicates the buffer should wrap around on next read.
                    continue;
                    
                case 4:
                    {
                        // Locate the controller that sent the report
                        auto device = controller::locateController(hos::GetVersion() < hos::Version_9_0_0 ? &realPacket->data.address : &realPacket->data.v2.address);
                        if (!device) {
                            continue;
                        } 
                        
                        if (device->isSwitchController()) {
                            // Write unmodified packet directly to fake buffer
                            g_fakeBuffer->Write(realPacket->header.type, &realPacket->data, realPacket->header.size);
                        }
                        else {
                            const bluetooth::HidReport *inReport;
                            bluetooth::HidReport *outReport;
                            // copy address and stuff over
                            if (hos::GetVersion() < hos::Version_9_0_0) {
                                g_fakeReportData->size = 0x42;
                                std::memcpy(&g_fakeReportData->address, &realPacket->data.address, sizeof(bluetooth::Address));
                                inReport = &realPacket->data.report;
                                outReport = &g_fakeReportData->report;
                            }
                            else {
                                std::memcpy(&g_fakeReportData->v2.address, &realPacket->data.v2.address, sizeof(bluetooth::Address));
                                inReport = &realPacket->data.v2.report;
                                outReport = &g_fakeReportData->v2.report;
                            }

                            auto switchData = reinterpret_cast<controller::SwitchReportData *>(&outReport->data);
                            switchData->input0x30.timer = os::ConvertToTimeSpan(realPacket->header.timestamp).GetMilliSeconds() & 0xff;

                            // Translate packet to switch pro format
                            device->convertReportFormat(inReport, outReport);

                            // Write the converted report to our fake buffer
                            g_fakeBuffer->Write(4, g_fakeReportData, sizeof(g_fakeReportBuffer));                
                        }
                    }
                    break;

                default:
                    BTDRV_LOG_FMT("unknown packet received: %d", realPacket->header.type); 
                    g_fakeBuffer->Write(realPacket->header.type, &realPacket->data, realPacket->header.size);
                    break;
            }

        } 
    }

    void _HandleEventDeprecated(void) {

        std::scoped_lock lk(g_eventDataLock);
        R_ABORT_UNLESS(btdrvGetHidReportEventInfo(&g_currentEventType, g_eventDataBuffer, sizeof(g_eventDataBuffer)));

        auto eventData = reinterpret_cast<bluetooth::HidEventData *>(g_eventDataBuffer);

        //BTDRV_LOG_FMT("hid report event [%02d]", g_currentEventType);

        switch (g_currentEventType) {

            case HidEvent_GetReport:
                {
                    // Locate the controller that sent the report
                    auto device = controller::locateController(&eventData->getReport.address);
                    if (!device) {
                        return;
                    }
                    
                    if (device->isSwitchController()) {
                        //BTDRV_LOG_DATA_MSG(&eventData->getReport.report_data, eventData->getReport.report_length, "Switch controller -> Write");
                        g_fakeBuffer->Write(g_currentEventType, &eventData->getReport.report_data, eventData->getReport.report_length);
                    }
                    else {
                        const bluetooth::HidReport *inReport;
                        bluetooth::HidReport *outReport;

                        g_fakeReportData->size = 0x42; // Todo: check size is correct for report 0x30
                        std::memcpy(&g_fakeReportData->address, &eventData->getReport.address, sizeof(bluetooth::Address));
                        inReport = &eventData->getReport.report_data.report;
                        outReport = &g_fakeReportData->report;

                        auto switchData = reinterpret_cast<controller::SwitchReportData *>(&outReport->data);
                        switchData->input0x30.timer = os::ConvertToTimeSpan(os::GetSystemTick()).GetMilliSeconds() & 0xff;

                        // Translate packet to switch pro format
                        device->convertReportFormat(inReport, outReport);

                        // Write the converted report to our fake buffer
                        g_fakeBuffer->Write(4, g_fakeReportData, sizeof(g_fakeReportBuffer));  
                    }
                }
                break;

            default:
                BTDRV_LOG_FMT("unknown packet received: %d", g_currentEventType); 
                g_fakeBuffer->Write(g_currentEventType, &eventData->getReport.report_data, eventData->getReport.report_length);
                break;
        }
    }

    void HandleEvent(void) {
        
        if (hos::GetVersion() < hos::Version_7_0_0)
            _HandleEventDeprecated();
        else 
            _HandleEvent();

        if (!g_redirectHidReportEvents)
            os::SignalSystemEvent(&g_btHidReportSystemEventFwd);
        else
            os::SignalSystemEvent(&g_btHidReportSystemEventUser);

    }

}
