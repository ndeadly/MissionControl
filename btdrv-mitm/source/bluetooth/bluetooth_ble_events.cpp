#include "bluetooth_ble_events.hpp"
#include "../btdrv_mitm_flags.hpp"

#include "../btdrv_mitm_logging.hpp"

namespace ams::bluetooth::ble {

    namespace {

        os::ThreadType g_eventHandlerThread;
        alignas(os::ThreadStackAlignment) u8 g_eventHandlerThreadStack[0x2000];
        //u8 g_bt_ble_event_data_buffer[0x400];
        //BluetoothEventType g_current_bt_ble_event_type;

        os::SystemEventType g_btBleSystemEvent;
        os::SystemEventType g_btBleSystemEventFwd;
        os::SystemEventType g_btBleSystemEventUser;

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

    void HandleEvent(void) {
        BTDRV_LOG_FMT("ble event fired");
        
        // Signal our forwarder events
        if (!g_redirectEvents || g_preparingForSleep)
            os::SignalSystemEvent(&g_btBleSystemEventFwd);
        else
            os::SignalSystemEvent(&g_btBleSystemEventUser);
    }


    void BluetoothBleEventThreadFunc(void *arg) {
        while (true) {
            // Wait for real bluetooth event 
            os::WaitSystemEvent(&g_btBleSystemEvent);

            HandleEvent();
        }
    }

    Result InitializeEvents(void) {
        R_TRY(os::CreateSystemEvent(&g_btBleSystemEventFwd, os::EventClearMode_AutoClear, true));
        R_TRY(os::CreateSystemEvent(&g_btBleSystemEventUser, os::EventClearMode_AutoClear, true));

        return ams::ResultSuccess();
    }

    Result StartEventHandlerThread(void) {
        R_TRY(os::CreateThread(&g_eventHandlerThread, 
            BluetoothBleEventThreadFunc, 
            nullptr, 
            g_eventHandlerThreadStack, 
            sizeof(g_eventHandlerThreadStack), 
            9
            //38 // priority of btm sysmodule + 1
        ));

        os::StartThread(&g_eventHandlerThread); 

        return ams::ResultSuccess();
    }

}
