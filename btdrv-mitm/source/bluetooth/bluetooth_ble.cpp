#include "bluetooth_ble.hpp"

#include <atomic>
#include <mutex>
#include <cstring>
#include "../btdrv_mitm_flags.hpp"

#include "../btdrv_mitm_logging.hpp"

namespace ams::bluetooth::ble {

    namespace {

        std::atomic<bool> g_isInitialized(false);

        //os::ThreadType g_eventHandlerThread;
        //alignas(os::ThreadStackAlignment) u8 g_eventHandlerThreadStack[0x2000];

        os::Mutex g_eventDataLock(false);
        u8 g_eventDataBuffer[0x400];
        BleEventType g_currentEventType;

        os::SystemEventType g_btBleSystemEvent;
        os::SystemEventType g_btBleSystemEventFwd;
        os::SystemEventType g_btBleSystemEventUser;

        /*
        void EventThreadFunc(void *arg) {
            while (true) {
                os::WaitSystemEvent(&g_btBleSystemEvent);
                HandleEvent();
            }
        }
        */

    }

    bool IsInitialized(void) {
        return g_isInitialized;
    }

    os::SystemEventType *GetSystemEvent(void) {
        return &g_btBleSystemEvent;
    }

    os::SystemEventType *GetForwardEvent(void) {
        return &g_btBleSystemEventFwd;
    }

    os::SystemEventType *GetUserForwardEvent(void) {
        return &g_btBleSystemEventUser;
    }

    Result Initialize(Handle eventHandle) {
        //os::AttachReadableHandleToSystemEvent(&g_btBleSystemEvent, eventHandle, false, os::EventClearMode_AutoClear);
        os::AttachReadableHandleToSystemEvent(&g_btBleSystemEvent, eventHandle, true, os::EventClearMode_AutoClear);

        R_TRY(os::CreateSystemEvent(&g_btBleSystemEventFwd, os::EventClearMode_AutoClear, true));
        R_TRY(os::CreateSystemEvent(&g_btBleSystemEventUser, os::EventClearMode_AutoClear, true));

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
        //os::DestroyThread(&g_eventHandlerThread);

        os::DestroySystemEvent(&g_btBleSystemEventUser);
        os::DestroySystemEvent(&g_btBleSystemEventFwd);

        g_isInitialized = false;
    }

    Result GetEventInfo(ncm::ProgramId program_id, BleEventType *type, u8* buffer, size_t size) {
        std::scoped_lock lk(g_eventDataLock); 
        {
            *type = g_currentEventType;
            std::memcpy(buffer, g_eventDataBuffer, size);
        }
        
        return ams::ResultSuccess();
    }

    void HandleEvent(void) {
        std::scoped_lock lk(g_eventDataLock);

        R_ABORT_UNLESS(btdrvGetBleManagedEventInfo(&g_currentEventType, g_eventDataBuffer, sizeof(g_eventDataBuffer)));

        BTDRV_LOG_FMT("[%02d] BLE Event", g_currentEventType);
        
        // Signal our forwarder events
        //if (!g_redirectEvents || g_preparingForSleep)
        if (!g_redirectEvents)
            os::SignalSystemEvent(&g_btBleSystemEventFwd);
        else
            os::SignalSystemEvent(&g_btBleSystemEventUser);
    }

}
