#include "bluetooth_core_events.hpp"
#include "../btdrv_mitm_flags.hpp"

#include "../btdrv_mitm_logging.hpp"

ams::os::ThreadType g_bt_event_task_thread;
alignas(ams::os::ThreadStackAlignment) u8 g_bt_event_task_stack[0x2000];
//u8 g_bt_event_data_buffer[0x400];
//BluetoothEventType g_current_bt_event_type;

ams::os::SystemEventType g_btSystemEvent;
ams::os::SystemEventType g_btSystemEventFwd;
ams::os::SystemEventType g_btSystemEventUser;


void BluetoothEventThreadFunc(void *arg) {
    while (true) {
        // Wait for real bluetooth event 
        ams::os::WaitSystemEvent(&g_btSystemEvent);

        BTDRV_LOG_FMT("bluetooth event fired");
        
        // Signal our forwarder events
        if (!g_redirectEvents || g_preparingForSleep)
            ams::os::SignalSystemEvent(&g_btSystemEventFwd);
        else
            ams::os::SignalSystemEvent(&g_btSystemEventUser);
    }
}

ams::Result InitializeBluetoothCoreEvents(void) {
    R_TRY(ams::os::CreateSystemEvent(&g_btSystemEventFwd, ams::os::EventClearMode_AutoClear, true));
    R_TRY(ams::os::CreateSystemEvent(&g_btSystemEventUser, ams::os::EventClearMode_AutoClear, true));

    return ams::ResultSuccess();
}

ams::Result StartBluetoothCoreEventThread(void) {
    R_TRY(ams::os::CreateThread(&g_bt_event_task_thread, 
                BluetoothEventThreadFunc, 
                nullptr, 
                g_bt_event_task_stack, 
                sizeof(g_bt_event_task_stack), 
                9
                //37 // priority of btm sysmodule
            ));

    ams::os::StartThread(&g_bt_event_task_thread); 

    return ams::ResultSuccess();
}
