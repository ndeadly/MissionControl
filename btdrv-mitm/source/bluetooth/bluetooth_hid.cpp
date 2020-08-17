#include "bluetooth_hid.hpp"

#include <atomic>
#include <mutex>
#include <cstring>

#include "../controllers/controllermanager.hpp"
#include "../btdrv_mitm_flags.hpp"

#include "../btdrv_mitm_logging.hpp"

namespace ams::bluetooth::hid {

    namespace {

        std::atomic<bool> g_isInitialized(false);
        std::atomic<bool> g_bufferRead(false);

        os::Mutex g_eventDataLock(false);
        u8 g_eventDataBuffer[0x480];
        HidEventType g_currentEventType;

        os::SystemEventType g_systemEvent;
        os::SystemEventType g_systemEventFwd;
        os::SystemEventType g_systemEventUserFwd;
        os::EventType       g_dataReadEvent;

    }

    bool IsInitialized(void) {
        return g_isInitialized;
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
        os::AttachReadableHandleToSystemEvent(&g_systemEvent, eventHandle, false, os::EventClearMode_ManualClear);

        R_TRY(os::CreateSystemEvent(&g_systemEventFwd, os::EventClearMode_AutoClear, true));
        R_TRY(os::CreateSystemEvent(&g_systemEventUserFwd, os::EventClearMode_AutoClear, true));
        os::InitializeEvent(&g_dataReadEvent, false, os::EventClearMode_AutoClear);

        g_isInitialized = true;

        return ams::ResultSuccess();
    }

    void Finalize(void) {
        os::FinalizeEvent(&g_dataReadEvent);
        os::DestroySystemEvent(&g_systemEventUserFwd);
        os::DestroySystemEvent(&g_systemEventFwd); 

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
                controller::attachDeviceHandler(&eventData->connectionState.address);
                //BTDRV_LOG_FMT("device connected");
                break;
            case HidConnectionState_Disconnected:
                controller::removeDeviceHandler(&eventData->connectionState.address);
                //BTDRV_LOG_FMT("device disconnected");
                break;
            default:
                break;
        }
    }

    void handleUnknown07Event(HidEventData *eventData) {
        // Fix for xbox one disconnection. Don't know what this value is for, but it appears to be 0 for other controllers
        if (hos::GetVersion() < hos::Version_9_0_0)
            eventData->unknown07._unk1 = 0;
        else
            eventData->unknown07.v2._unk1 = 0;
    }

    void HandleEvent(void) {
        {
            std::scoped_lock lk(g_eventDataLock);
            R_ABORT_UNLESS(btdrvGetHidEventInfo(&g_currentEventType, g_eventDataBuffer, sizeof(g_eventDataBuffer)));
        }

        BTDRV_LOG_FMT("[%02d] HID Event", g_currentEventType);

        auto eventData = reinterpret_cast<HidEventData *>(g_eventDataBuffer);

        switch (g_currentEventType) {

            case HidEvent_ConnectionState:
                handleConnectionStateEvent(eventData);
                break;
            case HidEvent_Unknown07:
                handleUnknown07Event(eventData);
                break;
            default:
                BTDRV_LOG_DATA(g_eventDataBuffer, sizeof(g_eventDataBuffer));
                break;
        }

        os::SignalSystemEvent(&g_systemEventFwd);
        os::WaitEvent(&g_dataReadEvent);

        if (g_systemEventUserFwd.state)
            os::SignalSystemEvent(&g_systemEventUserFwd);

    }

}
