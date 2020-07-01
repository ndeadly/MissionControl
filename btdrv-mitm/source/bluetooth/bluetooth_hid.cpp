#include "bluetooth_hid.hpp"

#include <atomic>
#include <mutex>
#include <cstring>

#include "../controllermanager.hpp"
#include "../btdrv_mitm_flags.hpp"

#include "../btdrv_mitm_logging.hpp"

namespace ams::bluetooth::hid {

    namespace {

        std::atomic<bool> g_isInitialized(false);
        std::atomic<bool> g_bufferRead(false);

        os::Mutex g_eventDataLock(false);
        u8 g_eventDataBuffer[0x480];
        HidEventType g_currentEventType;

        os::SystemEventType g_btHidSystemEvent;
        os::SystemEventType g_btHidSystemEventFwd;
        os::SystemEventType g_btHidSystemEventUser;
        os::EventType       g_dataReadEvent;

    }

    bool IsInitialized(void) {
        return g_isInitialized;
    }

    os::SystemEventType *GetSystemEvent(void) {
        return &g_btHidSystemEvent;
    }

    os::SystemEventType *GetForwardEvent(void) {
        return &g_btHidSystemEventFwd;
    }

    os::SystemEventType *GetUserForwardEvent(void) {
        return &g_btHidSystemEventUser;
    }

    Result Initialize(Handle eventHandle) {
        //os::AttachReadableHandleToSystemEvent(&g_btHidSystemEvent, eventHandle, false, os::EventClearMode_AutoClear);
        os::AttachReadableHandleToSystemEvent(&g_btHidSystemEvent, eventHandle, false, os::EventClearMode_ManualClear);

        R_TRY(os::CreateSystemEvent(&g_btHidSystemEventFwd, os::EventClearMode_AutoClear, true));
        R_TRY(os::CreateSystemEvent(&g_btHidSystemEventUser, os::EventClearMode_AutoClear, true));
        os::InitializeEvent(&g_dataReadEvent, false, os::EventClearMode_AutoClear);

        g_isInitialized = true;

        return ams::ResultSuccess();
    }

    void Finalize(void) {
        os::FinalizeEvent(&g_dataReadEvent);
        os::DestroySystemEvent(&g_btHidSystemEventUser);
        os::DestroySystemEvent(&g_btHidSystemEventFwd); 

        g_isInitialized = false;           
    }

    Result GetEventInfo(ncm::ProgramId program_id, HidEventType *type, u8* buffer, size_t size) {
        std::scoped_lock lk(g_eventDataLock);

        *type = g_currentEventType;
        std::memcpy(buffer, g_eventDataBuffer, size);

        os::SignalEvent(&g_dataReadEvent);

        return ams::ResultSuccess();
    }

    void handleConnectionStateEvent(HidEventData *eventData) {
        switch (eventData->connectionState.state) {
            case HidConnectionState_Connected:
                ams::mitm::btdrv::attachDeviceHandler(&eventData->connectionState.address);
                BTDRV_LOG_FMT("device connected");
                break;
            case HidConnectionState_Disconnected:
                ams::mitm::btdrv::removeDeviceHandler(&eventData->connectionState.address);
                BTDRV_LOG_FMT("device disconnected");
                break;
            default:
                break;
        }
    }

    void HandleEvent(void) {
        {
            std::scoped_lock lk(g_eventDataLock);
            R_ABORT_UNLESS(btdrvGetHidEventInfo(&g_currentEventType, g_eventDataBuffer, sizeof(g_eventDataBuffer)));
        }

        BTDRV_LOG_FMT("[%02d] HID Event", g_currentEventType);

        os::SignalSystemEvent(&g_btHidSystemEventFwd);
        os::WaitEvent(&g_dataReadEvent);

        if (g_btHidSystemEventUser.state) {
            os::SignalSystemEvent(&g_btHidSystemEventUser);
            //os::TimedWaitEvent(&g_dataReadEvent, TimeSpan::FromMilliSeconds(500));
        }

        HidEventData *eventData = reinterpret_cast<HidEventData *>(g_eventDataBuffer);
        switch (g_currentEventType) {

            case HidEvent_ConnectionState:
                handleConnectionStateEvent(eventData);
                break;

            default:
                break;
        }

    }

}
