#include "bluetooth_ble.hpp"

#include <atomic>
#include <mutex>
#include <cstring>

#include "../btdrv_mitm_flags.hpp"

#include "../btdrv_mitm_logging.hpp"

namespace ams::bluetooth::ble {

    namespace {

        std::atomic<bool> g_isInitialized(false);

        os::Mutex g_eventDataLock(false);
        u8 g_eventDataBuffer[0x400];
        BleEventType g_currentEventType;

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

    Result GetEventInfo(ncm::ProgramId program_id, BleEventType *type, u8* buffer, size_t size) {
        std::scoped_lock lk(g_eventDataLock); 

        *type = g_currentEventType;
        std::memcpy(buffer, g_eventDataBuffer, size);

        os::SignalEvent(&g_dataReadEvent);
        
        return ams::ResultSuccess();
    }

    void HandleEvent(void) {
        {
            std::scoped_lock lk(g_eventDataLock); 
            R_ABORT_UNLESS(btdrvGetBleManagedEventInfo(&g_currentEventType, g_eventDataBuffer, sizeof(g_eventDataBuffer)));
        }

        BTDRV_LOG_FMT("[%02d] BLE Event", g_currentEventType);

        if (!g_redirectBleEvents) {
            os::SignalSystemEvent(&g_systemEventFwd);
            os::WaitEvent(&g_dataReadEvent);
        }

        if (g_systemEventUserFwd.state) {
            os::SignalSystemEvent(&g_systemEventUserFwd);
        }

    }

}
