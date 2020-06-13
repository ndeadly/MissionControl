#include "bluetooth_core_events.hpp"
#include "../btdrv_mitm_flags.hpp"

#include "../btdrv_mitm_logging.hpp"

namespace ams::bluetooth::core {

    namespace {

        os::ThreadType g_eventHandlerThread;
        alignas(os::ThreadStackAlignment) u8 g_eventHandlerThreadStack[0x2000];
        //u8 g_eventDataBuffer[0x400];
        //BluetoothEventType g_currentEventType;

        os::SystemEventType g_btSystemEvent;
        os::SystemEventType g_btSystemEventFwd;
        os::SystemEventType g_btSystemEventUser;

    }

    os::SystemEventType *GetSystemEvent(void) {
        return &g_btSystemEvent;
    }

    os::SystemEventType *GetForwardEvent(void) {
        return &g_btSystemEventFwd;
    }

    os::SystemEventType *GetUserForwardEvent(void) {
        return &g_btSystemEventUser;
    }

    void HandleEvent(void) {
        BTDRV_LOG_FMT("bluetooth event fired");
        
        // Signal our forwarder events
        if (!g_redirectEvents || g_preparingForSleep)
            os::SignalSystemEvent(&g_btSystemEventFwd);
        else
            os::SignalSystemEvent(&g_btSystemEventUser);
    }

    void BluetoothEventThreadFunc(void *arg) {
        while (true) {
            // Wait for real bluetooth event 
            os::WaitSystemEvent(&g_btSystemEvent);

            HandleEvent();
        }
    }

    Result InitializeEvents(void) {
        R_TRY(os::CreateSystemEvent(&g_btSystemEventFwd, os::EventClearMode_AutoClear, true));
        R_TRY(os::CreateSystemEvent(&g_btSystemEventUser, os::EventClearMode_AutoClear, true));

        return ams::ResultSuccess();
    }

    Result StartEventHandlerThread(void) {
        R_TRY(os::CreateThread(&g_eventHandlerThread, 
                    BluetoothEventThreadFunc, 
                    nullptr, 
                    g_eventHandlerThreadStack, 
                    sizeof(g_eventHandlerThreadStack), 
                    9
                    //37 // priority of btm sysmodule
                ));

        os::StartThread(&g_eventHandlerThread); 

        return ams::ResultSuccess();
    }

}
