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

        //os::ThreadType g_eventHandlerThread;
        //alignas(os::ThreadStackAlignment) u8 g_eventHandlerThreadStack[0x2000];

        os::Mutex g_eventDataLock(false);
        u8 g_eventDataBuffer[0x480];
        HidEventType g_currentEventType;

        os::SystemEventType g_btHidSystemEvent;
        os::SystemEventType g_btHidSystemEventFwd;
        os::SystemEventType g_btHidSystemEventUser;

        /*
        void EventThreadFunc(void *arg) {
            while (true) {
                os::WaitSystemEvent(&g_btHidSystemEvent);
                HandleEvent();
            }
        }
        */

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
        /*
        if (hos::GetVersion() >= hos::Version_7_0_0)
            R_ABORT_UNLESS(hiddbgAttachHdlsWorkBuffer());
        */
        //os::AttachReadableHandleToSystemEvent(&g_btHidSystemEvent, eventHandle, false, os::EventClearMode_AutoClear);
        os::AttachReadableHandleToSystemEvent(&g_btHidSystemEvent, eventHandle, true, os::EventClearMode_AutoClear);

        R_TRY(os::CreateSystemEvent(&g_btHidSystemEventFwd, os::EventClearMode_AutoClear, true));
        R_TRY(os::CreateSystemEvent(&g_btHidSystemEventUser, os::EventClearMode_AutoClear, true));

        /*
        R_TRY(os::CreateThread(&g_eventHandlerThread, 
            EventThreadFunc, 
            nullptr, 
            g_eventHandlerThreadStack, 
            sizeof(g_eventHandlerThreadStack), 
            9
        ));

        os::StartThread(&g_eventHandlerThread); 
        */

        g_isInitialized = true;

        return ams::ResultSuccess();
    }

    void Finalize(void) {
        /*
        if (hos::GetVersion() >= hos::Version_7_0_0)
            R_ABORT_UNLESS(hiddbgReleaseHdlsWorkBuffer());
        */

        //os::DestroyThread(&g_eventHandlerThread);

        os::DestroySystemEvent(&g_btHidSystemEventUser);
        os::DestroySystemEvent(&g_btHidSystemEventFwd); 

        g_isInitialized = false;           
    }

    Result GetEventInfo(ncm::ProgramId program_id, HidEventType *type, u8* buffer, size_t size) {
        std::scoped_lock lk(g_eventDataLock);

        *type = g_currentEventType;
        std::memcpy(buffer, g_eventDataBuffer, size);

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
        std::scoped_lock lk(g_eventDataLock);
        
        R_ABORT_UNLESS(btdrvGetHidEventInfo(&g_currentEventType, g_eventDataBuffer, sizeof(g_eventDataBuffer)));

        BTDRV_LOG_FMT("[%02d] HID Event", g_currentEventType);

        HidEventData *eventData = reinterpret_cast<HidEventData *>(g_eventDataBuffer);

        switch (g_currentEventType) {

            case HidEvent_ConnectionState:
                handleConnectionStateEvent(eventData);
                break;

            default:
                break;
        }

            // Signal our forwarder events
        os::SignalSystemEvent(&g_btHidSystemEventFwd);
        os::SignalSystemEvent(&g_btHidSystemEventUser);
        //if (!g_redirectEvents || g_preparingForSleep)
        /*
        if (!g_redirectEvents || g_currentEventType == HidEvent_Unknown07)
            os::SignalSystemEvent(&g_btHidSystemEventFwd);
        else
            os::SignalSystemEvent(&g_btHidSystemEventUser);
        */
    }

}
