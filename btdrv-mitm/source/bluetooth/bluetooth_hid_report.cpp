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
        alignas(os::ThreadStackAlignment) u8 g_eventHandlerThreadStack[0x1000];

        // This is only required  on fw < 7.0.0
        u8 g_eventDataBuffer[0x480];
        bluetooth::HidEventType g_currentEventType;

        os::SystemEventType g_systemEvent;
        os::SystemEventType g_systemEventFwd;
        os::SystemEventType g_systemEventUserFwd;

        SharedMemory g_realBtShmem;
        SharedMemory g_fakeBtShmem;

        bluetooth::CircularBuffer *g_realBuffer;
        bluetooth::CircularBuffer *g_fakeBuffer;

        bluetooth::HidReportData g_fakeReportData;

        void EventThreadFunc(void *arg) {
            while (true) {
                os::WaitSystemEvent(&g_systemEvent);
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
        return &g_systemEvent;
    }

    os::SystemEventType *GetForwardEvent(void) {
        return &g_systemEventFwd;
    }

    os::SystemEventType *GetUserForwardEvent(void) {
        return &g_systemEventUserFwd;
    }

    Result Initialize(Handle eventHandle) {
        os::AttachReadableHandleToSystemEvent(&g_systemEvent, eventHandle, false, os::EventClearMode_AutoClear);

        R_TRY(os::CreateSystemEvent(&g_systemEventFwd, os::EventClearMode_AutoClear, true));
        R_TRY(os::CreateSystemEvent(&g_systemEventUserFwd, os::EventClearMode_AutoClear, true));

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

        os::DestroySystemEvent(&g_systemEventUserFwd);
        os::DestroySystemEvent(&g_systemEventFwd); 

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

    Result WriteHidReportBuffer(const bluetooth::Address *address, const bluetooth::HidReport *report) {
        if (hos::GetVersion() < hos::Version_9_0_0) {
            g_fakeReportData.size = g_fakeReportData.report.size + 0x11;
            std::memcpy(&g_fakeReportData.address, address, sizeof(bluetooth::Address));
        }
        else {
            std::memcpy(&g_fakeReportData.v2.address, address, sizeof(bluetooth::Address));
        }
        std::memcpy(&g_fakeReportData.report, report, report->size + sizeof(report->size));

        g_fakeBuffer->Write(HidEvent_GetReport, &g_fakeReportData, g_fakeReportData.report.size + 0x11); 

        os::SignalSystemEvent(&g_systemEventFwd);

        return ams::ResultSuccess();
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

    void HandleEvent(void) {
        if (!g_redirectHidReportEvents) {
            if (hos::GetVersion() < hos::Version_7_0_0) {
                auto eventData = reinterpret_cast<bluetooth::HidEventData *>(g_eventDataBuffer);
                R_ABORT_UNLESS(btdrvGetHidReportEventInfo(&g_currentEventType, g_eventDataBuffer, sizeof(g_eventDataBuffer)));

                switch (g_currentEventType) {
                    case HidEvent_GetReport:
                        {
                            auto device = controller::locateHandler(&eventData->getReport.address);
                            if (!device)
                                return;

                            device->handleIncomingReport(&eventData->getReport.report_data.report);
                        }
                        break;
                    default:
                        g_fakeBuffer->Write(g_currentEventType, &eventData->getReport.report_data, eventData->getReport.report_length);
                        break;
                }
            }
            else {
                while (true) {
                    auto realPacket = g_realBuffer->Read();
                    if (!realPacket)
                        break;

                    g_realBuffer->Free();

                    switch (realPacket->header.type) {
                        case 0xff:
                            continue;
                        case HidEvent_GetReport:
                            {
                                auto device = controller::locateHandler(hos::GetVersion() < hos::Version_9_0_0 ? &realPacket->data.address : &realPacket->data.v2.address);
                                if (!device)
                                    continue;

                                device->handleIncomingReport(&realPacket->data.report);
                            }
                            break;
                        default:
                            g_fakeBuffer->Write(realPacket->header.type, &realPacket->data, realPacket->header.size);
                            break;
                    }
                } 
            }
        }
        else {
            os::SignalSystemEvent(&g_systemEventUserFwd);
        }
    }

}
