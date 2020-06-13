#include "bluetooth_ble_events.hpp"
#include "../btdrv_mitm_flags.hpp"

#include "../btdrv_mitm_logging.hpp"

ams::os::ThreadType g_bt_ble_event_task_thread;
alignas(ams::os::ThreadStackAlignment) u8 g_bt_ble_event_task_stack[0x2000];
//u8 g_bt_ble_event_data_buffer[0x400];
//BluetoothEventType g_current_bt_ble_event_type;

ams::os::SystemEventType g_btBleSystemEvent;
ams::os::SystemEventType g_btBleSystemEventFwd;
ams::os::SystemEventType g_btBleSystemEventUser;


void BluetoothBleEventThreadFunc(void *arg) {
    while (true) {
        // Wait for real bluetooth event 
        ams::os::WaitSystemEvent(&g_btBleSystemEvent);

        BTDRV_LOG_FMT("ble event fired");
        
        // Signal our forwarder events
        if (!g_redirectEvents || g_preparingForSleep)
            ams::os::SignalSystemEvent(&g_btBleSystemEventFwd);
        else
            ams::os::SignalSystemEvent(&g_btBleSystemEventUser);
    }
}

ams::Result InitializeBluetoothBleEvents(void) {
    R_TRY(ams::os::CreateSystemEvent(&g_btBleSystemEventFwd, ams::os::EventClearMode_AutoClear, true));
    R_TRY(ams::os::CreateSystemEvent(&g_btBleSystemEventUser, ams::os::EventClearMode_AutoClear, true));

    return ams::ResultSuccess();
}

ams::Result StartBluetoothBleEventThread(void) {
    R_TRY(ams::os::CreateThread(&g_bt_ble_event_task_thread, 
        BluetoothBleEventThreadFunc, 
        nullptr, 
        g_bt_ble_event_task_stack, 
        sizeof(g_bt_ble_event_task_stack), 
        9
        //38 // priority of btm sysmodule + 1
    ));

    ams::os::StartThread(&g_bt_ble_event_task_thread); 

    return ams::ResultSuccess();
}
